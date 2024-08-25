// Copyright (c) 2024 Robin Davies
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "Updater.hpp"
#include "json.hpp"
#include "config.hpp"
#include <sys/eventfd.h>
#include <unistd.h>
#include <stdexcept>
#include <chrono>
#include <poll.h>
#include "Lv2Log.hpp"
#include "SysExec.hpp"
#include "json_variant.hpp"
#include "ss.hpp"
#include "Lv2Log.hpp"
#include <algorithm>

using namespace pipedal;

#define TEST_UPDATE

#ifndef DEBUG
#undef TEST_UPDATE // do NOT leat this leak into a production build!
#endif

static constexpr uint64_t CLOSE_EVENT = 0;
static constexpr uint64_t CHECK_NOW_EVENT = 1;
static constexpr uint64_t UNCACHED_CHECK_NOW_EVENT = 2;


static std::filesystem::path WORKING_DIRECTORY = "/var/pipedal/updates";
static std::filesystem::path UPDATE_STATUS_CACHE_FILE = WORKING_DIRECTORY / "updateStatus.json";

static std::string GITHUB_RELEASES_URL = "https://api.github.com/repos/rerdavies/pipedal/releases";

Updater::clock::duration Updater::updateRate = std::chrono::duration_cast<Updater::clock::duration>(std::chrono::days(1));
static std::chrono::system_clock::duration CACHE_DURATION = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::minutes(30));

std::mutex cacheMutex;
static UpdateStatus GetCachedUpdateStatus()
{
    std::lock_guard lock{cacheMutex};
    try
    {
        if (std::filesystem::exists(UPDATE_STATUS_CACHE_FILE))
        {
            std::ifstream f{UPDATE_STATUS_CACHE_FILE};
            if (!f.is_open())
            {
                json_reader reader(f);
                UpdateStatus status;
                reader.read(&status);

                // cached curruent version might come from a different version.
                status.ResetCurrentVersion();
                return status;
            }
        }
    }
    catch (const std::exception &e)
    {
        Lv2Log::error(SS("Unable to read cached UpdateStatus. " << e.what()));
    }
    return UpdateStatus();
}

static void SetCachedUpdateStatus(UpdateStatus &updateStatus)
{
    std::lock_guard lock{cacheMutex};
    updateStatus.LastUpdateTime(std::chrono::system_clock::now());
    try
    {
        std::ofstream f{UPDATE_STATUS_CACHE_FILE};
        json_writer writer{f};
        writer.write(updateStatus);
    }
    catch (const std::exception &e)
    {
        Lv2Log::error(SS("Unable to write cached UpdateStatus. " << e.what()));
    }
}
Updater::Updater()
{
    cachedUpdateStatus = GetCachedUpdateStatus();
    updatePolicy = cachedUpdateStatus.UpdatePolicy();
    currentResult = cachedUpdateStatus;

    int fds[2];
    int rc = pipe(fds);
    if (rc != 0)
    {
        throw std::runtime_error("Updater: cant create event pipe.");
    }
    this->event_reader = fds[0];
    this->event_writer = fds[1];

    this->thread = std::make_unique<std::thread>([this]()
                                                 { ThreadProc(); });
    CheckNow();
}
Updater::~Updater()
{
    Stop();
}
void Updater::Stop()
{
    if (stopped)
    {
        return;
    }
    stopped = true;

    if (event_writer != -1)
    {
        uint64_t value = CLOSE_EVENT;
        write(this->event_writer, &value, sizeof(uint64_t));
    }
    if (thread)
    {
        thread->join();
        thread = nullptr;
    }
    if (event_reader != -1)
    {
        close(event_reader);
    }
    if (event_writer != -1)
    {
        close(event_writer);
    }
}

void Updater::CheckNow()
{
    uint64_t value = CHECK_NOW_EVENT;
    write(this->event_writer, &value, sizeof(uint64_t));
}

void Updater::SetUpdateListener(UpdateListener &&listener)
{
    std::lock_guard lock{mutex};
    this->listener = listener;
    if (hasInfo)
    {
        listener(currentResult);
    }
}

void Updater::ThreadProc()
{
    struct pollfd pfd;
    pfd.fd = this->event_reader;
    pfd.events = POLLIN;

    while (true)
    {
        int ret = poll(&pfd, 1, std::chrono::duration_cast<std::chrono::milliseconds>(updateRate).count()); // 1000 ms timeout

        if (ret == -1)
        {
            Lv2Log::error("Updater: Poll error.");
            break;
        }
        else if (ret == 0)
        {
            CheckForUpdate(true);
        }
        else
        {
            // Event occurred
            uint64_t value;
            ssize_t s = read(event_reader, &value, sizeof(uint64_t));
            if (s == sizeof(uint64_t))
            {
                if (value == CHECK_NOW_EVENT)
                {
                    CheckForUpdate(true);
                } else if (value == UNCACHED_CHECK_NOW_EVENT)
                {
                    CheckForUpdate(false);
                }
                else
                {
                    break;
                }
            }
        }
    }
}

class GithubAsset
{
public:
    GithubAsset(json_variant &v);
    std::string name;
    std::string browser_download_url;
    std::string updated_at;
};
class GithubRelease
{
public:
    GithubRelease(json_variant &v);

    const GithubAsset *GetDownloadForCurrentArchitecture() const;
    bool draft = true;
    bool prerelease = true;
    std::string name;
    std::string url;
    std::string version;
    std::string body;
    std::vector<GithubAsset> assets;
    std::string published_at;
};

GithubAsset::GithubAsset(json_variant &v)
{
    auto o = v.as_object();
    this->name = o->at("name").as_string();
    this->browser_download_url = o->at("browser_download_url").as_string();
    this->updated_at = o->at("updated_at").as_string();
}
GithubRelease::GithubRelease(json_variant &v)
{
    auto o = v.as_object();
    this->name = o->at("name").as_string();
    this->draft = o->at("draft").as_bool();
    this->prerelease = o->at("prerelease").as_bool();
    this->body = o->at("body").as_string();

    auto assets = o->at("assets").as_array();
    for (size_t i = 0; i < assets->size(); ++i)
    {
        auto &el = assets->at(i);
        this->assets.push_back(GithubAsset(el));
    }
    this->published_at = o->at("published_at").as_string();
}

static std::vector<std::string> split(const std::string &s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}
static std::string justTheVersion(const std::string &assetName)
{
    // eg. pipedal_1.2.41_arm64.deb
    auto t = split(assetName, '_');
    if (t.size() != 3)
    {
        throw std::runtime_error("Unable to parse version.");
    }
    return t[1];
}

int compareVersions(const std::string &l, const std::string &r)
{
    std::stringstream sl(l);
    std::stringstream sr(r);

    int majorL = -1, majorR = 1, minorL = -1,
        minorR = -1, buildL = -1, buildR = -1;
    sl >> majorL;
    sr >> majorR;
    if (majorL != majorR)
    {
        return (majorL < majorR) ? -1 : 1;
    }
    char discard;
    sl >> discard >> minorL;
    sr >> discard >> minorR;
    if (minorL != minorR)
    {
        return minorL < minorR ? -1 : 1;
    }
    sl >> discard >> buildL;
    sr >> discard >> buildR;

    if (buildL != buildR)
    {
        return buildL < buildR ? -1 : 1;
    }
    return 0;
}
static std::string normalizeReleaseName(const std::string &releaseName)
{
    // e.g.   "PiPedal 1.2.34 Release" -> "PiPedal v1.2.34-Release"
    if (releaseName.empty())
        return "";

    std::string result = releaseName;

    auto nPos = result.find(' ');
    if (nPos == std::string::npos)
    {
        return result;
    }
    ++nPos;
    if (nPos >= result.length())
    {
        return result;
    }
    char c = releaseName[nPos];
    if (c >= '0' && c <= '9')
    {
        result.insert(result.begin() + nPos, 'v');
    }
    nPos = result.find(' ', nPos);
    if (nPos != std::string::npos)
    {
        result.at(nPos) = '-';
    }
    return result;
}

static bool IsCacheValid(const UpdateStatus &updateStatus)
{
    if (!updateStatus.IsValid() || !updateStatus.IsOnline())
    {
        return false;
    }
    auto now = std::chrono::system_clock::now();
    auto validStart = updateStatus.LastUpdateTime();
    auto validEnd = validStart + CACHE_DURATION;
    return now >= validStart && now < validEnd;
}



UpdateRelease Updater::getUpdateRelease(
    const std::vector<GithubRelease> &githubReleases,
    const std::string &currentVersion,
    const UpdateReleasePredicate &predicate)
{
    for (const auto &githubRelease : githubReleases)
    {
        auto *asset = githubRelease.GetDownloadForCurrentArchitecture();
        if (!asset)
            continue;

        if (!predicate(githubRelease))
            continue;
        UpdateRelease updateRelease;
        updateRelease.upgradeVersion_ = justTheVersion(asset->name);
        updateRelease.updateAvailable_ = compareVersions(currentVersion, updateRelease.upgradeVersion_) < 0;
        updateRelease.upgradeVersionDisplayName_ = normalizeReleaseName(githubRelease.name);
        updateRelease.assetName_ = asset->name;
        updateRelease.updateUrl_ = asset->browser_download_url;
        return updateRelease;
    }
    return UpdateRelease();
}

void Updater::CheckForUpdate(bool useCache)
{
    UpdateStatus updateResult; 

    {
        std::lock_guard lock {mutex};
        if (useCache && IsCacheValid(cachedUpdateStatus))
        {
            this->currentResult = cachedUpdateStatus;
            this->currentResult.UpdatePolicy(this->updatePolicy);
            if (listener)
            {
                listener(this->currentResult);
            }
            return;
        }
        updateResult = this->currentResult;
    }
    std::string args = SS("-s " << GITHUB_RELEASES_URL);


    updateResult.errorMessage_ = "";

    try
    {
        auto result = sysExecForOutput("curl", args);
        if (result.exitCode != EXIT_SUCCESS)
        {
            throw std::runtime_error("Server has no internet access.");
        }
        else
        {
            std::stringstream ss(result.output);
            json_reader reader(ss);
            json_variant vResult(reader);

            if (vResult.is_object())
            {
                // an HTML error.
                updateResult.isOnline_ = false;
                auto o = vResult.as_object();
                std::string message = o->at("message").as_string();
                auto status_code = o->at("status_code").as_int64();
                throw std::runtime_error(SS("Service error. ()" << status_code << ": " << message << ")"));
            }
            else
            {
                json_variant::array_ptr vArray = vResult.as_array();

                std::vector<GithubRelease> releases;
                for (size_t i = 0; i < vArray->size(); ++i)
                {
                    auto &el = vArray->at(0);
                    GithubRelease release{el};
                    if (!release.draft && release.GetDownloadForCurrentArchitecture() != nullptr)
                    {
                        releases.push_back(std::move(release));
                    }
                }
                std::sort(
                    releases.begin(),
                    releases.end(),
                    [](const GithubRelease &left, const GithubRelease &right)
                    {
                        return left.published_at > right.published_at; // latest date first.
                    });
                updateResult.releaseOnlyRelease_ = getUpdateRelease(
                    releases,
                    updateResult.currentVersion_,
                    [](const GithubRelease &githubRelease)
                    {
                        return !githubRelease.prerelease &&
                               githubRelease.name.find("Release") != std::string::npos;
                    });

                updateResult.releaseOrBetaRelease_ = getUpdateRelease(
                    releases,
                    updateResult.currentVersion_,
                    [](const GithubRelease &githubRelease)
                    {
                        return !githubRelease.prerelease &&
                               (githubRelease.name.find("Release") != std::string::npos ||
                                githubRelease.name.find("Beta") != std::string::npos);
                    });
                updateResult.devRelease_ = getUpdateRelease(
                    releases,
                    updateResult.currentVersion_,
                    [](const GithubRelease &githubRelease)
                    {
                        return true;
                    });
#ifdef TEST_UPDATE
                updateResult.releaseOrBetaRelease_.upgradeVersionDisplayName_ = "PiPedal v1.2.41-Beta";
                updateResult.devRelease_.upgradeVersionDisplayName_ = "PiPedal v1.2.39-Experimental";
                updateResult.devRelease_.upgradeVersion_ = "1.2.39";
                updateResult.devRelease_.updateAvailable_ = false;
#endif
                updateResult.isValid_ = true;
                updateResult.isOnline_ = true;
            }
        }
    }
    catch (const std::exception &e)
    {
        Lv2Log::error(SS("Failed to fetch update info. " << e.what()));
        updateResult.errorMessage_ = e.what();
        updateResult.isValid_ = false;
        updateResult.isOnline_ = false;
    }
    {
        std::lock_guard lock{mutex};
        updateResult.UpdatePolicy(this->updatePolicy);
        this->currentResult = updateResult;
        SetCachedUpdateStatus(this->currentResult);
        if (listener)
        {
            listener(this->currentResult);
        }
    }
}
bool UpdateRelease::operator==(const UpdateRelease &other) const
{
    return (updateAvailable_ == other.updateAvailable_) &&
           (upgradeVersion_ == other.upgradeVersion_) &&
           (upgradeVersionDisplayName_ == other.upgradeVersionDisplayName_) &&
           (assetName_ == other.assetName_) &&
           (updateUrl_ == other.updateUrl_);
}

bool UpdateStatus::operator==(const UpdateStatus &other) const
{
    return (lastUpdateTime_ == other.lastUpdateTime_) &&
           (isValid_ == other.isValid_) &&
           (errorMessage_ == other.errorMessage_) &&
           (isOnline_ == other.isOnline_) &&
           (currentVersion_ == other.currentVersion_) &&
           (currentVersionDisplayName_ == other.currentVersionDisplayName_) &&
           (updatePolicy_ == other.updatePolicy_) &&
           (releaseOnlyRelease_ == other.releaseOnlyRelease_) &&
           (releaseOrBetaRelease_ == other.releaseOrBetaRelease_) &&
           (devRelease_ == other.devRelease_);
}

UpdatePolicyT Updater::GetUpdatePolicy()
{
    std::lock_guard lock{mutex};
    return updatePolicy;
}
void Updater::SetUpdatePolicy(UpdatePolicyT updatePolicy)
{
    std::lock_guard lock{mutex};
    if (updatePolicy == this->updatePolicy)
        return;
    this->updatePolicy = updatePolicy;
    if (this->currentResult.UpdatePolicy() != updatePolicy)
    {
        this->currentResult.UpdatePolicy(updatePolicy);
        SetCachedUpdateStatus(this->currentResult);
        if (listener)
        {
            listener(currentResult);
        }
    }
}
void Updater::ForceUpdateCheck()
{
    uint64_t value = UNCACHED_CHECK_NOW_EVENT;
    write(this->event_writer, &value, sizeof(uint64_t));
}

UpdateStatus::UpdateStatus()
{
    currentVersion_ = PROJECT_VER;
    currentVersionDisplayName_ = PROJECT_DISPLAY_VERSION;

#ifdef TEST_UPDATE
    // uncomment this line to test upgrading.
    currentVersion_ = "1.2.39";
    currentVersionDisplayName_ = "PiPedal 1.2.39-Debug";
#endif
}

void UpdateStatus::ResetCurrentVersion()
{
    currentVersion_ = PROJECT_VER;
    currentVersionDisplayName_ = PROJECT_DISPLAY_VERSION;

#ifdef TEST_UPDATE
    // uncomment this line to test upgrading.
    currentVersion_ = "1.2.39";
    currentVersionDisplayName_ = "PiPedal 1.2.39-Debug";
#endif
}


std::chrono::system_clock::time_point UpdateStatus::LastUpdateTime() const
{
    std::chrono::system_clock::duration duration{this->lastUpdateTime_};
    std::chrono::system_clock::time_point tp{duration};
    return tp;
}

void UpdateStatus::LastUpdateTime(const std::chrono::system_clock::time_point &timePoint)
{
    this->lastUpdateTime_ = timePoint.time_since_epoch().count();
}

const GithubAsset *GithubRelease::GetDownloadForCurrentArchitecture() const
{
    // deb package names end in {DEBIAN_ARCHITECTURE}.deb
    // pipedal build gets this value from `dpkg --print-architecture`
#ifndef DEBIAN_ARCHITECTURE // deb package names end in {DEBIAN_ARCHITECTURE}.deb
#error DEBIAN_ARCHITECTURE not defined
#endif
    std::string downloadEnding = SS("_" << (DEBIAN_ARCHITECTURE) << ".deb");

    for (auto &asset : assets)
    {
        if (asset.name.ends_with(downloadEnding))
        {
            return &asset;
        }
    }
    return nullptr;
}


UpdateRelease::UpdateRelease()
{
}

std::string Updater::GetUpdateFilename(const std::string &url)
{
    std::lock_guard lock(mutex);

    // partialy whitelisting, partly avoiding having to parse a URL.
    if (this->currentResult.releaseOnlyRelease_.UpdateUrl() == url)
    {
        return this->currentResult.releaseOnlyRelease_.AssetName();
    }
    if (this->currentResult.releaseOrBetaRelease_.UpdateUrl() == url)
    {
        return this->currentResult.releaseOrBetaRelease_.AssetName();
    }
    if (this->currentResult.devRelease_.UpdateUrl() == url)
    {
        return this->currentResult.devRelease_.AssetName();
    }
    throw std::runtime_error("Permission denied. Invalid url.");

}
static std::string unCRLF(const std::string &text)
{
    std::ostringstream ss;
    for (char c : text)
    {
        if (c == '\r')
            continue;
        if (c == '\n') {
            ss << '/';
        } else 
        {
            ss << c;
        }
    }
    return ss.str();
}

static void removeOldSiblings(int numberToKeep, const std::filesystem::path &fileToKeep)
{
    namespace fs = std::filesystem;

    auto directory = fileToKeep.parent_path();
    if (directory.empty()) return; // superstition.
    struct RemoveEntry {
        fs::path path;
        fs::file_time_type time;
    };
    std::vector<RemoveEntry> entries;
    for (const auto&dirEntry : fs::directory_iterator(directory))
    {
        if (dirEntry.is_regular_file())
        {
            if (dirEntry.path() != fileToKeep)
            {
                dirEntry.last_write_time();
                entries.push_back(RemoveEntry { .path = dirEntry.path(), .time = dirEntry.last_write_time()});
            }
        }
    }
    std::sort(
        entries.begin(),entries.end(),
        [](const RemoveEntry&left, const RemoveEntry&right)
        {
            return left.time > right.time; // by time descending
        }
    );
    for (size_t i = numberToKeep; i < entries.size(); ++i)
    {
        fs::remove(entries[i].path);
    }
}
std::filesystem::path Updater::DownloadUpdate(const std::string &url)
{
    namespace fs = std::filesystem;
    std::string filename = GetUpdateFilename(url);
    if (filename.empty())
    {
        throw std::runtime_error("Permission denied. Invalid url.");
    }    
    auto downloadDirectory = WORKING_DIRECTORY / "downloads";
    std::filesystem::create_directories(downloadDirectory);

    auto downloadPath = downloadDirectory / filename;

    try {
        fs::remove(downloadPath);
        std::string args = SS("-s -L " << url << " -o " << downloadPath << " 2>&1");
        auto curlOutput = sysExecForOutput("curl", args);
        if (curlOutput.exitCode != EXIT_SUCCESS)
        {
            Lv2Log::error(SS("Update download failed." << unCRLF(curlOutput.output)));
            throw std::runtime_error("PiPedal server does not have access to the internet.");
        }
        if (!fs::exists(downloadPath) || fs::file_size(downloadPath) == 0)
        {
            throw std::runtime_error("Download failed.");
        }
        try {
            removeOldSiblings(2, downloadPath);
        } catch (const std::exception&e)
        {
            Lv2Log::error(SS("Can't remove download siblings" << e.what()));
            // and carry on.
        }
        return downloadPath;
    } catch (const std::exception &e)
    {
        std::filesystem::remove(downloadPath);
        throw;
    }

}

///////////////////////////////////////////////////////////////////////////////////////////////

JSON_MAP_BEGIN(UpdateRelease)
JSON_MAP_REFERENCE(UpdateRelease, updateAvailable)
JSON_MAP_REFERENCE(UpdateRelease, upgradeVersion)
JSON_MAP_REFERENCE(UpdateRelease, upgradeVersionDisplayName)
JSON_MAP_REFERENCE(UpdateRelease, assetName)
JSON_MAP_REFERENCE(UpdateRelease, updateUrl)
JSON_MAP_END();

JSON_MAP_BEGIN(UpdateStatus)
JSON_MAP_REFERENCE(UpdateStatus, lastUpdateTime)
JSON_MAP_REFERENCE(UpdateStatus, isValid)
JSON_MAP_REFERENCE(UpdateStatus, errorMessage)
JSON_MAP_REFERENCE(UpdateStatus, isOnline)
JSON_MAP_REFERENCE(UpdateStatus, currentVersion)
JSON_MAP_REFERENCE(UpdateStatus, currentVersionDisplayName)
JSON_MAP_REFERENCE(UpdateStatus, updatePolicy)
JSON_MAP_REFERENCE(UpdateStatus, releaseOnlyRelease)
JSON_MAP_REFERENCE(UpdateStatus, releaseOrBetaRelease)
JSON_MAP_REFERENCE(UpdateStatus, devRelease)
JSON_MAP_END();
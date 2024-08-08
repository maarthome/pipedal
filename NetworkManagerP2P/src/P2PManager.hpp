#pragma once

#include <memory>



class P2PManager {
protected:
    P2PManager() { }
public:
    using self=P2PManager;
    using ptr = std::unique_ptr<self>;

    static ptr Create();

    virtual ~P2PManager() {}

    virtual bool Init() = 0;
    virtual void Run() = 0;
    virtual void Stop() = 0;
    virtual bool IsFinished() = 0;
    virtual void Wait() = 0;
    virtual bool ReloadRequested() = 0;
    virtual bool TerminatedNormally() = 0;
};

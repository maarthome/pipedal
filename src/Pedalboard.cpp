// Copyright (c) 2022 Robin Davies
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

#include "pch.h"
#include "Pedalboard.hpp"
#include "AtomConverter.hpp"


using namespace pipedal;


static const PedalboardItem* GetItem_(const std::vector<PedalboardItem>&items,int64_t pedalboardItemId)
{
    for (size_t i = 0; i < items.size(); ++i)
    {
        auto &item = items[i];
        if (items[i].instanceId() == pedalboardItemId)
        {
            return &(items[i]);
        }
        if (item.isSplit())
        {
            const PedalboardItem* t = GetItem_(item.topChain(),pedalboardItemId);
            if (t != nullptr) return t;
            t = GetItem_(item.bottomChain(),pedalboardItemId);
            if (t != nullptr) return t;
        }
    }
    return nullptr;
}

static void GetAllItems(std::vector<PedalboardItem*> & result, std::vector<PedalboardItem>&items)
{
    for (auto& item: items)
    {
        if (item.isSplit())
        {
            GetAllItems(result,item.topChain());
            GetAllItems(result,item.bottomChain());
        }
        result.push_back(&item);
    }    
}
std::vector<PedalboardItem*> Pedalboard::GetAllPlugins()
{
    std::vector<PedalboardItem*> result;
    GetAllItems(result,this->items());
    return result;
}


const PedalboardItem*Pedalboard::GetItem(int64_t pedalItemId) const
{
    return GetItem_(this->items(),pedalItemId);
}
PedalboardItem*Pedalboard::GetItem(int64_t pedalItemId)
 {
     return const_cast<PedalboardItem*>(GetItem_(this->items(),pedalItemId));
 }


ControlValue* PedalboardItem::GetControlValue(const std::string&symbol)
{
    for (size_t i = 0; i < this->controlValues().size(); ++i)
    {
        if (this->controlValues()[i].key() == symbol)
        {
            return &(this->controlValues()[i]);
        }
    }
    return nullptr;
}

bool Pedalboard::SetItemEnabled(int64_t pedalItemId, bool enabled)
{
    PedalboardItem*item = GetItem(pedalItemId);
    if (!item) return false;
    if (item->isEnabled() != enabled)
    {
        item->isEnabled(enabled);
        return true;
    }
    return false;

}


bool Pedalboard::SetControlValue(int64_t pedalItemId, const std::string &symbol, float value)
{
    PedalboardItem*item = GetItem(pedalItemId);
    if (!item) return false;
    ControlValue*controlValue = item->GetControlValue(symbol);
    if (controlValue == nullptr) return false;
    if (controlValue->value() != value)
    {
        controlValue->value(value);
        return true;
    }
    return false;
}


PedalboardItem Pedalboard::MakeEmptyItem()
{
    uint64_t instanceId = NextInstanceId();

    PedalboardItem result;
    result.instanceId(instanceId);
    result.uri(EMPTY_PEDALBOARD_ITEM_URI);
    result.pluginName("");
    result.isEnabled(true);
    return result;
}


PedalboardItem Pedalboard::MakeSplit()
{
    uint64_t instanceId = NextInstanceId();

    PedalboardItem result;
    result.instanceId(instanceId);
    result.uri(SPLIT_PEDALBOARD_ITEM_URI);
    result.pluginName("");
    result.isEnabled(true);

    result.topChain().push_back(MakeEmptyItem());
    result.bottomChain().push_back(MakeEmptyItem());
    result.controlValues().push_back(
        ControlValue(SPLIT_SPLITTYPE_KEY,0));
    result.controlValues().push_back(
        ControlValue(SPLIT_SELECT_KEY,0));
    result.controlValues().push_back(
        ControlValue(SPLIT_MIX_KEY,0));
    result.controlValues().push_back(
        ControlValue(SPLIT_PANL_KEY,0));
    result.controlValues().push_back(
        ControlValue(SPLIT_VOLL_KEY,-3));
    result.controlValues().push_back(
        ControlValue(SPLIT_PANR_KEY,0));
    result.controlValues().push_back(
        ControlValue(SPLIT_VOLR_KEY,-3));
    
    return result;
}



Pedalboard Pedalboard::MakeDefault()
{
    // copy insanity. but it happens so rarely.
    Pedalboard result;

    result.items().push_back(result.MakeEmptyItem());
    result.name("Default Preset");

    return result;
}


bool IsPedalboardSplitItem(const PedalboardItem*self, const std::vector<PedalboardItem>&value)
{
    return self->uri() == SPLIT_PEDALBOARD_ITEM_URI;
}

bool Pedalboard::ApplySnapshot(int64_t snapshotIndex)
{
    if (snapshotIndex < 0 || 
        snapshotIndex >= this->snapshots_.size() || 
        this->snapshots_[snapshotIndex] == nullptr ||
        this->selectedSnapshot() == snapshotIndex)
    {
        return false;
    }
    std::map<int64_t, SnapshotValue*> indexedValues;
    Snapshot *snapshot = this->snapshots_[snapshotIndex].get();

    for (auto &value: snapshot->values_)
    {
        indexedValues[value.instanceId_] = &value;
    }

    auto plugins = this->GetAllPlugins();
    for (PedalboardItem *pedalboardItem: plugins)
    {
        SnapshotValue*snapshotValue = indexedValues[pedalboardItem->instanceId()];
        if (snapshotValue)
        {
            pedalboardItem->ApplySnapshotValue(snapshotValue);
        }
    }
    return true;
}

void PedalboardItem::ApplySnapshotValue(SnapshotValue*snapshotValue)
{
    std::map<std::string,float> cumulativeValues;
    for (auto &controlValue: this->controlValues())
    {
        cumulativeValues[controlValue.key()] = controlValue.value();
    }
    for (auto&controlValue : snapshotValue->controlValues_)
    {
        cumulativeValues[controlValue.key()] = controlValue.value();
    }
    this->controlValues().clear();
    for (auto&pair: cumulativeValues)
    {
        this->controlValues_.push_back(ControlValue(pair.first.c_str(),pair.second));
    }
    if (this->lv2State() != snapshotValue->lv2State_)
    {
        this->lv2State(snapshotValue->lv2State_);
        this->stateUpdateCount(this->stateUpdateCount()+1);
    }

}


// can we just send a snapshot-style uddate instead of reloading plugins? All settings are ignored.
bool Pedalboard::IsStructureIdentical(const Pedalboard &other) const
{
    if (this->nextInstanceId_ != other.nextInstanceId_) // quick check that catches 95% of structural changes.
    {
        return false;
    }
    if (this->items_.size() != other.items_.size()) 
    {
        return false;
    }
    for (size_t i = 0; i < this->items_.size();++i) 
    {
        if (!this->items_[i].IsStructurallyIdentical(other.items_[i]))
        {
            return false;
        }
    }
    return true;
}

bool PedalboardItem::IsStructurallyIdentical(const PedalboardItem&other) const
{
    if (this->instanceId() != other.instanceId()) // must match in order to ensure that realtime message passing works.
    {
        return false;
    }
    if (this->uri() != other.uri())
    {
        return false;
    }
    if (this->isSplit()) // so is the other by virtue of idential uris.
    {
        if (topChain().size() != other.topChain().size())
        {
            return false;
        }
        for (size_t i = 0; i < topChain().size(); ++i)
        {
            if (!topChain()[i].IsStructurallyIdentical(other.topChain()[i] ))
            {
                return false;
            }
        }
        if (bottomChain().size() != other.bottomChain().size())
        {
            for (size_t i = 0; i < bottomChain().size(); ++i)
            {
                if (!bottomChain()[i].IsStructurallyIdentical(other.bottomChain()[i]))
                {
                    return false;
                }
            }
        }
    }
    return true;
}

void PedalboardItem::AddToSnapshotFromCurrentSettings(Snapshot&snapshot) const
{
    SnapshotValue snapshotValue;
    snapshotValue.instanceId_ = this->instanceId_;

    for (const ControlValue &value: this->controlValues_)
    {
        snapshotValue.controlValues_.push_back(value);
    }
    for (const auto&pathProperty: this->pathProperties_)
    {
        snapshotValue.pathProperties_[pathProperty.first] = pathProperty.second;
    }
    snapshotValue.lv2State_ = this->lv2State_;
    snapshot.values_.push_back(std::move(snapshotValue));

    if (this->isSplit())
    {
        for (auto&item: this->topChain_)
        {
            item.AddToSnapshotFromCurrentSettings(snapshot);;
        }
        for (auto&item: this->bottomChain_)
        {
            item.AddToSnapshotFromCurrentSettings(snapshot);
        }
    }
}

void PedalboardItem::AddResetsForMissingProperties(Snapshot&snapshot, size_t*index) const
{
    // structure must be identical
    // items must be enumerated in the same order as AddToSnapshotFromCurrentSettings
    SnapshotValue&snapshotValue = snapshot.values_[*index];
    
    if (snapshotValue.instanceId_ != this->instanceId())
    {
        throw std::runtime_error("Pedalboard structure does not match.");
    }

    for (auto&property: this->pathProperties_)
    {
        auto f = snapshotValue.pathProperties_.find(property.first);
        if (f == snapshotValue.pathProperties_.end())
        {
            snapshotValue.pathProperties_[property.first] = AtomConverter::EmptyPathstring();
        }
    }   
    ++(*index);

    if (this->isSplit())
    {
        for (auto&item: this->topChain())
        {
            item.AddResetsForMissingProperties(snapshot,index);
        }
        for (auto&item: this->bottomChain())
        {
            item.AddResetsForMissingProperties(snapshot,index);
        }
    }

}

Snapshot Pedalboard::MakeSnapshotFromCurrentSettings(const Pedalboard &previousPedalboard)
{
    Snapshot snapshot;
    // name and color don't matter. this is strictly for loading purposes.
    for (auto &item : this->items())
    {
        item.AddToSnapshotFromCurrentSettings(snapshot);
    }
    // a neccesary precondition: the previous pedalboard must have identical structure, 
    // so we can just 
    size_t index = 0;
    for (auto&item: previousPedalboard.items_)
    {
        item.AddResetsForMissingProperties(snapshot,&index);
    }
    return snapshot;

}



JSON_MAP_BEGIN(ControlValue)
    JSON_MAP_REFERENCE(ControlValue,key)
    JSON_MAP_REFERENCE(ControlValue,value)
JSON_MAP_END()



JSON_MAP_BEGIN(PedalboardItem)
    JSON_MAP_REFERENCE(PedalboardItem,instanceId)
    JSON_MAP_REFERENCE(PedalboardItem,uri)
    JSON_MAP_REFERENCE(PedalboardItem,isEnabled)
    JSON_MAP_REFERENCE(PedalboardItem,controlValues)
    JSON_MAP_REFERENCE(PedalboardItem,pluginName)
    JSON_MAP_REFERENCE_CONDITIONAL(PedalboardItem,topChain,IsPedalboardSplitItem)
    JSON_MAP_REFERENCE_CONDITIONAL(PedalboardItem,bottomChain,&IsPedalboardSplitItem)
    JSON_MAP_REFERENCE(PedalboardItem,midiBindings)
    JSON_MAP_REFERENCE(PedalboardItem,stateUpdateCount)
    JSON_MAP_REFERENCE(PedalboardItem,lv2State)
    JSON_MAP_REFERENCE(PedalboardItem,lilvPresetUri)
    JSON_MAP_REFERENCE(PedalboardItem,pathProperties)
JSON_MAP_END()


JSON_MAP_BEGIN(Pedalboard)
    JSON_MAP_REFERENCE(Pedalboard,name)
    JSON_MAP_REFERENCE(Pedalboard,input_volume_db)
    JSON_MAP_REFERENCE(Pedalboard,output_volume_db)
    JSON_MAP_REFERENCE(Pedalboard,items)
    JSON_MAP_REFERENCE(Pedalboard,nextInstanceId)
    JSON_MAP_REFERENCE(Pedalboard,snapshots)
    JSON_MAP_REFERENCE(Pedalboard,selectedSnapshot)
JSON_MAP_END()

JSON_MAP_BEGIN(SnapshotValue)
    JSON_MAP_REFERENCE(SnapshotValue,instanceId)
    JSON_MAP_REFERENCE(SnapshotValue,controlValues)
    JSON_MAP_REFERENCE(SnapshotValue,lv2State)
    JSON_MAP_REFERENCE(SnapshotValue,pathProperties)
JSON_MAP_END()

JSON_MAP_BEGIN(Snapshot)
    JSON_MAP_REFERENCE(Snapshot,name)
    JSON_MAP_REFERENCE(Snapshot,color)
    JSON_MAP_REFERENCE(Snapshot,values)
JSON_MAP_END()



// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "UEOSC.h"
#include "Modules/ModuleManager.h"

class FUEOSCModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
    }
    virtual void ShutdownModule() override
    {
    }
    virtual bool IsGameModule() const override
    {
        return true;
    }
};

IMPLEMENT_GAME_MODULE(FUEOSCModule, UEOSC);

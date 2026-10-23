#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FBlueprintWorldBridgeModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void InstallRedirectsForWorld(class UWorld* World);
    void RemoveOwnedRedirects();

    FDelegateHandle BlueprintSubsystemBeginPlayHook;
    TMap<FString, FString> OwnedRedirects;
};

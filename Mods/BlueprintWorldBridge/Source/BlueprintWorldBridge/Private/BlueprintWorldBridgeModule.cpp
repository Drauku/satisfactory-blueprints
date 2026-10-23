#include "BlueprintWorldBridgeModule.h"

#include "BlueprintWorldNames.h"
#include "Engine/World.h"
#include "FGBlueprintSubsystem.h"
#include "FGSaveSystem.h"
#include "Patching/NativeHookManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogBlueprintWorldBridge, Log, All);

namespace
{
FString ToUnrealString(const std::string_view Value)
{
    const FUTF8ToTCHAR Converted(
        reinterpret_cast<const UTF8CHAR*>(Value.data()),
        static_cast<int32>(Value.size()));
    return FString(Converted.Length(), Converted.Get());
}
}

void FBlueprintWorldBridgeModule::StartupModule()
{
    // Native hooks are unsafe in editor worlds. The packaged game and dedicated server both
    // run this path, so blueprint references are resolved consistently for every participant.
    if (!WITH_EDITOR)
    {
        BlueprintSubsystemBeginPlayHook = SUBSCRIBE_UOBJECT_METHOD_AFTER(
            AFGBlueprintSubsystem,
            BeginPlay,
            [this](AFGBlueprintSubsystem* BlueprintSubsystem)
            {
                if (IsValid(BlueprintSubsystem))
                {
                    InstallRedirectsForWorld(BlueprintSubsystem->GetWorld());
                }
            });
    }
}

void FBlueprintWorldBridgeModule::ShutdownModule()
{
    if (BlueprintSubsystemBeginPlayHook.IsValid())
    {
        UNSUBSCRIBE_UOBJECT_METHOD(
            AFGBlueprintSubsystem,
            BeginPlay,
            BlueprintSubsystemBeginPlayHook);
        BlueprintSubsystemBeginPlayHook.Reset();
    }

    RemoveOwnedRedirects();
}

void FBlueprintWorldBridgeModule::InstallRedirectsForWorld(UWorld* World)
{
    if (!IsValid(World))
    {
        UE_LOG(LogBlueprintWorldBridge, Warning, TEXT("Cannot install blueprint redirects: world is invalid."));
        return;
    }

    const FString SanitizedMapName = UFGSaveSystem::SanitizeMapName(World->GetMapName());
    const auto TargetUtf8 = StringCast<UTF8CHAR>(*SanitizedMapName);
    const std::string Target = BlueprintWorldBridge::CanonicalWorldName(
        std::string_view(reinterpret_cast<const char*>(TargetUtf8.Get()), TargetUtf8.Length()));

    if (Target.empty())
    {
        UE_LOG(LogBlueprintWorldBridge, Warning,
            TEXT("Cannot install blueprint redirects: map name '%s' normalized to an empty name."),
            *World->GetMapName());
        return;
    }

    RemoveOwnedRedirects();

    int32 InstalledCount = 0;
    const FString TargetName = ToUnrealString(Target);
    for (const std::string_view Source : BlueprintWorldBridge::KnownBlueprintWorldNames)
    {
        const FString SourceName = ToUnrealString(Source);
        if (SourceName.Equals(TargetName, ESearchCase::CaseSensitive))
        {
            continue;
        }

        const FMapRedirector* Existing = UFGSaveSystem::mBlueprintMapRedirectors.FindByPredicate(
            [&SourceName](const FMapRedirector& Redirect)
            {
                return Redirect.OldMapName.Equals(SourceName, ESearchCase::CaseSensitive);
            });
        if (Existing != nullptr)
        {
            UE_LOG(LogBlueprintWorldBridge, Warning,
                TEXT("Leaving existing blueprint map redirect '%s' -> '%s' unchanged; it may be owned by FactoryGame or another mod."),
                *Existing->OldMapName,
                *Existing->NewMapName);
            continue;
        }

        if (UFGSaveSystem::AddBlueprintMapRedirector(SourceName, TargetName))
        {
            OwnedRedirects.Add(SourceName, TargetName);
            ++InstalledCount;
        }
        else
        {
            UE_LOG(LogBlueprintWorldBridge, Warning,
                TEXT("FactoryGame rejected blueprint map redirect '%s' -> '%s'."),
                *SourceName,
                *TargetName);
        }
    }

    UE_LOG(LogBlueprintWorldBridge, Display,
        TEXT("Installed %d blueprint world redirect(s) for '%s'. Blueprint files remain unchanged."),
        InstalledCount,
        *TargetName);
}

void FBlueprintWorldBridgeModule::RemoveOwnedRedirects()
{
    for (const TPair<FString, FString>& Owned : OwnedRedirects)
    {
        // Do not remove a redirect if another system replaced ours after installation.
        const bool StillOwned = UFGSaveSystem::mBlueprintMapRedirectors.ContainsByPredicate(
            [&Owned](const FMapRedirector& Redirect)
            {
                return Redirect.OldMapName.Equals(Owned.Key, ESearchCase::CaseSensitive)
                    && Redirect.NewMapName.Equals(Owned.Value, ESearchCase::CaseSensitive);
            });
        if (StillOwned)
        {
            UFGSaveSystem::RemoveBlueprintMapRedirector(Owned.Key);
        }
    }

    OwnedRedirects.Empty();
}

IMPLEMENT_MODULE(FBlueprintWorldBridgeModule, BlueprintWorldBridge)

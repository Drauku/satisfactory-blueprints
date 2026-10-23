#include "BlueprintWorldBridgeModule.h"

#include "BlueprintWorldNames.h"
#include "FGSaveSystem.h"
#include "Misc/ScopeLock.h"
#include "Patching/NativeHookManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogBlueprintWorldBridge, Log, All);

namespace
{
std::string ToUtf8(const FString& Value)
{
    const auto Converted = StringCast<UTF8CHAR>(*Value);
    return std::string(reinterpret_cast<const char*>(Converted.Get()), Converted.Length());
}

FString ToUnrealString(const std::string_view Value)
{
    const FUTF8ToTCHAR Converted(
        reinterpret_cast<const UTF8CHAR*>(Value.data()),
        static_cast<int32>(Value.size()));
    return FString(Converted.Length(), Converted.Get());
}

FString CanonicalWorldName(const FString& RawName)
{
    return ToUnrealString(BlueprintWorldBridge::CanonicalWorldName(ToUtf8(RawName)));
}
}

void FBlueprintWorldBridgeModule::StartupModule()
{
    // Native hooks are unsafe in editor worlds. The packaged game and dedicated server both
    // run this path, so blueprint references are resolved consistently for every participant.
    if (WITH_EDITOR)
    {
        return;
    }

    // Post-initialization runs before any actor's BeginPlay, so the current world is known
    // before AFGBlueprintSubsystem can load a blueprint.
    PostWorldInitializationHandle = FWorldDelegates::OnPostWorldInitialization.AddRaw(
        this, &FBlueprintWorldBridgeModule::HandlePostWorldInitialization);
    WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddRaw(
        this, &FBlueprintWorldBridgeModule::HandleWorldCleanup);

    // FactoryGame asks this function to translate the world name stored in a blueprint
    // whenever it loads one. Handling it directly means any world name works, with no list.
    FindBlueprintWorldHook = SUBSCRIBE_METHOD(
        UFGSaveSystem::FindNewBlueprintWorldMapName,
        [this](TCallScope<bool(*)(const FString&, FString&)>& Scope, const FString& OldMapName, FString& OutNewMapName)
        {
            // Run FactoryGame and any other mod's handlers first; an existing redirect wins.
            if (Scope(OldMapName, OutNewMapName))
            {
                return;
            }
            if (ResolveStoredWorld(OldMapName, OutNewMapName))
            {
                Scope.Override(true);
            }
        });
}

void FBlueprintWorldBridgeModule::ShutdownModule()
{
    if (FindBlueprintWorldHook.IsValid())
    {
        UNSUBSCRIBE_METHOD(UFGSaveSystem::FindNewBlueprintWorldMapName, FindBlueprintWorldHook);
        FindBlueprintWorldHook.Reset();
    }

    FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitializationHandle);
    FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
    PostWorldInitializationHandle.Reset();
    WorldCleanupHandle.Reset();
}

void FBlueprintWorldBridgeModule::HandlePostWorldInitialization(
    UWorld* World,
    const UWorld::InitializationValues Values)
{
    if (!IsValid(World) || !World->IsGameWorld())
    {
        return;
    }

    // Worlds FactoryGame creates at runtime (for example to stage blueprints) live in transient
    // packages and must not replace the save's real world.
    const FString PackageName = World->GetOutermost()->GetName();
    if (PackageName.StartsWith(TEXT("/Temp/")))
    {
        UE_LOG(LogBlueprintWorldBridge, Verbose, TEXT("Ignoring transient world '%s'."), *PackageName);
        return;
    }

    const FString WorldName = CanonicalWorldName(UFGSaveSystem::SanitizeMapName(World->GetMapName()));
    if (WorldName.IsEmpty())
    {
        UE_LOG(LogBlueprintWorldBridge, Warning,
            TEXT("Map name '%s' normalized to an empty name; blueprints will not be redirected in this world."),
            *World->GetMapName());
        return;
    }

    {
        FScopeLock Lock(&CurrentWorldLock);
        CurrentWorld = World;
        CurrentWorldName = WorldName;
        CurrentPackageName = PackageName;
        LoggedStoredWorlds.Reset();
        bCheckedStockWorld = false;
    }

    UE_LOG(LogBlueprintWorldBridge, Display,
        TEXT("Current world is '%s' (%s). Blueprints from other worlds will be redirected here; files are not modified."),
        *WorldName,
        *PackageName);
}

void FBlueprintWorldBridgeModule::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
    FScopeLock Lock(&CurrentWorldLock);
    if (CurrentWorld.Get() == World)
    {
        CurrentWorld.Reset();
        CurrentWorldName.Reset();
        CurrentPackageName.Reset();
        LoggedStoredWorlds.Reset();
        bCheckedStockWorld = false;
    }
}

bool FBlueprintWorldBridgeModule::ResolveStoredWorld(const FString& StoredWorld, FString& OutWorld)
{
    FString TargetWorld;
    FString TargetPackage;
    bool bCheckStockWorld = false;
    bool bFirstSighting = false;
    {
        FScopeLock Lock(&CurrentWorldLock);
        TargetWorld = CurrentWorldName;
        TargetPackage = CurrentPackageName;
        bCheckStockWorld = !bCheckedStockWorld && !TargetWorld.IsEmpty();
        bCheckedStockWorld |= bCheckStockWorld;

        bool bAlreadyLogged = false;
        LoggedStoredWorlds.Add(StoredWorld, &bAlreadyLogged);
        bFirstSighting = !bAlreadyLogged;
    }

    // Checked on the first blueprint load rather than at world start, so menu worlds are ignored.
    if (bCheckStockWorld)
    {
        CheckStockWorldName(TargetWorld, TargetPackage);
    }

    if (!BlueprintWorldBridge::NeedsWorldRedirect(ToUtf8(StoredWorld), ToUtf8(TargetWorld)))
    {
        return false;
    }

    if (bFirstSighting)
    {
        FString RenamedWorld;
        if (UFGSaveSystem::FindNewMapName(StoredWorld, RenamedWorld))
        {
            UE_LOG(LogBlueprintWorldBridge, Display,
                TEXT("The base game renamed world '%s' to '%s'."),
                *StoredWorld,
                *RenamedWorld);
        }

        UE_LOG(LogBlueprintWorldBridge, Display,
            TEXT("Redirecting blueprints from world '%s' to '%s'."),
            *StoredWorld,
            *TargetWorld);
    }

    OutWorld = TargetWorld;
    return true;
}

void FBlueprintWorldBridgeModule::CheckStockWorldName(const FString& WorldName, const FString& PackageName) const
{
    const FString StockWorld = ToUnrealString(BlueprintWorldBridge::StockWorldName);

    // The base game keeps its own table of renamed maps. An entry for the standard name means a
    // game update replaced it.
    FString RenamedStockWorld;
    if (UFGSaveSystem::FindNewMapName(StockWorld, RenamedStockWorld)
        && !RenamedStockWorld.Equals(StockWorld, ESearchCase::IgnoreCase))
    {
        UE_LOG(LogBlueprintWorldBridge, Warning,
            TEXT("The base game now renames its standard world '%s' to '%s'. Redirects still work; ")
            TEXT("update StockWorldName in BlueprintWorldNames.h to silence this warning."),
            *StockWorld,
            *RenamedStockWorld);
    }

    if (BlueprintWorldBridge::StockWorldWasRenamed(ToUtf8(PackageName), ToUtf8(WorldName)))
    {
        UE_LOG(LogBlueprintWorldBridge, Warning,
            TEXT("Base-game map '%s' uses world name '%s' instead of '%s'. Redirects still work; ")
            TEXT("update StockWorldName in BlueprintWorldNames.h to silence this warning."),
            *PackageName,
            *WorldName,
            *StockWorld);
    }
}

IMPLEMENT_MODULE(FBlueprintWorldBridgeModule, BlueprintWorldBridge)

#pragma once

#include <string>
#include <string_view>

namespace BlueprintWorldBridge
{
// The world name FactoryGame used for its stock map when this mod was written. It is only
// a canary: if a stock map loads under another name, the base game has renamed its standard
// world and the log says so. Redirects never depend on this value.
inline constexpr std::string_view StockWorldName = "Persistent_Level";

// Stock maps are cooked under the game's own content root; mod maps live under their plugin's
// mount point (for example /PEP_Mod/...).
inline constexpr std::string_view StockMapPackageRoot = "/Game/FactoryGame/";

// UWorld::GetMapName normally returns a leaf name, but PIE and commandlet builds can add
// prefixes and callers may supply a package/object path. Keep this helper Unreal-free so its
// edge cases can be tested without the proprietary Satisfactory toolchain.
inline std::string CanonicalWorldName(std::string_view RawName)
{
    const auto LastSlash = RawName.find_last_of("/\\");
    if (LastSlash != std::string_view::npos)
    {
        RawName.remove_prefix(LastSlash + 1);
    }

    const auto LastDot = RawName.find_last_of('.');
    if (LastDot != std::string_view::npos)
    {
        RawName.remove_prefix(LastDot + 1);
    }

    constexpr std::string_view PiePrefix = "UEDPIE_";
    if (RawName.starts_with(PiePrefix))
    {
        auto PrefixEnd = PiePrefix.size();
        while (PrefixEnd < RawName.size() && RawName[PrefixEnd] >= '0' && RawName[PrefixEnd] <= '9')
        {
            ++PrefixEnd;
        }
        if (PrefixEnd < RawName.size() && RawName[PrefixEnd] == '_')
        {
            RawName.remove_prefix(PrefixEnd + 1);
        }
    }

    constexpr std::string_view DedicatedPrefix = "UEDPC_";
    if (RawName.starts_with(DedicatedPrefix))
    {
        RawName.remove_prefix(DedicatedPrefix.size());
    }

    return std::string(RawName);
}

// Unreal package and FName comparisons ignore case, so world names must as well.
inline bool SameWorldName(std::string_view Left, std::string_view Right)
{
    if (Left.size() != Right.size())
    {
        return false;
    }
    for (std::size_t Index = 0; Index < Left.size(); ++Index)
    {
        const auto Lower = [](char Value)
        {
            return Value >= 'A' && Value <= 'Z' ? static_cast<char>(Value - 'A' + 'a') : Value;
        };
        if (Lower(Left[Index]) != Lower(Right[Index]))
        {
            return false;
        }
    }
    return true;
}

// A blueprint recorded in one world needs redirecting when it is loaded into a different one.
// Unknown or empty names are left to FactoryGame rather than guessed at.
inline bool NeedsWorldRedirect(std::string_view StoredWorld, std::string_view CurrentWorld)
{
    const std::string Stored = CanonicalWorldName(StoredWorld);
    const std::string Current = CanonicalWorldName(CurrentWorld);
    return !Stored.empty() && !Current.empty() && !SameWorldName(Stored, Current);
}

inline bool IsStockMapPackage(std::string_view PackageName)
{
    return PackageName.size() >= StockMapPackageRoot.size()
        && SameWorldName(PackageName.substr(0, StockMapPackageRoot.size()), StockMapPackageRoot);
}

// True when a stock map no longer uses the name this mod was written against.
inline bool StockWorldWasRenamed(std::string_view PackageName, std::string_view WorldName)
{
    return IsStockMapPackage(PackageName) && !SameWorldName(CanonicalWorldName(WorldName), StockWorldName);
}
} // namespace BlueprintWorldBridge

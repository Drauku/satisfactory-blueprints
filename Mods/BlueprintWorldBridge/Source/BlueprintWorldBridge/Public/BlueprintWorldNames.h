#pragma once

#include <array>
#include <string>
#include <string_view>

namespace BlueprintWorldBridge
{
inline constexpr std::array<std::string_view, 3> KnownBlueprintWorldNames{
    "Persistent_Level",
    "SkyblockLevel",
    "ProceduralWorldLevel",
};

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
} // namespace BlueprintWorldBridge

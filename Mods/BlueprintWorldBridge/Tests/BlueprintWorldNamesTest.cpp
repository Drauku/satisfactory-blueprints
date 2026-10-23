#include "BlueprintWorldNames.h"

#include <cassert>
#include <iostream>

using BlueprintWorldBridge::CanonicalWorldName;
using BlueprintWorldBridge::NeedsWorldRedirect;
using BlueprintWorldBridge::SameWorldName;
using BlueprintWorldBridge::StockWorldWasRenamed;

int main()
{
    assert(CanonicalWorldName("Persistent_Level") == "Persistent_Level");
    assert(CanonicalWorldName("SkyblockLevel") == "SkyblockLevel");
    assert(CanonicalWorldName("ProceduralWorldLevel") == "ProceduralWorldLevel");
    assert(CanonicalWorldName("/Game/FactoryGame/Map/GameLevel01/Persistent_Level") == "Persistent_Level");
    assert(CanonicalWorldName("/Game/Mods/PEP/SkyblockLevel.SkyblockLevel") == "SkyblockLevel");
    assert(CanonicalWorldName("UEDPIE_0_Persistent_Level") == "Persistent_Level");
    assert(CanonicalWorldName("UEDPIE_12_ProceduralWorldLevel") == "ProceduralWorldLevel");
    assert(CanonicalWorldName("UEDPC_SkyblockLevel") == "SkyblockLevel");
    assert(CanonicalWorldName("").empty());

    assert(SameWorldName("Persistent_Level", "persistent_level"));
    assert(!SameWorldName("Persistent_Level", "Persistent_Level2"));

    // Any world name works, including ones this mod has never heard of.
    assert(NeedsWorldRedirect("Persistent_Level", "SkyblockLevel"));
    assert(NeedsWorldRedirect("ProceduralWorldLevel", "Persistent_Level"));
    assert(NeedsWorldRedirect("HexWorld", "1_Snowy"));
    assert(NeedsWorldRedirect("SatifactoryPlusWorld", "UEDPIE_0_Persistent_Level"));
    assert(!NeedsWorldRedirect("Persistent_Level", "Persistent_Level"));
    assert(!NeedsWorldRedirect("/Game/Mods/PEP/SkyblockLevel.SkyblockLevel", "skyblocklevel"));
    assert(!NeedsWorldRedirect("", "Persistent_Level"));
    assert(!NeedsWorldRedirect("Persistent_Level", ""));

    assert(!StockWorldWasRenamed("/Game/FactoryGame/Map/GameLevel01/Persistent_Level", "Persistent_Level"));
    assert(StockWorldWasRenamed("/Game/FactoryGame/Map/GameLevel02/Persistent_Level2", "Persistent_Level2"));
    assert(!StockWorldWasRenamed("/PEP_Mod/Maps/SkyblockLevel", "SkyblockLevel"));
    assert(!StockWorldWasRenamed("/BlankMap/BlankLevel2", "BlankLevel2"));

    std::cout << "BlueprintWorldNames tests passed\n";
}

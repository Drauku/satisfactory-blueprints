#include "BlueprintWorldNames.h"

#include <cassert>
#include <iostream>

using BlueprintWorldBridge::CanonicalWorldName;

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

    std::cout << "BlueprintWorldNames tests passed\n";
}

#include "yrpp/MouseClass.h"
namespace game { namespace {
MouseClass root;
LayerClass layers[5];
int tile_variant_table[64]{};
bool tile_variant_initialized=false;
} }
GScreenClass& GScreenClass::Instance = game::root;
MapClass& MapClass::Instance = game::root;
DisplayClass& DisplayClass::Instance = game::root;
RadarClass& RadarClass::Instance = game::root;
PowerClass& PowerClass::Instance = game::root;
SidebarClass& SidebarClass::Instance = game::root;
TabClass& TabClass::Instance = game::root;
ScrollClass& ScrollClass::Instance = game::root;
MouseClass& MouseClass::Instance = game::root;
LayerClass (&MapClass::ObjectsInLayers)[5] = game::layers;
CellClass& MapClass::InvalidCell = []() -> CellClass& { static CellClass cell; return cell; }();
int (&CellClass::TileVariantTable)[64] = game::tile_variant_table;
bool& CellClass::TileVariantTableInitialized = game::tile_variant_initialized;

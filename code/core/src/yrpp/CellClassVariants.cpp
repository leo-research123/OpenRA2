// Existing YRpp Cell/Tile/Randomizer objects; YR 4814F0 and tables 81CCA8,
// 81CCE8 calibrate the variant selection missing from the pinned declaration.
#include "yrpp/CellClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/Randomizer.h"
#include <algorithm>
#include <bit>

int CellClass::GetTileVariant(int tile_index,int count) const noexcept {
    if (!TileVariantTableInitialized) {
        TileVariantTableInitialized=true;
        std::fill_n(TileVariantTable,64,-1);
        constexpr int offsets[]{-9,-8,-7,-1,1,7,8,9};
        for (int i=0;i<64;++i) {
            int candidate=0;
            for (int attempt=0;attempt<64;++attempt) {
                candidate=Randomizer::Global.Random()&7;
                bool collision=false;
                for (int delta : offsets) {
                    // Original wraps the linear index, including across row edges.
                    if (TileVariantTable[(i+delta+64)%64]==candidate) { collision=true; break; }
                }
                if (!collision) break;
            }
            TileVariantTable[i]=candidate;
        }
    }
    int x=MapCoords.X,y=MapCoords.Y;
    if (Height) {
        if (tile_index<0 || tile_index>=IsometricTileTypeClass::Array.Count) return 0;
        try {
            const auto* tmp=reinterpret_cast<const TMPStruct*>(IsometricTileTypeClass::Array[tile_index]->GetImage());
            if (!tmp || tmp->Columns<=0 || tmp->Rows<=0) return 0;
            // 4815D9/4815F4 subtract the subtile's column before division.
            x=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(x-static_cast<unsigned char>(Height)%tmp->Columns))/tmp->Columns;
            y=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(y-static_cast<unsigned char>(Height)/tmp->Columns))/tmp->Rows;
        } catch (...) { return 0; }
    }
    if (count>4) return TileVariantTable[(y&7)*8+(x&7)];
    static constexpr int pattern[]{0,1,2,3,3,2,1,0,2,3,0,1,1,0,3,2};
    return pattern[(y&3)*4+(x&3)];
}

bool CellClass::GetTerrainTile(IsometricTileTypeClass*& output,int& variant) const noexcept {
    output=nullptr; variant=0;
    using Tile=IsometricTileTypeClass;
    const bool clear=IsoTileTypeIndex<0 || IsoTileTypeIndex==0xffff || IsoTileTypeIndex>=Tile::Array.Count;
    const int index=clear ? Tile::ClearTile : IsoTileTypeIndex;
    if (index<0 || index>=Tile::Array.Count) return false;
    auto* tile=Tile::Array[index];
    if (!tile || tile->unk_2F0<=0) return false;
    try {
        if (clear) variant=GetTileVariant(index,tile->unk_2F0);
        else if (tile->unk_2F0>1) {
            const auto* tmp=reinterpret_cast<const TMPStruct*>(tile->GetImage());
            const TMPImage* image=nullptr;
            if (tmp && tmp->Columns>0 && tmp->Rows>0 && tmp->Columns<=255 && tmp->Rows<=255)
                tmp->GetSubTile(static_cast<unsigned char>(Height)%(tmp->Columns*tmp->Rows),image);
            variant=image && (image->Flags&4) ? (static_cast<DWORD>(Flags)>>13)&1 : GetTileVariant(index,tile->unk_2F0);
        }
        if (variant>tile->unk_2F0-1) variant%=tile->unk_2F0;
        output=tile;
        return true;
    } catch (...) { variant=0; return false; }
}

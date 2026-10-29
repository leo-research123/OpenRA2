/*
    ObjectTypes are initialized by INI files.
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractTypeClass.h"
#include "yrpp/FileSystem.h"

#include "yrpp/Drawing.h"
#include "yrpp/IndexClass.h"

// forward declarations
class TechnoTypeClass;
class HouseTypeClass;
class ObjectClass;
class BuildingClass;

class NOVTABLE ObjectTypeClass : public AbstractTypeClass {
public:
    static DynamicVectorClass<ObjectTypeClass*>& Array;  // AC1418
    // IPersistStream
    /// VA: 0x005F9720; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override { JMP_STD(0x005F9720); }
    /// VA: 0x005F9950; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override { JMP_STD(0x005F9950); }
    virtual HRESULT YRPP_STDCALL GetSizeMax(ULARGE_INTEGER* pcbSize) override;

    // Common fields and resource preparation; see api/type_resources.hpp.
    /// VA: 0x005F92D0.
    virtual bool LoadFromINI(CCINIClass* ini) override;
    // False reports failed/unavailable native dependencies; exceptions stay local.
    /// VA: 0x005F9070
    bool LoadTypeImage();
    /// VA: 0x005F9070
    void LoadTypeImageOriginal() { JMP_THIS(0x005F9070); }

    // Destructor
    virtual ~ObjectTypeClass();

    // ObjectTypeClass
    /// VA: 0x0041CF80; local body in core/src/yrpp.
    virtual CoordStruct* vt_entry_6C(CoordStruct* dest, CoordStruct* source) const;

    /// VA: 0x00428E40; local body in core/src/yrpp.
    virtual DWORD GetOwners() const;
    /// VA: 0x005F75B0; local body in core/src/yrpp.
    virtual int GetPipMax() const;
    /// VA: 0x005F75C0; reference retained, not a local implementation.
    virtual void vt_entry_78(DWORD dwUnk) const { JMP_THIS(0x005F75C0); }
    /// VA: 0x005F75E0
    virtual CoordStruct* Dimension2(CoordStruct* pDest) { JMP_THIS(0x005F75E0); }
    /// VA: implementation-defined (pure virtual).
    virtual bool SpawnAtMapCoords(CellStruct* pMapCoords, HouseClass* pOwner) = 0;
    /// VA: 0x005F7610; local body in core/src/yrpp.
    virtual int GetActualCost(HouseClass* pHouse) const;
    /// VA: 0x005F7620; local body in core/src/yrpp.
    virtual int GetBuildSpeed() const;
    /// VA: implementation-defined (pure virtual).
    virtual ObjectClass* CreateObject(HouseClass* pOwner) = 0;
    /// VA: 0x005F7640
    virtual CellStruct* GetFoundationData(bool includeBib) const;
    /// VA: 0x005F7900
    virtual BuildingClass* FindFactory(bool allowOccupied, bool requirePower, bool requireCanBuild,
                                       HouseClass const* house) const;
    /// VA: 0x005F7630; local body in core/src/yrpp.
    virtual SHPStruct* GetCameo() const;
    virtual SHPStruct* GetImage() const;

    /// VA: 0x005004E0.
    static bool YRPP_FASTCALL IsBuildCat5(AbstractType abstractID, int idx) { JMP_STD(0x5004E0); }

    // the same thunk as IsBuildCat5, with the return type the value actually has
    /// VA: 0x005004E0.
    static BuildCat YRPP_FASTCALL GetBuildCat(AbstractType abstractID, int idx) { JMP_STD(0x5004E0); }

    /// VA: 0x0048DCD0.
    static TechnoTypeClass* YRPP_FASTCALL GetTechnoType(AbstractType abstractID, int idx) { JMP_STD(0x48DCD0); }

    /// VA: 0x005F8110.
#if defined(RA2_YRPP_GAME)
    void LoadVoxel() { JMP_STD(0x5F8110); }
#else
    void LoadVoxel();
#endif

    // Loads the optional voxel turret/barrel of a unit with a sprite body.
    // The original caller ignores the incidental contents of AL on return.
    // Host exceptions are contained; partially loaded optional resources stay attached.
    /// VA: 0x005F8CE0
#if defined(RA2_YRPP_GAME)
    void LoadSpriteTurretVoxels() noexcept { JMP_THIS(0x5F8CE0); }
#else
    void LoadSpriteTurretVoxels() noexcept;
#endif

    // Startup load of the shared PIPBRD/PIPS/PIPS2/TALKBUBL shapes into their
    // FileSystem globals; a shape already loaded is kept.
    /// VA: 0x005F76B0
#if defined(RA2_YRPP_GAME)
    static void One_Time() { JMP_STD(0x5F76B0); }
#else
    static void One_Time();
#endif

    // Shutdown release of the One_Time shapes that were read into owned memory.
    /// VA: 0x005F77F0
#if defined(RA2_YRPP_GAME)
    static void Free_One_Time() { JMP_STD(0x5F77F0); }
#else
    static void Free_One_Time();
#endif

    // Loads one charger turret VXL/HVA pair; false when the pair is unusable.
    /// VA: 0x005F7A90.
#if defined(RA2_YRPP_GAME)
    bool LoadChargerTurretVoxel(const char* name, int index) { JMP_THIS(0x5F7A90); }
#else
    bool LoadChargerTurretVoxel(const char* name, int index);
#endif

    // Loads one charger barrel VXL/HVA pair; a missing barrel file is accepted.
    /// VA: 0x005F7DB0.
#if defined(RA2_YRPP_GAME)
    bool LoadChargerBarrelVoxel(const char* name, int index) { JMP_THIS(0x5F7DB0); }
#else
    bool LoadChargerBarrelVoxel(const char* name, int index);
#endif

    // Releases every charger turret and barrel pair.
    /// VA: 0x005F8080.
#if defined(RA2_YRPP_GAME)
    void ReleaseChargerVoxels() { JMP_THIS(0x5F8080); }
#else
    void ReleaseChargerVoxels();
#endif

    // Constructor
    ObjectTypeClass(const char* pID) noexcept;
    ObjectTypeClass(const ObjectTypeClass&) = delete;
    ObjectTypeClass& operator=(const ObjectTypeClass&) = delete;

protected:
    explicit __forceinline ObjectTypeClass(noinit_t) noexcept : AbstractTypeClass(noinit_t()) {}

    // Properties

public:
    ColorStruct RadialColor;
    BYTE unused_9B;
    Armor Armor;
    int Strength;
    SHPStruct* Image = nullptr;
    bool ImageAllocated = false;
    PROTECTED_PROPERTY(BYTE, align_A9[3]);
    SHPStruct* AlphaImage;
    VoxelStruct MainVoxel{};
    VoxelStruct TurretVoxel{};  //also used for WO voxels
    VoxelStruct BarrelVoxel{};

    VoxelStruct ChargerTurrets[0x12]{};
    VoxelStruct ChargerBarrels[0x12]{};

    bool NoSpawnAlt;
    PROTECTED_PROPERTY(BYTE, align_1E9[3]);
    int MaxDimension;
    int CrushSound;    //index
    int AmbientSound;  //index

    char ImageFile[0x19];

    bool AlternateArcticArt;
    bool ArcticArtInUse;  //not read from ini

    char AlphaImageFile[0x19];

    bool Theater;
    bool Crushable;
    bool Bombable;
    bool RadarInvisible;
    bool Selectable;
    bool LegalTarget;
    bool Insignificant;
    bool Immune;
    bool IsLogic;  // add objects to the logic vector
    bool AllowCellContent;
    bool Voxel;
    bool NewTheater;
    bool HasRadialIndicator;
    bool IgnoresFirestorm;
    bool UseLineTrail;
    ColorStruct LineTrailColor;
    PROTECTED_PROPERTY(BYTE, align_23E[2]);
    int LineTrailColorDecrement;

    IndexClass<VoxelIndexKey, VoxelCacheStruct*> VoxelMainCache;
    IndexClass<VoxelIndexKey, VoxelCacheStruct*> VoxelTurretWeaponCache;
    IndexClass<ShadowVoxelIndexKey, VoxelCacheStruct*> VoxelShadowCache;
    IndexClass<VoxelIndexKey, VoxelCacheStruct*> VoxelTurretBarrelCache;
};

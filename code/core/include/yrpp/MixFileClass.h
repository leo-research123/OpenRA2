// YRpp MixFileClass.h, calibrated to the fixed gamemd.exe (SOURCE.json).
// EA-derived method implementations retain their notices in MixFileClass.cpp.
#pragma once

#include "yrpp/GenericList.h"
#include "yrpp/platform/ABI.h"

template<typename T> class DynamicVectorClass;
struct MixHeaderData { DWORD ID; DWORD Offset; DWORD Size; };
static_assert(sizeof(MixHeaderData) == 12);

// EA WWLib PKey (pk.h), calibrated to gamemd 40D9C0/632870/633130.
// The original Int<64> values contain 64 little-endian 32-bit limbs each.
// Keep the MIX key contract here, without adding a new YRpp header.
class PKey {
public:
    PKey() noexcept = default;
    PKey(const void* exponent, const void* modulus) noexcept;
    void Decode_Modulus(const void* der) noexcept;
    void Decode_Exponent(const void* der) noexcept;
    int Plain_Block_Size() const noexcept { return int((int64_t(BitPrecision) - 1) / 8); }
    int Crypt_Block_Size() const noexcept { return Plain_Block_Size() + 1; }
    int Block_Count(int length) const noexcept;
    int Encrypt(const void* source, int length, void* destination) const noexcept;
    int Decrypt(const void* source, int length, void* destination) const noexcept;

    uint32_t Modulus[64]{};
    uint32_t Exponent[64]{};
    int32_t BitPrecision = 0;
};
static_assert(sizeof(PKey) == 0x204);
static_assert(offsetof(PKey, Exponent) == 0x100);
static_assert(offsetof(PKey, BitPrecision) == 0x200);

class MixFileClass : public Node<MixFileClass> {
public:
    struct GenericMixFiles
    {
        MixFileClass* RA2MD;
        MixFileClass* RA2;
        MixFileClass* LANGUAGE;
        MixFileClass* LANGMD;
        MixFileClass* THEATER_TEMPERAT;
        MixFileClass* THEATER_TEMPERATMD;
        MixFileClass* THEATER_TEM;
        MixFileClass* GENERIC;
        MixFileClass* GENERMD;
        MixFileClass* THEATER_ISOTEMP;
        MixFileClass* THEATER_ISOTEM;
        MixFileClass* ISOGEN;
        MixFileClass* ISOGENMD;
        MixFileClass* MOVIES02D;
        MixFileClass* UNKNOWN_1;
        MixFileClass* MAIN;
        MixFileClass* CONQMD;
        MixFileClass* CONQUER;
        MixFileClass* CAMEOMD;
        MixFileClass* CAMEO;
        MixFileClass* CACHEMD;
        MixFileClass* CACHE;
        MixFileClass* LOCALMD;
        MixFileClass* LOCAL;
        MixFileClass* NTRLMD;
        MixFileClass* NEUTRAL;
        MixFileClass* MAPSMD02D;
        MixFileClass* MAPS02D;
        MixFileClass* UNKNOWN_2;
        MixFileClass* UNKNOWN_3;
        MixFileClass* SIDEC02DMD;
        MixFileClass* SIDEC02D;
    };

    // Active-stage globals bind once to host storage or original game memory.
    // Later-stage arrays below remain declarations until their phase is ported.
    static List<MixFileClass>& MIXes; // upstream slot 0xABEFD8
    static DynamicVectorClass<MixFileClass*>& Array; // upstream slot 0x884D90
    static DynamicVectorClass<MixFileClass*>& Array_Alt; // upstream slot 0x884DC0
    static DynamicVectorClass<MixFileClass*>& Maps; // upstream slot 0x884DA8
    static DynamicVectorClass<MixFileClass*>& Movies; // upstream slot 0x884DE0
    static MixFileClass*& MULTIMD; // upstream slot 0x884DD8
    static MixFileClass*& MULTI; // upstream slot 0x884DDC
    static GenericMixFiles& Generics; // upstream slot 0x884DF8
    static MixFileClass*& SIDENC; // Original 0x884E78, after Generics' existing range.

    // Target 5301A0 returns AL; upstream void/JMP_THIS declaration is corrected.
    static bool YRPP_CDECL Bootstrap();
    // 0x00530460..0x0053067F common-map package portion of later startup:
    // CONQMD, four generic terrain archives, then CONQUER.
    // Requires a bound core resource environment; already mounted slots remain.
    static bool LoadTerrainMixes() noexcept;
    // Installed-directory map branch of 530460: MAPSMD*.MIX, falling back
    // to MAPS*.MIX, followed by MULTIMD.MIX. CD selection remains separate.
    static bool LoadMapMixes() noexcept;
    // 534FA0: Yuri shares side index 1's archives. MD, base, then noncached.
    // Consumers must release side-dependent images before changing the side.
    static bool LoadSidebarMixes(int side_index) noexcept;
    static void UnloadSidebarMixes() noexcept;
    explicit MixFileClass(const char* filename);
    // Original 5B3C20 contract. A null key leaves the input stream unchanged.
    MixFileClass(const char* filename, const PKey* key);
    static const PKey& DefaultKey() noexcept;
    ~MixFileClass() override;
    MixFileClass(const MixFileClass&) = delete;
    MixFileClass& operator=(const MixFileClass&) = delete;
    static bool YRPP_FASTCALL Offset(const char* filename, void** data,
        MixFileClass** mixfile, int* offset, int* length); // 0x5B4430
    static void* Retrieve(char* name, bool forceShapeCache); // Global VA: 0x005B40B0, not migrated
    static void DestroyCache(); // 0x5B4310, not migrated
    static bool YRPP_FASTCALL Cache(const char* = nullptr, const void* = nullptr);
    // Host-only inspection helpers, no extra object state or virtual slots.
    const MixHeaderData* find(uint32_t id) const;
    // True for non-null storage with a positive count; otherwise outputs null/0.
    // Borrows this MIX's storage. Do not modify the directory or unload the MIX
    // while using the returned entries; callers must serialize those operations.
    bool headers(const MixHeaderData*& entries, int& count) const noexcept;

    const char* FileName = nullptr;
    bool IsDigest = false; // upstream Blowfish at +10 is actually the digest flag
    bool IsEncrypted = false;
    bool IsAllocated = false; // upstream omits the owned-body flag at +12
    int CountFiles = 0;
    int FileSize = 0;
    int FileStartOffset = 0;
    MixHeaderData* Headers = nullptr;
    void* Data = nullptr; // upstream field_24 is a pointer
};

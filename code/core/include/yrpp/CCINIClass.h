// YRpp 9402d7da; original members calibrated.
#pragma once

#include "yrpp/platform/ABI.h"
#include "yrpp/BasicStructures.h"
#include "yrpp/YRMathVector.h"
#include <cstddef>
#include "yrpp/GenericList.h"
#include "yrpp/ArrayClasses.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/IndexClass.h"

using Point2D = Vector2D<int>;
using CoordStruct = Vector3D<int>;
class Straw;
class Pipe;
using byte = BYTE;
class TechnoTypeClass;

// Basic INI class
class INIClass
{
public:
    struct INIComment
    {
        char* Value = nullptr;
        INIComment* Next = nullptr;
    };

    class INIEntry : public Node<INIEntry>
    {
        public:
            INIEntry() = default;
            ~INIEntry() override;
            // Owns the key, value and comments. Copying is disabled for project
            // safety; this does not reconstruct the original copy contract.
            INIEntry(const INIEntry&) = delete;
            INIEntry& operator=(const INIEntry&) = delete;

            char* Key = nullptr;
            char* Value = nullptr;
            INIComment* Comments = nullptr;
            char* CommentString = nullptr;
            int PreIndentCursor = 0;
            int PostIndentCursor = 0;
            int CommentCursor = 0;
    };

    class INISection : public Node<INISection>
    {
        public:

            INISection() = default;
            ~INISection() override;

            char* Name = nullptr;
            List<INIEntry> Entries;
            IndexClass <int, INIEntry*> EntryIndex;
            INIComment* Comments = nullptr;
    };

    INIClass();

protected:
    INIClass(bool);

public:
    virtual ~INIClass();

    // Target 0x525A60 / 0x526470, missing from the upstream declaration.
    int ReadStraw(Straw& input, bool loadComments = false);
    int WritePipe(Pipe& output);
    INIEntry* FindEntry(const char* section, const char* key); // 0x526B10
    static bool IsBlankValue(const char* value);
    INIClass(const INIClass&) = delete;
    INIClass& operator=(const INIClass&) = delete;
    void Reset();

    bool Clear(const char* section = nullptr, const char* key = nullptr);

    INISection* GetSection(const char* pSection);

    int GetKeyCount(const char* pSection); //Get the amount of keys in a section.
    const char* GetKeyName(const char* pSection, int nKeyIndex); //Get the name of a key number in a section.

    // Reads an ANSI string. Returns the string's length.
    int ReadString(const char* pSection, const char* pKey, const char* pDefault, char* pBuffer, size_t szBufferSize);
    int GetString(const char* pSection, const char* pKey, char* pBuffer,size_t szBufferSize);

    // Writes an ANSI string.
    bool WriteString(const char* pSection, const char* pKey, const char* pString);

    // Escaped UTF-16 units; returns the first-NUL length. Capacity includes the
    // terminator (repairs the target's Count+1 overflow / unterminated fallback).
    int ReadUnicodeString(const char* pSection, const char* pKey, const wchar_t* pDefault, wchar_t* pBuffer, size_t szBufferSize);
    // Writes an escaped Unicode string.
    bool WriteUnicodeString(const char* pSection, const char* pKey, const wchar_t* pString);

    // Reads an boolean value.
    bool ReadBool(const char* pSection, const char* pKey, bool bDefault);
    void GetBool(const char* pSection, const char* pKey, bool& bValue);
    // Writes an boolean value.
    bool WriteBool(const char* pSection, const char* pKey, bool bValue);

    // Reads an integer value.
    int ReadInteger(const char* pSection, const char* pKey, int nDefault);
    void GetInteger(const char* pSection, const char* pKey, int& nValue);
    // Writes an integer value.
    bool WriteInteger(const char* pSection, const char* pKey, int nValue, int format = 0);

    // Reads a decimal value.
    /// VA: 0x005283D0.
    double ReadDouble(const char* pSection, const char* pKey, double dDefault);
    void GetDouble(const char* pSection, const char* pKey, double& nValue);

    // Writes a decimal value.
    bool WriteDouble(const char* pSection, const char* pKey, double dValue);

    int ReadRate(const char* pSection, const char* pKey, int nDefault);
    void GetRate(const char* pSection, const char* pKey, int& nValue);
    bool WriteRate(const char* pSection, const char* pKey, int nValue);

    // Missing/malformed components use defaults instead of target stack garbage.
    // Reads two integer values.
    int* Read2Integers(int* pBuffer, const char* pSection, const char* pKey, int* pDefault);
    Point2D* ReadPoint2D(Point2D& ret, const char* pSection, const char* pKey, Point2D& defValue);
    void GetPoint2D(const char* pSection, const char* pKey, Point2D& value);
    // Writes two integer values.
    bool Write2Integers(const char* pSection, const char* pKey, int* pValues);

    // Reads three integer values.
    int* Read3Integers(int* pBuffer, const char* pSection, const char* pKey, int* pDefault);
    // 527CC0: a missing key parses "0,0,0,0"; an empty key retains defaults.
    int* Read4Integers(int* output, const char* section, const char* key, int* fallback);
    CoordStruct* ReadPoint3D(CoordStruct& ret, const char* pSection, const char* pKey, CoordStruct& defValue);
    void GetPoint3D(const char* pSection, const char* pKey, CoordStruct& value);

    // Reads three byte values.
    byte* Read3Bytes(byte* pBuffer, const char* pSection, const char* pKey, byte* pDefault);
    // Writes three byte values.
    bool Write3Bytes(const char* pSection, const char* pKey, byte* pValues);

    // Tests whether the given section and key exists. If key is NULL, only the section will be looked for.
    bool Exists(const char* pSection, const char* pKey);

    int ReadTime(const char* pSection, const char* pKey, int nDefault);

    bool WriteTime(const char* pSection, const char* pKey, int nValue);

    // C&C helpers. Registry/StringTable lookups require the host's existing game
    // services (api/ini_runtime.hpp), automatically bound by the original target.

#define INI_READ(item, addr) \
    int Read ## item(const char* pSection, const char* pKey, int pDefault) \
        ;

    // Target uses pDefault as a name-table index, then scans the whole table.
    INI_READ(Pip, 0x4748A0);

    // PipScale= to idx
    INI_READ(PipScale, 0x474940);

    // Category= to idx
    INI_READ(Category, 0x4749E0);

    // Color=%s to idx
    INI_READ(ColorString, 0x474A90);

    // Foundation= to idx
    INI_READ(Foundation, 0x474DA0);

    // MovementZone= to idx
    INI_READ(MovementZone, 0x474E40);

    // SpeedType= to idx
    INI_READ(SpeedType, 0x476FC0);

    // [SW]Action= to idx
    INI_READ(SWAction, 0x474EE0);

    // [SW]Type= to idx
    INI_READ(SWType, 0x474F50);

    // EVA Event name to idx
    INI_READ(VoxName, 0x474FA0);

    // Factory= to idx
    INI_READ(Factory, 0x474FF0);

    INI_READ(BuildCat, 0x475060);

    // Parses a list of Countries and returns a bitfield, i.e. Owner= or RequiredHouses=
    INI_READ(HouseTypesList, 0x4750D0);

    // Parses a list of Houses and returns a bitfield, i.e. Allies= in map
    INI_READ(HousesList, 0x475260);

    INI_READ(ArmorType, 0x4753F0);

    INI_READ(LandType, 0x4754B0);

    // supports MP names (<Player @ X>) too, wtf
    // ALLOCATES if country name is not found
    // returns idx of country it reads
    INI_READ(HouseType, 0x475540);

    // ALLOCATES if name is not found
    INI_READ(Side, 0x4756F0);

    // returns index of movie with this filename
    INI_READ(Movie, 0x4757D0);

    // map theater
    INI_READ(Theater, 0x475870);

    INI_READ(Theme, 0x4758F0);

    INI_READ(Edge, 0x475980);

    INI_READ(Powerup, 0x4759F0);

    // [Anim]Layer= to idx
    INI_READ(Layer, 0x477050);

    INI_READ(VHPScan, 0x477590);

    // Color=%d,%d,%d to idx , used to parse [Colors]
    ColorStruct* ReadColor(ColorStruct* pBuffer, const char* pSection, const char* pKey, ColorStruct const& defValue);

    ColorStruct ReadColor(const char* const pSection, const char* const pKey, ColorStruct const& defValue);

    void GetColor(const char* const pSection, const char* const pKey, ColorStruct& value);

    bool WriteColor(const char* const pSection, const char* const pKey, ColorStruct const& color);

    // OverlayPack, OverlayDataPack, IsoMapPack5
    // Those uses 1=xxxx, 2=xxxx, 3=xxxx .etc.
    size_t ReadUUBlock(const char* const pSection, void* pBuffer, size_t length);

    bool WriteUUBlock(const char* const pSection, void* pBuffer, size_t length);

    // 18 bytes
    byte* ReadAbilities(byte* pBuffer, const char* pSection, const char* pKey, byte* pDefault);

    TechnoTypeClass* GetTechnoType(const char* pSection, const char* pKey);

    // safer and more convenient overload for string reading
    template <size_t Size>
    constexpr int ReadString(const char* pSection, const char* pKey, const char* pDefault, char(&pBuffer)[Size])
    {
        return this->ReadString(pSection, pKey, pDefault, pBuffer, Size);
    }

    template <size_t Size>
    constexpr int GetString(const char* pSection, const char* pKey, char(&pBuffer)[Size])
    {
        return ReadString(pSection, pKey, pBuffer, pBuffer);
    }

    // safer and more convenient overload for escaped unicode string reading
    template <size_t Size>
    constexpr int ReadUnicodeString(const char* pSection, const char* pKey, const wchar_t* pDefault, wchar_t(&pBuffer)[Size])
    {
        return this->ReadUnicodeString(pSection, pKey, pDefault, pBuffer, Size);
    }

    // Original return-buffer ABI: pBuffer is uninitialized TypeList storage.
    // Destroy a previous value before reusing its storage.
    static TypeList<int>* YRPP_FASTCALL GetPrerequisites(TypeList<int>* pBuffer, INIClass* pINI,
        const char* pSection, const char* pKey, TypeList<int> Defaults);
    // Explicit return-buffer forms of 475D70 / 4764F0, following GetPrerequisites
    // above. These signatures are not raw EXE entry signatures. Output is
    // uninitialized storage; the caller destroys the constructed TypeList.
    static TypeList<int>* YRPP_FASTCALL GetIntegers(TypeList<int>* output, INIClass* ini,
        const char* section, const char* key, TypeList<int> defaults);
    static TypeList<TechnoTypeClass*>* YRPP_FASTCALL GetTechnoTypes(TypeList<TechnoTypeClass*>* output,
        INIClass* ini, const char* section, const char* key, TypeList<TechnoTypeClass*> defaults);
    bool WriteIntegers(const char* section, const char* key, const TypeList<int>& values); // 475F30
    bool WriteTechnoTypes(const char* section, const char* key, const TypeList<TechnoTypeClass*>& values); // 4766F0
    bool WriteMovie(const char* section, const char* key, int index); // 475820
    bool WriteTheme(const char* section, const char* key, int index); // 475950

    static bool IsBlank(const char *pValue);

    // Properties

public:
private:
    INIEntry* ReadEntry(const char* section, const char* key);
public:
    using IndexType = IndexClass<int, INISection*>;

    char* CurrentSectionName = nullptr; // borrowed pointer-identity cache, original +04
    INISection* CurrentSection = nullptr;
    List<INISection> Sections;
    IndexType SectionIndex; // <CRCValue of the Name, Pointer to the section>
    INIComment* LineComments = nullptr;
};

// Extended INI class specified for C&C use
class CCINIClass : public INIClass
{
public:
    // Target composition binds references to host storage or the original globals.
    // STATIC
    static DWORD& RulesHash;
    static DWORD& ArtHash;
    static DWORD& AIHash;

    // westwood genius shines again

    // this is a pointer in the class
    static CCINIClass*& INI_Rules;

    // these are static class variables, why the fuck did you differentiate them, WW?
    static CCINIClass& INI_AI;
    static CCINIClass& INI_Art;
    static CCINIClass& INI_UIMD;
    static CCINIClass& INI_RA2MD;

    // non-static
    CCINIClass();

    ~CCINIClass() override;

    int LoadFromFile(const char* filename, bool loadComments = false);
    static CCINIClass* LoadINIFile(const char* filename);
    static void UnloadINIFile(CCINIClass*& ini);

    // Target status: 0=parse failure, 1=success, 2=missing/mismatched digest.
    // Parses an INI file from a CCFile
    int ReadCCFile(FileClass* pCCFile, bool bDigest = false, bool bLoadComments = false);

    int WriteCCFile(FileClass *pCCFile, bool bDigest = false);

    // Copies the string table entry pointed to by the INI value into pBuffer.
    int ReadStringtableEntry(const char* pSection, const char* pKey, wchar_t* pBuffer, size_t szBufferSize);

    template <size_t Size>
    int ReadStringtableEntry(const char* pSection, const char* pKey, wchar_t(&pBuffer)[Size])
    {
        return this->ReadStringtableEntry(pSection, pKey, pBuffer, Size);
    }

    DWORD GetCRC();

private:
    void CalculateDigest();

    // Properties

public:

    bool Digested = false; // original byte +40; no host flags in this object
    byte Digest[20]{};
};

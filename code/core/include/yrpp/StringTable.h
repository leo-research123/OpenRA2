/*
    StringTable related stuff
*/

#pragma once

#include "yrpp/platform/ABI.h"
#include <cstring>
#include <cwchar>

#define CSF_SIGNATURE 0x43534620 //" FSC"
#define CSF_LABEL_SIGNATURE 0x4C424C20 //" LBL"
#define CSF_VALUE_SIGNATURE 0x53545220 //" RTS"
#define CSF_EXVALUE_SIGNATURE 0x53545257 //"WRTS"

enum class CSFLanguages : unsigned int
{
    US = 0,
    UK = 1,
    German = 2,
    French = 3,
    Spanish = 4,
    Italian = 5,
    Japanese = 6,
    Jabberwockie = 7,
    Korean = 8,
    Chinese = 9,
    Unknown = 10
};

struct CSFHeader
{
    DWORD Signature; //should be CSF_SIGNATURE
    int CSFVersion; //RA2 uses 3
    int NumLabels;
    int NumValues;
    DWORD unused_0xC;
    CSFLanguages Language; //CSF_LANG_*, forced to US if CSFVersion < 2
};

struct CSFLabel
{
    char Name[0x20]; //limits the label name length to 31
    int NumValues; //one label can have multiple values attached, that's never used though
    int FirstValueIndex; //in the global StringTable::Values() array
};

struct CSFString
{
    CSFString* PreviousEntry;
    wchar_t Text[258]; // 0x208-byte node on Windows x86 (734ED6).

    CSFString() : PreviousEntry(nullptr)
    {
        *Text = 0;
    }
};

struct CSFLanguage
{
    CSFLanguages Index; // one of the language constants
    char const* Name;   // the display name
    char const* Short;  // two letter language code
    char const* Letter; // one letter language code
};

class StringTable
{
public:
    static CSFString*& LastLoadedString;
    static int& MaxLabelLen;
    static int& LabelCount;
    static int& ValueCount;
    static CSFLanguages& Language;
    static int& IsLoaded;
    static char*& FileName;
    static CSFLabel*& Labels;
    static wchar_t**& Values;
    static char**& ExtraValues;

    static const wchar_t* YRPP_FASTCALL LoadString(
        const char* pLabel,
        char** pOutExtraData = nullptr,
        const char* pSourceCodeFileName = __FILE__,
        int nSourceCodeFileLine = __LINE__
    );

    static const wchar_t* FetchString(
        const char* pLabel,
        const wchar_t* pDefault = L"",
        char** pSpeech = nullptr,
        const char* pFile = __FILE__,
        int nLine = __LINE__
    )
    {
        if (pLabel && strlen(pLabel) && !IsNone(pLabel))
            return LoadString(pLabel, pSpeech, pFile, nLine);
        else
            return pDefault;
    }

    static const wchar_t* TryFetchString(
        const char* pLabel,
        const wchar_t* pDefault = L"",
        char** pSpeech = nullptr,
        const char* pFile = __FILE__,
        int nLine = __LINE__
    )
    {
        if (pLabel && strlen(pLabel) && !IsNone(pLabel))
        {
            auto lpValue = LoadString(pLabel, pSpeech, pFile, nLine);
            if (wcsncmp(lpValue, L"MISSING:", 8))
                return lpValue;
        }

        return pDefault;
    }

    // Serial resource lifecycle. FileName borrows the caller's string; values
    // and missing-label nodes remain valid until Unload or a successful ReadFile.
    // Malformed input fails without publishing a partial table.
    static bool YRPP_FASTCALL LoadFile(const char* pFileName);

    static bool YRPP_FASTCALL ReadFile(const char* pFileName);

    static CSFLanguage const* YRPP_FASTCALL GetLanguage(CSFLanguages language);

    static const char* YRPP_FASTCALL GetLanguageName(CSFLanguages language);

    static void Unload();

private:
    static bool IsNone(const char* label);
};

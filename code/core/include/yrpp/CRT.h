#pragma once

#include "yrpp/platform/ABI.h"

#include <wchar.h>

#include "yrpp/ASMMacros.h"
// contains functions that are part of the C runtime library and have been declared ingame
// just declaring them so we don't need to include our own duplicates

class CRT {
public:
        // unicode manipulations - "wcs" stands for "wide char string" or wchar_t equivalent of "str"

        /// VA: 0x007CA489.
        static wchar_t * YRPP_CDECL wcscpy(wchar_t * Dest, const wchar_t *Src)
            { JMP_STD(0x7CA489); }

        /// VA: 0x007CA422.
        static wchar_t * YRPP_CDECL wcsncpy(wchar_t *Dest, const wchar_t *Source, size_t Count)
            { JMP_STD(0x7CA422); }

        /// VA: 0x007CA3C5.
        static wchar_t *YRPP_CDECL wcsrchr(const wchar_t *Str, wchar_t Ch)
            { JMP_STD(0x7CA3C5); }

        /// VA: 0x007CA405.
        static size_t YRPP_CDECL wcslen(const wchar_t *Str)
            { JMP_STD(0x7CA405); }

        /// VA: 0x007CD7CE.
        static size_t YRPP_CDECL wcscspn(const wchar_t* pFirst ,const wchar_t* pSecond )
            { JMP_STD(0x7CD7CE); }

        /// VA: 0x007CA45F.
        static wchar_t *YRPP_CDECL wcscat(wchar_t *Dest, const wchar_t *Source)
            { JMP_STD(0x7CA45F); }

        /// VA: 0x007CA5D3.
        static int YRPP_CDECL wcscmp(const wchar_t *Str1, const wchar_t *Str2)
            { JMP_STD(0x7CA5D3); }

        /// VA: 0x007CA564.
        static int YRPP_CDECL swprintf(wchar_t *Buffer, const wchar_t *Format, ...)
            { JMP_STD(0x7CA564); }

        /// VA: 0x00727D60.
        static wchar_t * YRPP_FASTCALL wcstrim(wchar_t *Buffer)
            { JMP_STD(0x727D60); }

        /// VA: 0x007CA8C6.
        static wchar_t* YRPP_CDECL wcschr(const wchar_t*Str ,wchar_t a2)
            { JMP_STD(0x7CA8C6); }

        /// VA: 0x007CB504.
        static wchar_t* YRPP_CDECL wcsncat(wchar_t* a1, const wchar_t* a2, size_t a3)
            { JMP_STD(0x7CB504); }

        // memory management
        /// VA: 0x007C9430.
        static void *YRPP_CDECL malloc(size_t sz)
            { JMP_STD(0x7C9430); }

        /// VA: 0x007C93E8.
        static void YRPP_CDECL free(const void* p)
            { JMP_STD(0x7C93E8); }

        /// VA: 0x007C8E17.
        static void *YRPP_CDECL _new(size_t sz)
            { JMP_STD(0x7C8E17); }

        /// VA: 0x007C8B3D.
        static void YRPP_CDECL _delete(void *p)
            { JMP_STD(0x7C8B3D); }

        /// VA: 0x007D75E0.
        static void*YRPP_CDECL _memset(void* p, int nInt, size_t sz)
            { JMP_STD(0x7D75E0);}

        /// VA: 0x007CA090.
        static void* YRPP_CDECL _memmove(void* dst, const void* src, size_t count)
            { JMP_STD(0x7CA090); }

        // strings
        /// VA: 0x007C9BFD.
        static int YRPP_CDECL atoi(const char* Str)
            { JMP_STD(0x7C9BFD);}

        /// VA: 0x007C9D66.
        static double YRPP_CDECL atof(const char* Str)
            { JMP_STD(0x7C9D66); }

        /// VA: 0x007C99E1.
        static int YRPP_CDECL isspace(char* Str)
            { JMP_STD(0x7C99E1); }

        /// VA: 0x007D5408.
        static char* YRPP_CDECL strdup(const char *Src)
            { JMP_STD(0x7D5408); }

        /// VA: 0x007D4C00.
        static char* YRPP_CDECL strcats(char* StrTo, char* StrFrom)
            { JMP_STD(0x7D4C00); }

        /// VA: 0x007D4C00.
        static char* YRPP_CDECL strcat(char* StrTo, const char* StrFrom)
            { JMP_STD(0x7D4C00); }

        /// VA: 0x007D4BF0.
        static char* YRPP_CDECL strcpy(char* StrTo, const char* StrFrom)
            { JMP_STD(0x7D4BF0);}

        /// VA: 0x007C8D20.
        static int YRPP_CDECL strcmpi(const char *lhs, const char *rhs)
            { JMP_STD(0x7C8D20); }

        /// VA: 0x007CDA90.
        static int YRPP_CDECL strcmp(const char *lhs, const char *rhs)
            { JMP_STD(0x7CDA90); }

        /// VA: 0x007CAF30.
        static char *YRPP_CDECL strchr(const char *Str, int Val)
            { JMP_STD(0x7CAF30); }

        /// VA: 0x007C8DF0.
        static char *YRPP_CDECL strrchr(const char *Str, int Ch)
            { JMP_STD(0x7C8DF0); }

        /// VA: 0x007C91D0.
        static char *YRPP_CDECL strncpy(char *Dest, const char *Source, size_t Count)
            { JMP_STD(0x7C91D0); }

        /// VA: 0x007CA4B0.
        static char *YRPP_CDECL strstr(const char *Str, const char *SubStr)
            { JMP_STD(0x7CA4B0); }

        /// VA: 0x007DCFC4.
        static char *YRPP_CDECL strupr(char* pInput)
            { JMP_STD(0x7DCFC4); }

        /// VA: 0x007CA530.
        static int YRPP_CDECL sscanf(const char *, const char *, ...)
            { JMP_STD(0x7CA530); }

        /// VA: 0x007CD680.
        static int YRPP_CDECL _strnicmp(const char* a1, const char* a2, size_t a3)
            { JMP_STD(0x7CD680); }

        /// VA: 0x007CB550.
        static char *YRPP_CDECL strncat(char *Dest, const char *Source, size_t Count)
            { JMP_STD(0x7CB550); }

        /// VA: 0x007C9CC2.
        static char * YRPP_CDECL strtok(char * Str ,const char * Delim)
            { JMP_STD(0x7C9CC2);}

        /// VA: 0x007C8EF4.
        static int YRPP_CDECL sprintf(char *Buffer, const char *Format, ...)
            { JMP_STD(0x7C8EF4); }

        /// VA: 0x007CB7BA.
        static int YRPP_CDECL vsprintf(char *, const char *, va_list)
            { JMP_STD(0x7CB7BA); }

        /// VA: 0x00727CF0.
        static char * YRPP_FASTCALL strtrim(char * Buffer)
            { JMP_STD(0x727CF0); }

        /// VA: 0x007D15A0.
        static size_t YRPP_CDECL strlen(const char *input)
            { JMP_STD(0x7D15A0); }

        /// VA: 0x007CD790.
        static size_t YRPP_CDECL strcspn(const char* pFirst , const char* pSecond)
            { JMP_STD(0x7CD790); }

        // misc
        /// VA: 0x007CA090.
        static void *YRPP_CDECL memcpy(void *Dst, const void *Src, size_t Size)
            { JMP_STD(0x7CA090); }

        /// VA: 0x007D0A20.
        static void *YRPP_CDECL memcpy_B(void *Dst, const void *Src, size_t Size)
            { JMP_STD(0x7D0A20); }

        /// VA: 0x007C8B48.
        static void YRPP_CDECL qsort(void *buf, size_t num, size_t size, int (YRPP_CDECL *compare)(const void *lhs, const void *rhs))
            { JMP_STD(0x7C8B48); }

        /// VA: 0x007C8E25.
        static void *YRPP_CDECL bsearch(const void *, const void *, size_t, size_t, int (YRPP_CDECL *)(const void *, const void *))
            { JMP_STD(0x7C8E25); }

        /// VA: 0x007D107D.
        static size_t YRPP_CDECL msize(void* nBlock)
            { JMP_STD(0x7D107D); }

        /// VA: 0x007C5EE4.
        static void YRPP_CDECL setfpmode()
            { JMP_STD(0x7C5EE4); }

        /// VA: 0x007CC2AC.
        static size_t YRPP_CDECL mbstowcs(wchar_t* lpWideCharStr, const char* lpMultiByteStr, size_t a3)
            { JMP_STD(0x7CC2AC); }

        // files
        /// VA: 0x007CA845.
        static FILE *YRPP_CDECL fopen(const char *, const char *)
            { JMP_STD(0x7CA845); }

        /// VA: 0x007C94EB.
        static size_t YRPP_CDECL fread(void *, size_t, size_t, FILE *)
            { JMP_STD(0x7C94EB); }

        /// VA: 0x007C9602.
        static size_t YRPP_CDECL fwrite(const void *, size_t, size_t, FILE *)
            { JMP_STD(0x7C9602); }

        /// VA: 0x007CA7D8.
        static int YRPP_CDECL fprintf(FILE *, const char *, ...)
            { JMP_STD(0x7CA7D8); }

        /// VA: 0x007CB302.
        static int YRPP_CDECL vfprintf(FILE *File, const char *Format, va_list ArgList)
            { JMP_STD(0x7CB302); }

        /// VA: 0x007CB19C.
        static int YRPP_CDECL fflush(FILE *)
            { JMP_STD(0x7CB19C); }

        /// VA: 0x007CA75B.
        static int YRPP_CDECL fclose(FILE *)
            { JMP_STD(0x7CA75B); }

        /// VA: 0x007C9FF0.
        static void YRPP_CDECL makepath(char* arg1 , const char* arg2, const char* arg3 ,  const char* arg4, const char* arg5)
            { JMP_STD(0x7C9FF0); }

        // exit
        /// VA: 0x007CBDDC.
        [[noreturn]] static void YRPP_CDECL exit(int Code)
            { JMP_STD(0x7CBDDC); }

        /// VA: 0x007C970C.
        _onexit_t YRPP_CDECL onexit(_onexit_t Func)
            { JMP_STD(0x7C970C); }
};

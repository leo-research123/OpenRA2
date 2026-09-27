/*
    EVA Messages!
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ArrayClasses.h"
#include "yrpp/GeneralDefinitions.h"
#include "yrpp/CCINIClass.h"

#include "yrpp/Helpers/CompileTime.h"

class AudioStream;

class VoxClass
{
public:
    /// Global VA: 0x00B1D4A0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<VoxClass*>, Array, 0xB1D4A0u)
#else
    static DynamicVectorClass<VoxClass*>& Array;
#endif

    /// Global VA: 0x00B1D4C8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(int, EVAIndex, 0xB1D4C8u)
#else
    static int& EVAIndex;
#endif

    static VoxClass* Find(const char* pName)
    {
        for(int i = 0; i < Array.Count; ++i) {
            if(!_strcmpi(Array[i]->Name, pName)) {
                return Array[i];
            }
        }
        return nullptr;
    }

    static int FindIndex(const char* pName)
    {
        for(int i = 0; i < Array.Count; ++i) {
            if(!_strcmpi(Array[i]->Name, pName)) {
                return i;
            }
        }
        return -1;
    }

    /// VA: 0x00752700
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static void YRPP_FASTCALL Play(const char* name, int control = -1, int priority = -1) { JMP_STD(0x752700); }
#else
    static void YRPP_FASTCALL Play(const char* name, int control = -1, int priority = -1) noexcept;
#endif

    /// VA: 0x00752480
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static void YRPP_FASTCALL PlayIndex(int index, int control = -1, int priority = -1) { JMP_STD(0x752480); }
#else
    static void YRPP_FASTCALL PlayIndex(int index, int control = -1, int priority = -1) noexcept;
#endif

    /// VA: 0x00750E20.
    static void YRPP_FASTCALL PlayAtPos(int index, CoordStruct *pCoords, DWORD dwUnk = 0)
        { JMP_STD(0x750E20); }

    // Remove all waiting requests for this definition; current playback is not stopped.
    /// VA: 0x00752A40
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static void YRPP_FASTCALL SilenceIndex(int index) { JMP_STD(0x752A40); }
#else
    static void YRPP_FASTCALL SilenceIndex(int index) noexcept;
#endif

    /// VA: 0x00753330
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static const char* YRPP_FASTCALL GetName(int index) { JMP_STD(0x753330); }
#else
    static const char* YRPP_FASTCALL GetName(int index) noexcept;
#endif

    /// VA: 0x007531A0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static void DeleteAll() { JMP_STD(0x7531A0); }
#else
    static void DeleteAll() noexcept;
#endif


    // Original audio circular link (0x004072C0/0x004072D0), then the EVA
    // request payload allocated by 0x00752590. No replacement queue model.
    struct QueueLink { QueueLink* Next; QueueLink* Previous; QueueLink* Sentinel; };
    struct QueueEntry : QueueLink {
        VoxClass* Voice;
        int Unused; // Original enqueue leaves offset 0x10 untouched.
        int Priority;
        int Control;
        int Serial;
    };
    /// Global VA: 0xB1D3C8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(QueueLink, InterruptQueue, 0xB1D3C8)
#else
    static QueueLink& InterruptQueue;
#endif
    /// Global VA: 0xB1D3F0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(QueueLink, CriticalQueue, 0xB1D3F0)
#else
    static QueueLink& CriticalQueue;
#endif
    /// Global VA: 0xB1D4B8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(QueueEntry*, StandardSlot, 0xB1D4B8)
#else
    static QueueEntry*& StandardSlot;
#endif
    /// Global VA: 0xB1D4BC.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(int, Initialized, 0xB1D4BC)
#else
    static int& Initialized;
#endif
    /// Global VA: 0xB1D4C0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(int, NextSerial, 0xB1D4C0)
#else
    static int& NextSerial;
#endif
    /// Global VA: 0xB1D4C4.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(VoxClass*, Current, 0xB1D4C4)
#else
    static VoxClass*& Current;
#endif
    /// Global VA: 0xB1D4CC.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(AudioStream*, Stream, 0xB1D4CC)
#else
    static AudioStream*& Stream;
#endif
    /// Global VA: 0xB1D4D0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(unsigned long long, GapMilliseconds, 0xB1D4D0)
#else
    static unsigned long long& GapMilliseconds;
#endif
    /// Global VA: 0xB1D3B8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(int, CurrentControl, 0xB1D3B8)
#else
    static int& CurrentControl;
#endif
    /// Global VA: 0xB1D3E0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(int, CurrentPriority, 0xB1D3E0)
#else
    static int& CurrentPriority;
#endif
    /// Global VA: 0xB1D3D8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(int, SuppressCount, 0xB1D3D8)
#else
    static int& SuppressCount;
#endif
    /// Global VA: 0xB1D428.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(int, PauseCount, 0xB1D428)
#else
    static int& PauseCount;
#endif
    /// Global VA: 0x00B1D450.
#if defined(RA2_YRPP_GAME)
    DEFINE_ARRAY_REFERENCE(QueueLink, [4], PriorityQueues, 0xB1D450)
#else
    static QueueLink (&PriorityQueues)[4];
#endif
    /// VA: 0x00752290
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static bool Initialize() { JMP_STD(0x752290); }
#else
    static bool Initialize() noexcept;
#endif
    /// VA: 0x00752340
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static void Shutdown() { JMP_STD(0x752340); }
#else
    static void Shutdown() noexcept;
#endif
    /// VA: 0x00752370
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static void ClearQueue() { JMP_STD(0x752370); }
#else
    static void ClearQueue() noexcept;
#endif
    /// VA: 0x00752590
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static void YRPP_FASTCALL Enqueue(VoxClass* voice, int control, int priority) { JMP_STD(0x752590); }
#else
    static void YRPP_FASTCALL Enqueue(VoxClass* voice, int control, int priority) noexcept;
#endif
    /// VA: 0x00752680
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static QueueEntry* YRPP_FASTCALL FindQueued(const VoxClass* voice) { JMP_STD(0x752680); }
#else
    static QueueEntry* YRPP_FASTCALL FindQueued(const VoxClass* voice) noexcept;
#endif
    /// VA: 0x00752760
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static void Update() { JMP_STD(0x752760); }
#else
    static void Update() noexcept;
#endif
    /// VA: 0x007529A0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static void YRPP_FASTCALL Stop(bool clearQueue) { JMP_STD(0x7529A0); }
#else
    static void YRPP_FASTCALL Stop(bool clearQueue) noexcept;
#endif
    /// VA: 0x007529E0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static bool IsSpeaking() { JMP_STD(0x7529E0); }
#else
    static bool IsSpeaking() noexcept;
#endif
    /// VA: 0x00753000
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static bool YRPP_FASTCALL LoadAllFromINI(CCINIClass* ini) { JMP_STD(0x753000); }
#else
    static bool YRPP_FASTCALL LoadAllFromINI(CCINIClass* ini) noexcept;
#endif
    /// VA: 0x00753570
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static int Suppress() { JMP_STD(0x753570); }
#else
    static int Suppress() noexcept;
#endif
    /// VA: 0x00753580
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static int Unsuppress() { JMP_STD(0x753580); }
#else
    static int Unsuppress() noexcept;
#endif
    /// VA: 0x007535B0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static int Pause() { JMP_STD(0x7535B0); }
#else
    static int Pause() noexcept;
#endif
    /// VA: 0x007535D0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static void Reset() { JMP_STD(0x7535D0); }
#else
    static void Reset() noexcept;
#endif
    /// VA: 0x00753620
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static int Resume() { JMP_STD(0x753620); }
#else
    static int Resume() noexcept;
#endif

    // Properties

public:

    char Name[0x28];
    float Volume;			//as in eva.ini
    char Yuri [0x9];		//as in eva.ini
    char Russian [0x9];		//as in eva.ini
    char Allied [0x9];		//as in eva.ini
    VoxPriority Priority;	//as in eva.ini
    VoxType Type;			//as in eva.ini
    int unknown_int_50;

    // Definition registration/removal; stop and clear queued references before deletion.
    /// VA: 0x00752CB0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) VoxClass(const char* name) { JMP_THIS(0x752CB0); }
#else
    VoxClass(const char* name) noexcept;
#endif

    /// VA: 0x00752D60
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) ~VoxClass() { JMP_THIS(0x752D60); }
#else
    ~VoxClass() noexcept;
#endif

    /// VA: 0x00753380
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) const char* GetFilename() const { JMP_THIS(0x753380); }
#else
    const char* GetFilename() const noexcept;
#endif

    /// VA: 0x00752DB0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) bool LoadFromINI(CCINIClass* ini) { JMP_THIS(0x752DB0); }
#else
    bool LoadFromINI(CCINIClass* ini) noexcept;
#endif
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(VoxClass)==0x54);
static_assert(sizeof(VoxClass::QueueLink)==0x0C);
static_assert(sizeof(VoxClass::QueueEntry)==0x20);
#endif

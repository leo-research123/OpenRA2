#pragma once

#include "yrpp/platform/ABI.h"
#include "yrpp/BasicStructures.h"
#include "yrpp/ArrayClasses.h"
#include "yrpp/Helpers/CompileTime.h"

class AlphaLightingRemapClass
{
public:
    static DynamicVectorClass<AlphaLightingRemapClass*>& Array; // Global VA: 0x0088A080

    // Notice:
    // When a ConvertClass is constructed by the game, it will generate [IntensityCount] color
    // tables from dark to bright. Each of them just changes the intensity of the source palette.
    //
    // Find or create the remapping table for the requested intensity count.
    /// VA: 0x00420140.
    static AlphaLightingRemapClass* YRPP_STDCALL FindOrAllocate(int intensityCount);

    // Release a reference and free the table when its reference count reaches zero.
    /// VA: 0x00420270.
    static void YRPP_STDCALL Release(AlphaLightingRemapClass* pItem);

    // The image hook map records this entry; existing YRpp notes also describe inlined call sites.
    /// VA: 0x004202F0.
    explicit AlphaLightingRemapClass(int intensityCount) noexcept;

    WORD Table[256][256];
    int IntensityCount;
    int RefCount;
};

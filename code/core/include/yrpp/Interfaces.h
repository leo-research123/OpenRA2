#pragma once

#include "yrpp/platform/ABI.h"
#include "yrpp/YRPPCore.h"
#include "yrpp/GeneralDefinitions.h"

#ifdef _WIN32
#include <comdef.h>
#else
using LONG = std::int32_t;
using ULONG = std::uint32_t;
using HRESULT = std::int32_t;
using VARIANT_BOOL = std::int16_t;
using BSTR = wchar_t*;
struct GUID { std::uint32_t Data1; std::uint16_t Data2, Data3; std::uint8_t Data4[8]; };
using IID = GUID;
using CLSID = GUID;
using REFIID = const IID&;
union ULARGE_INTEGER { struct { std::uint32_t LowPart, HighPart; }; std::uint64_t QuadPart; };
struct IStream;
struct IEnumConnectionPoints;
struct IConnectionPoint;
struct IUnknown {
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL QueryInterface(REFIID, void**) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual ULONG YRPP_STDCALL AddRef() = 0;
    /// VA: implementation-defined (pure virtual).
    virtual ULONG YRPP_STDCALL Release() = 0;
};
/// VA: implementation-defined (pure virtual).
struct IPersist : IUnknown { virtual HRESULT YRPP_STDCALL GetClassID(CLSID*) = 0; };
struct IConnectionPointContainer : IUnknown {
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL EnumConnectionPoints(IEnumConnectionPoints**) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL FindConnectionPoint(REFIID, IConnectionPoint**) = 0;
};
struct IPersistStream : IPersist {
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL IsDirty() = 0;
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL Load(IStream*) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL Save(IStream*, BOOL) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL GetSizeMax(ULARGE_INTEGER*) = 0;
};
#define __interface struct
#define _COM_SMARTPTR_TYPEDEF(interface_type, iid)
#endif
class Matrix3D;
struct DirStruct;

__interface __declspec(uuid("5FF0CA70-8B12-11D1-B708-00A024DDAFD1"))
ISwizzle : IUnknown
{
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL Reset() = 0;
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL Swizzle(void** pointer) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL Fetch_Swizzle_ID(void* pointer, LONG* id) const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL Here_I_Am(LONG id, void* pointer) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL Save_Interface(IStream* stream, IUnknown* pointer) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL Load_Interface(IStream* stream, GUID* riid, void** pointer) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL Get_Save_Size(int* size) const = 0;
};

_COM_SMARTPTR_TYPEDEF(ISwizzle, __uuidof(ISwizzle));

__interface __declspec(uuid("96F02EC3-6FE8-11D1-B6FD-00A024DDAFD1"))
IApplication : IUnknown
{
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL FullName(BSTR* retval) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL Name(BSTR* retval) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL Quit() = 0;
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL ScenarioName(BSTR* retval) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL FrameCount(long* retval) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL Swizzle_Interface(ISwizzle** pVal) = 0;
};

_COM_SMARTPTR_TYPEDEF(IApplication, __uuidof(IApplication));

__interface INoticeSource
{
    /// VA: implementation-defined (pure virtual).
    virtual void YRPP_STDCALL INoticeSource_Unknown() = 0;
};

__interface INoticeSink
{
    /// VA: implementation-defined (pure virtual).
    virtual bool YRPP_STDCALL INoticeSink_Unknown(DWORD dwUnknown) = 0;
};

__interface __declspec(uuid("170DAC82-12E4-11D2-8175-006008055BB5"))
IRTTITypeInfo : IUnknown
{
    /// VA: implementation-defined (pure virtual).
    virtual AbstractType YRPP_STDCALL What_Am_I() const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual int YRPP_STDCALL Fetch_ID() const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual void YRPP_STDCALL Create_ID() = 0;
};

_COM_SMARTPTR_TYPEDEF(IRTTITypeInfo, __uuidof(IRTTITypeInfo));

__interface __declspec(uuid("96F02EC4-6FE8-11D1-B6FD-00A024DDAFD1"))
IAIHouse : IUnknown
{
    /// VA: implementation-defined (pure virtual).
    virtual void YRPP_STDCALL Link_House(void* unknown) const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual void YRPP_STDCALL AI(int* unknown) = 0;
};

_COM_SMARTPTR_TYPEDEF(IAIHouse, __uuidof(IAIHouse));

__interface __declspec(uuid("941582E0-86DA-11D1-B706-00A024DDAFD1"))
IHouse : IUnknown
{
    /// VA: implementation-defined (pure virtual).
    virtual long YRPP_STDCALL				ID_Number() const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual BSTR YRPP_STDCALL				Name() const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual IApplication* YRPP_STDCALL		Get_Application() = 0;
    /// VA: implementation-defined (pure virtual).
    virtual long YRPP_STDCALL				Available_Money() const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual long YRPP_STDCALL				Available_Storage() const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual long YRPP_STDCALL				Power_Output() const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual long YRPP_STDCALL				Power_Drain() const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual long YRPP_STDCALL				Category_Quantity(Category category) const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual long YRPP_STDCALL				Category_Power(Category category) const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual CellStruct YRPP_STDCALL		Base_Center() const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL			Fire_Sale() const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL			All_To_Hunt() = 0;
};

_COM_SMARTPTR_TYPEDEF(IHouse, __uuidof(IHouse));

__interface __declspec(uuid("CAACF210-86E3-11D1-B706-00A024DDAFD1"))
IPublicHouse : IUnknown
{
    /// VA: implementation-defined (pure virtual).
    virtual long YRPP_STDCALL			ID_Number() const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual BSTR YRPP_STDCALL			Name() const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual long YRPP_STDCALL			Apparent_Category_Quantity(Category category) const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual long YRPP_STDCALL			Apparent_Category_Power(Category category) const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual CellStruct YRPP_STDCALL	Apparent_Base_Center() const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual bool YRPP_STDCALL			Is_Powered() const = 0;
};

_COM_SMARTPTR_TYPEDEF(IPublicHouse, __uuidof(IPublicHouse));

/*
typedef struct tagCONNECTDATA
{
    IUnknown* pUnk;
    unsigned long dwCookie;
}	CONNECTDATA;

__interface __declspec(uuid("B196B287-BAB4-101A-B69C-00AA00341D07"))
IEnumConnections : IUnknown
{
    virtual HRESULT YRPP_STDCALL RemoteNext(unsigned long cConnections, CONNECTDATA* rgcd, unsigned long* pcFetched) = 0;
    virtual HRESULT YRPP_STDCALL Skip(unsigned long cConnections) = 0;
    virtual HRESULT YRPP_STDCALL Reset() = 0;
    virtual HRESULT YRPP_STDCALL Clone(IEnumConnections** ppEnum) = 0;
};

__interface IConnectionPointContainer;
__interface __declspec(uuid("B196B286-BAB4-101A-B69C-00AA00341D07"))
IConnectionPoint : IUnknown
{
    virtual HRESULT YRPP_STDCALL GetConnectionInterface(GUID* pIID) = 0;
    virtual HRESULT YRPP_STDCALL GetConnectionPointContainer(IConnectionPointContainer** ppCPC) = 0;
    virtual HRESULT YRPP_STDCALL Advise(IUnknown* pUnkSink, unsigned long* pdwCookie) = 0;
    virtual HRESULT YRPP_STDCALL Unadvise(unsigned long dwCookie) = 0;
    virtual HRESULT YRPP_STDCALL EnumConnections(IEnumConnections** ppEnum) = 0;
};

__interface IEnumConnectionPoints;
__interface __declspec(uuid("B196B284-BAB4-101A-B69C-00AA00341D07"))
IConnectionPointContainer : IUnknown
{
    virtual HRESULT YRPP_STDCALL	EnumConnectionPoints(IEnumConnectionPoints** ppEnum) = 0;
    virtual HRESULT YRPP_STDCALL	FindConnectionPoint(REFIID riid, IConnectionPoint** ppCP) = 0;
};

__interface __declspec(uuid("B196B285-BAB4-101A-B69C-00AA00341D07"))
IEnumConnectionPoints : IUnknown
{
    virtual HRESULT YRPP_STDCALL RemoteNext(unsigned long cConnections, IConnectionPoint** ppCP, unsigned long* pcFetched) = 0;
    virtual HRESULT YRPP_STDCALL Skip(unsigned long cConnections) = 0;
    virtual HRESULT YRPP_STDCALL Reset() = 0;
    virtual HRESULT YRPP_STDCALL Clone(IEnumConnectionPoints** ppEnum) = 0;
};
*/

__interface __declspec(uuid("96F02EC7-6FE8-11D1-B6FD-00A024DDAFD1"))
IGameMap : IUnknown
{
    /// VA: implementation-defined (pure virtual).
    virtual long YRPP_STDCALL Is_Visible(CellStruct cell) = 0;
};

_COM_SMARTPTR_TYPEDEF(IGameMap, __uuidof(IGameMap));

__interface __declspec(uuid("0D5CD78E-6470-11D2-9B74-00104B972FE8"))
ILinkStream : IUnknown
{
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL Link_Stream(IUnknown* stream) = 0;
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL Unlink_Stream(IUnknown** stream) = 0;
};

_COM_SMARTPTR_TYPEDEF(ILinkStream, __uuidof(ILinkStream));

__interface __declspec(uuid("820F501C-4F39-11D2-9B70-00104B972FE8"))
IFlyControl : IUnknown
{
    /// VA: implementation-defined (pure virtual).
    virtual int YRPP_STDCALL Landing_Altitude() = 0;
    /// VA: implementation-defined (pure virtual).
    virtual int YRPP_STDCALL Landing_Direction() = 0;
    /// VA: implementation-defined (pure virtual).
    virtual long YRPP_STDCALL Is_Loaded() = 0;
    /// VA: implementation-defined (pure virtual).
    virtual long YRPP_STDCALL Is_Strafe() = 0;
    /// VA: implementation-defined (pure virtual).
    virtual long YRPP_STDCALL Is_Fighter() = 0;
    /// VA: implementation-defined (pure virtual).
    virtual long YRPP_STDCALL Is_Locked() = 0;
};

_COM_SMARTPTR_TYPEDEF(IFlyControl, __uuidof(IFlyControl));

__interface __declspec(uuid("070F3290-9841-11D1-B709-00A024DDAFD1"))
ILocomotion : IUnknown
{
    // Links object to locomotor.
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL Link_To_Object(void* pointer) = 0;

    // Sees if object is moving.
    /// VA: implementation-defined (pure virtual).
    virtual bool YRPP_STDCALL Is_Moving() = 0;

    // Fetches destination coordinate.
    /// VA: implementation-defined (pure virtual).
    virtual CoordStruct YRPP_STDCALL Destination() = 0;

    //  Fetches immediate (next cell) destination coordinate.
    /// VA: implementation-defined (pure virtual).
    virtual CoordStruct YRPP_STDCALL Head_To_Coord() = 0;

    // Determine if specific cell can be entered.
    /// VA: implementation-defined (pure virtual).
    virtual Move YRPP_STDCALL Can_Enter_Cell(CellStruct cell) = 0;

    // Should object cast a shadow?
    /// VA: implementation-defined (pure virtual).
    virtual bool YRPP_STDCALL Is_To_Have_Shadow() = 0;

    // Fetch voxel draw matrix.
    /// VA: implementation-defined (pure virtual).
    virtual Matrix3D YRPP_STDCALL Draw_Matrix(union VoxelIndexKey* pIndex) = 0;

    // Fetch shadow draw matrix.
    /// VA: implementation-defined (pure virtual).
    virtual Matrix3D YRPP_STDCALL Shadow_Matrix(union VoxelIndexKey* pIndex) = 0;

    // Draw point center location.
    /// VA: implementation-defined (pure virtual).
    virtual Point2D YRPP_STDCALL Draw_Point() = 0;

    // Shadow draw point center location.
    /// VA: implementation-defined (pure virtual).
    virtual Point2D YRPP_STDCALL Shadow_Point() = 0;

    // Visual character for drawing.
    /// VA: implementation-defined (pure virtual).
    virtual VisualType YRPP_STDCALL Visual_Character(bool raw) = 0;

    // Z adjust control value.
    /// VA: implementation-defined (pure virtual).
    virtual int YRPP_STDCALL Z_Adjust() = 0;

    // Z gradient control value.
    /// VA: implementation-defined (pure virtual).
    virtual ZGradient YRPP_STDCALL Z_Gradient() = 0;

    // Process movement of object.]
    /// VA: implementation-defined (pure virtual).
    virtual bool YRPP_STDCALL Process() = 0;

    // Instruct to move to location specified.
    /// VA: implementation-defined (pure virtual).
    virtual void YRPP_STDCALL Move_To(CoordStruct to) = 0;

    // Stop moving at first opportunity.
    /// VA: implementation-defined (pure virtual).
    virtual void YRPP_STDCALL Stop_Moving() = 0;

    // Try to face direction specified.
    /// VA: implementation-defined (pure virtual).
    virtual void YRPP_STDCALL Do_Turn(DirStruct coord) = 0;

    // Object is appearing in the world.
    /// VA: implementation-defined (pure virtual).
    virtual void YRPP_STDCALL Unlimbo() = 0;

    // Special tilting AI function.
    /// VA: implementation-defined (pure virtual).
    virtual void YRPP_STDCALL Tilt_Pitch_AI() = 0;

    // Locomotor becomes powered.
    /// VA: implementation-defined (pure virtual).
    virtual bool YRPP_STDCALL Power_On() = 0;

    // Locomotor loses power.
    /// VA: implementation-defined (pure virtual).
    virtual bool YRPP_STDCALL Power_Off() = 0;

    // Is locomotor powered?
    /// VA: implementation-defined (pure virtual).
    virtual bool YRPP_STDCALL Is_Powered() = 0;

    // Is locomotor sensitive to ion storms?
    /// VA: implementation-defined (pure virtual).
    virtual bool YRPP_STDCALL Is_Ion_Sensitive() = 0;

    // Push object in direction specified.
    /// VA: implementation-defined (pure virtual).
    virtual bool YRPP_STDCALL Push(DirStruct dir) = 0;

    // Shove object (with spin) in direction specified.
    /// VA: implementation-defined (pure virtual).
    virtual bool YRPP_STDCALL Shove(DirStruct dir) = 0;

    // Force drive track -- special case only.
    /// VA: implementation-defined (pure virtual).
    virtual void YRPP_STDCALL Force_Track(int track, CoordStruct coord) = 0;

    // What display layer is it located in.
    /// VA: implementation-defined (pure virtual).
    virtual Layer YRPP_STDCALL In_Which_Layer() = 0;

    // Don't use this function.
    /// VA: implementation-defined (pure virtual).
    virtual void YRPP_STDCALL Force_Immediate_Destination(CoordStruct coord) = 0;

    // Force a voxel unit to a given slope. Used in cratering.
    /// VA: implementation-defined (pure virtual).
    virtual void YRPP_STDCALL Force_New_Slope(int ramp) = 0;

    // Is it actually moving across the ground this very second?
    /// VA: implementation-defined (pure virtual).
    virtual bool YRPP_STDCALL Is_Moving_Now() = 0;

    // Actual current speed of object expressed as leptons per game frame.
    /// VA: implementation-defined (pure virtual).
    virtual int YRPP_STDCALL Apparent_Speed() = 0;

    // Special drawing feedback code (locomotor specific meaning)
    /// VA: implementation-defined (pure virtual).
    virtual int YRPP_STDCALL Drawing_Code() = 0;

    // Queries if any locomotor specific state prevents the object from firing.
    /// VA: implementation-defined (pure virtual).
    virtual FireError YRPP_STDCALL Can_Fire() = 0;

    // Queries the general state of the locomotor.
    /// VA: implementation-defined (pure virtual).
    virtual int YRPP_STDCALL Get_Status() = 0;

    // Forces a hunter seeker droid to find a target.
    /// VA: implementation-defined (pure virtual).
    virtual void YRPP_STDCALL Acquire_Hunter_Seeker_Target() = 0;

    // Is this object surfacing?
    /// VA: implementation-defined (pure virtual).
    virtual bool YRPP_STDCALL Is_Surfacing() = 0;

    // Lifts all occupation bits associated with the object off the map
    /// VA: implementation-defined (pure virtual).
    virtual void YRPP_STDCALL Mark_All_Occupation_Bits(MarkType mark) = 0;

    // Is this object in the process of moving into this coord.
    /// VA: implementation-defined (pure virtual).
    virtual bool YRPP_STDCALL Is_Moving_Here(CoordStruct to) = 0;

    // Will this object jump tracks?
    /// VA: implementation-defined (pure virtual).
    virtual bool YRPP_STDCALL Will_Jump_Tracks() = 0;

    // Infantry moving query function.
    /// VA: implementation-defined (pure virtual).
    virtual bool YRPP_STDCALL Is_Really_Moving_Now() = 0;

    // Falsifies the IsReallyMoving flag in WalkLocomotionClass.
    /// VA: implementation-defined (pure virtual).
    virtual void YRPP_STDCALL Stop_Movement_Animation() = 0;

    // Object is disappearing from the world.
    // Was added post TLB generation.
    /// VA: implementation-defined (pure virtual).
    virtual void YRPP_STDCALL Limbo() = 0;

    // Locks the locomotor from being deleted.
    /// VA: implementation-defined (pure virtual).
    virtual void YRPP_STDCALL Lock() = 0;

    // Unlocks the locomotor from being deleted.
    /// VA: implementation-defined (pure virtual).
    virtual void YRPP_STDCALL Unlock() = 0;

    // Queries internal variables.
    /// VA: implementation-defined (pure virtual).
    virtual int YRPP_STDCALL Get_Track_Number() = 0;

    // Queries internal variables.
    /// VA: implementation-defined (pure virtual).
    virtual int YRPP_STDCALL Get_Track_Index() = 0;

    // Queries internal variables.
    /// VA: implementation-defined (pure virtual).
    virtual int YRPP_STDCALL Get_Speed_Accum() = 0;
};

_COM_SMARTPTR_TYPEDEF(ILocomotion, __uuidof(ILocomotion));

// 'Piggyback' one locomotor onto another.
__interface __declspec(uuid("92FEA800-A184-11D1-B70A-00A024DDAFD1"))
IPiggyback : IUnknown
{
    // Piggybacks a locomotor onto this one.
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL Begin_Piggyback(ILocomotion* pointer) = 0;

    // End piggyback process and restore locomotor interface pointer.
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL End_Piggyback(ILocomotion** pointer) = 0;

    // Determines when should the piggybacking be ended (done automatically in FootClass::AI).
    /// VA: implementation-defined (pure virtual).
    virtual bool YRPP_STDCALL Is_Ok_To_End() = 0;

    // Fetches piggybacked locomotor class ID.
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL Piggyback_CLSID(GUID* classid) = 0;

    // Is it currently piggybacking another locomotor?
    /// VA: implementation-defined (pure virtual).
    virtual bool YRPP_STDCALL Is_Piggybacking() = 0;
};

_COM_SMARTPTR_TYPEDEF(IPiggyback, __uuidof(IPiggyback));

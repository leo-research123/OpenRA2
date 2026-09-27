#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/Interfaces.h"
#include "yrpp/FootClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/Drawing.h"
#include "yrpp/Matrix3D.h"
#include "yrpp/Helpers/CompileTime.h"

class LocomotionClass : public IPersistStream, public ILocomotion
{
public:
    class CLSIDs
    {
    public:
        /// Global VA: 0x007E9A30.
#if defined(RA2_YRPP_GAME)
        DEFINE_REFERENCE(CLSID const, Drive, 0x7E9A30u)
#else
        inline static constexpr CLSID Drive{0x4A582741,0x9839,0x11D1,{0xB7,0x09,0,0xA0,0x24,0xDD,0xAF,0xD1}};
#endif
        /// Global VA: 0x007E9A40.
#if defined(RA2_YRPP_GAME)
        DEFINE_REFERENCE(CLSID const, Hover, 0x7E9A40u)
#else
        inline static constexpr CLSID Hover{0x4A582742,0x9839,0x11D1,{0xB7,0x09,0,0xA0,0x24,0xDD,0xAF,0xD1}};
#endif
        /// Global VA: 0x007E9A50.
#if defined(RA2_YRPP_GAME)
        DEFINE_REFERENCE(CLSID const, Tunnel, 0x7E9A50u)
#else
        inline static constexpr CLSID Tunnel{0x4A582743,0x9839,0x11D1,{0xB7,0x09,0,0xA0,0x24,0xDD,0xAF,0xD1}};
#endif
        /// Global VA: 0x007E9A60.
#if defined(RA2_YRPP_GAME)
        DEFINE_REFERENCE(CLSID const, Walk, 0x7E9A60u)
#else
        inline static constexpr CLSID Walk{0x4A582744,0x9839,0x11D1,{0xB7,0x09,0,0xA0,0x24,0xDD,0xAF,0xD1}};
#endif
        /// Global VA: 0x007E9A70.
#if defined(RA2_YRPP_GAME)
        DEFINE_REFERENCE(CLSID const, Droppod, 0x7E9A70u)
#else
        inline static constexpr CLSID Droppod{0x4A582745,0x9839,0x11D1,{0xB7,0x09,0,0xA0,0x24,0xDD,0xAF,0xD1}};
#endif
        /// Global VA: 0x007E9A80.
#if defined(RA2_YRPP_GAME)
        DEFINE_REFERENCE(CLSID const, Fly, 0x7E9A80u)
#else
        inline static constexpr CLSID Fly{0x4A582746,0x9839,0x11D1,{0xB7,0x09,0,0xA0,0x24,0xDD,0xAF,0xD1}};
#endif
        /// Global VA: 0x007E9A90.
#if defined(RA2_YRPP_GAME)
        DEFINE_REFERENCE(CLSID const, Teleport, 0x7E9A90u)
#else
        inline static constexpr CLSID Teleport{0x4A582747,0x9839,0x11D1,{0xB7,0x09,0,0xA0,0x24,0xDD,0xAF,0xD1}};
#endif
        /// Global VA: 0x007E9AA0.
#if defined(RA2_YRPP_GAME)
        DEFINE_REFERENCE(CLSID const, Mech, 0x7E9AA0u)
#else
        inline static constexpr CLSID Mech{0x55D141B8,0xDB94,0x11D1,{0xAC,0x98,0,0x60,0x08,0x05,0x5B,0xB5}};
#endif
        /// Global VA: 0x007E9AB0.
#if defined(RA2_YRPP_GAME)
        DEFINE_REFERENCE(CLSID const, Ship, 0x7E9AB0u)
#else
        inline static constexpr CLSID Ship{0x2BEA74E1,0x7CCA,0x11D3,{0xBE,0x14,0,0x10,0x4B,0x62,0xA1,0x6C}};
#endif
        /// Global VA: 0x007E9AC0.
#if defined(RA2_YRPP_GAME)
        DEFINE_REFERENCE(CLSID const, Jumpjet, 0x7E9AC0u)
#else
        inline static constexpr CLSID Jumpjet{0x92612C46,0xF71F,0x11D1,{0xAC,0x9F,0,0x60,0x08,0x05,0x5B,0xB5}};
#endif
        /// Global VA: 0x007E9AD0.
#if defined(RA2_YRPP_GAME)
        DEFINE_REFERENCE(CLSID const, Rocket, 0x7E9AD0u)
#else
        inline static constexpr CLSID Rocket{0xB7B49766,0xE576,0x11D3,{0x9B,0xD9,0,0x10,0x4B,0x97,0x2F,0xE8}};
#endif
    };

    // IUnknown
    /// VA: 0x0055A9B0.
#if defined(RA2_YRPP_GAME)
    virtual HRESULT YRPP_STDCALL QueryInterface(REFIID iid, void** ppvObject) { JMP_STD(0x55A9B0); }
#else
    virtual HRESULT YRPP_STDCALL QueryInterface(REFIID iid, void** ppvObject);
#endif
    /// VA: 0x0055A950.
#if defined(RA2_YRPP_GAME)
    virtual ULONG YRPP_STDCALL AddRef() { JMP_STD(0x55A950); }
#else
    virtual ULONG YRPP_STDCALL AddRef();
#endif
    /// VA: 0x0055A970.
#if defined(RA2_YRPP_GAME)
    virtual ULONG YRPP_STDCALL Release() { JMP_STD(0x55A970); }
#else
    virtual ULONG YRPP_STDCALL Release();
#endif

    // IPersist
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) = 0;

    // IPersistStream
    /// VA: 0x004B4C30.
#if defined(RA2_YRPP_GAME)
    virtual HRESULT YRPP_STDCALL IsDirty() { JMP_STD(0x4B4C30); }
#else
    virtual HRESULT YRPP_STDCALL IsDirty() { return Dirty ? 0 : 1; }
#endif
    /// VA: 0x0055AAC0.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) { JMP_STD(0x55AAC0); } // Replaces the historical _purecall mapping at 0x004C9150.
    /// VA: 0x0055AA60.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) { JMP_STD(0x55AA60); }

    /// VA: 0x0055AB40.
#if defined(RA2_YRPP_GAME)
    virtual HRESULT YRPP_STDCALL GetSizeMax(ULARGE_INTEGER* pcbSize) { JMP_STD(0x55AB40); }
#else
    virtual HRESULT YRPP_STDCALL GetSizeMax(ULARGE_INTEGER* pcbSize);
#endif

    /// VA: 0x0055A6F0
    virtual ~LocomotionClass() { LinkedTo = nullptr; }
    /// VA: implementation-defined (pure virtual).
    virtual int Size() = 0;

    // ILocomotion
    // virtual HRESULT YRPP_STDCALL QueryInterface(REFIID iid, void** ppvObject) { JMP_STD(0x4D0510); }
    // virtual ULONG YRPP_STDCALL AddRef() { JMP_STD(0x4D0520); }
    // virtual ULONG YRPP_STDCALL Release() { JMP_STD(0x4D0530); }
    /// VA: 0x0055A710.
#if defined(RA2_YRPP_GAME)
    virtual HRESULT YRPP_STDCALL Link_To_Object(void* pointer) { JMP_STD(0x55A710); }
#else
    virtual HRESULT YRPP_STDCALL Link_To_Object(void* pointer) { Owner = LinkedTo = static_cast<FootClass*>(pointer); return 0; }
#endif
    /// VA: 0x0055ACD0.
#if defined(RA2_YRPP_GAME)
    virtual bool YRPP_STDCALL Is_Moving() { JMP_STD(0x55ACD0); }
#else
    virtual bool YRPP_STDCALL Is_Moving() { return false; }
#endif
    /// VA: 0x0055AC70.
#if defined(RA2_YRPP_GAME)
    virtual CoordStruct YRPP_STDCALL Destination() { JMP_STD(0x55AC70); }
#else
    virtual CoordStruct YRPP_STDCALL Destination() { return CoordStruct::Empty; }
#endif
    /// VA: 0x0055ACA0.
#if defined(RA2_YRPP_GAME)
    virtual CoordStruct YRPP_STDCALL Head_To_Coord() { JMP_STD(0x55ACA0); }
#else
    virtual CoordStruct YRPP_STDCALL Head_To_Coord() { return LinkedTo->Location; }
#endif
    /// VA: 0x0055ABF0.
#if defined(RA2_YRPP_GAME)
    virtual Move YRPP_STDCALL Can_Enter_Cell(CellStruct cell) { JMP_STD(0x55ABF0); }
#else
    virtual Move YRPP_STDCALL Can_Enter_Cell(CellStruct cell) { return static_cast<Move>(0); }
#endif
    /// VA: 0x0055ABE0.
#if defined(RA2_YRPP_GAME)
    virtual bool YRPP_STDCALL Is_To_Have_Shadow() { JMP_STD(0x55ABE0); }
#else
    virtual bool YRPP_STDCALL Is_To_Have_Shadow() { return true; }
#endif
    /// VA: 0x0055A730.
#if defined(RA2_YRPP_GAME)
    virtual Matrix3D YRPP_STDCALL Draw_Matrix(VoxelIndexKey* pIndex) { JMP_STD(0x55A730); }
#else
    virtual Matrix3D YRPP_STDCALL Draw_Matrix(VoxelIndexKey* pIndex);
#endif
    /// VA: 0x0055A7D0.
#if defined(RA2_YRPP_GAME)
    virtual Matrix3D YRPP_STDCALL Shadow_Matrix(VoxelIndexKey* pIndex) { JMP_STD(0x55A7D0); }
#else
    virtual Matrix3D YRPP_STDCALL Shadow_Matrix(VoxelIndexKey* pIndex);
#endif
    /// VA: 0x0055ABD0.
#if defined(RA2_YRPP_GAME)
    virtual Point2D YRPP_STDCALL Draw_Point() { JMP_STD(0x55ABD0); }
#else
    virtual Point2D YRPP_STDCALL Draw_Point() { return Point2D::Empty; }
#endif
    /// VA: 0x0055A8C0.
#if defined(RA2_YRPP_GAME)
    virtual Point2D YRPP_STDCALL Shadow_Point() { JMP_STD(0x55A8C0); }
#else
    virtual Point2D YRPP_STDCALL Shadow_Point();
#endif
    /// VA: 0x0055ABC0.
#if defined(RA2_YRPP_GAME)
    virtual VisualType YRPP_STDCALL Visual_Character(bool raw) { JMP_STD(0x55ABC0); }
#else
    virtual VisualType YRPP_STDCALL Visual_Character(bool raw) { return static_cast<VisualType>(0); }
#endif
    /// VA: 0x0055ABA0.
#if defined(RA2_YRPP_GAME)
    virtual int YRPP_STDCALL Z_Adjust() { JMP_STD(0x55ABA0); }
#else
    virtual int YRPP_STDCALL Z_Adjust() { return 0; }
#endif
    /// VA: 0x0055ABB0.
#if defined(RA2_YRPP_GAME)
    virtual ZGradient YRPP_STDCALL Z_Gradient() { JMP_STD(0x55ABB0); }
#else
    virtual ZGradient YRPP_STDCALL Z_Gradient() { return static_cast<ZGradient>(2); }
#endif
    /// VA: 0x0055AC60.
#if defined(RA2_YRPP_GAME)
    virtual bool YRPP_STDCALL Process() { JMP_STD(0x55AC60); }
#else
    virtual bool YRPP_STDCALL Process() { return true; }
#endif
    /// VA: 0x0055AC50.
#if defined(RA2_YRPP_GAME)
    virtual void YRPP_STDCALL Move_To(CoordStruct to) { JMP_STD(0x55AC50); }
#else
    virtual void YRPP_STDCALL Move_To(CoordStruct to) {  }
#endif
    /// VA: 0x0055AC40.
#if defined(RA2_YRPP_GAME)
    virtual void YRPP_STDCALL Stop_Moving() { JMP_STD(0x55AC40); }
#else
    virtual void YRPP_STDCALL Stop_Moving() {  }
#endif
    /// VA: 0x0055AC30.
#if defined(RA2_YRPP_GAME)
    virtual void YRPP_STDCALL Do_Turn(DirStruct coord) { JMP_STD(0x55AC30); }
#else
    virtual void YRPP_STDCALL Do_Turn(DirStruct coord) {  }
#endif
    /// VA: 0x0055AC20.
#if defined(RA2_YRPP_GAME)
    virtual void YRPP_STDCALL Unlimbo() { JMP_STD(0x55AC20); }
#else
    virtual void YRPP_STDCALL Unlimbo() {  }
#endif
    /// VA: 0x0055AB90.
#if defined(RA2_YRPP_GAME)
    virtual void YRPP_STDCALL Tilt_Pitch_AI() { JMP_STD(0x55AB90); }
#else
    virtual void YRPP_STDCALL Tilt_Pitch_AI() {  }
#endif
    /// VA: 0x0055A8F0.
#if defined(RA2_YRPP_GAME)
    virtual bool YRPP_STDCALL Power_On() { JMP_STD(0x55A8F0); }
#else
    virtual bool YRPP_STDCALL Power_On() { Powered = true; return Is_Powered(); }
#endif
    /// VA: 0x0055A910.
#if defined(RA2_YRPP_GAME)
    virtual bool YRPP_STDCALL Power_Off() { JMP_STD(0x55A910); }
#else
    virtual bool YRPP_STDCALL Power_Off() { Powered = false; return Is_Powered(); }
#endif
    /// VA: 0x0055A930.
#if defined(RA2_YRPP_GAME)
    virtual bool YRPP_STDCALL Is_Powered() { JMP_STD(0x55A930); }
#else
    virtual bool YRPP_STDCALL Is_Powered() { return Powered; }
#endif
    /// VA: 0x0055A940.
#if defined(RA2_YRPP_GAME)
    virtual bool YRPP_STDCALL Is_Ion_Sensitive() { JMP_STD(0x55A940); }
#else
    virtual bool YRPP_STDCALL Is_Ion_Sensitive() { return false; }
#endif
    /// VA: 0x0055AB70.
#if defined(RA2_YRPP_GAME)
    virtual bool YRPP_STDCALL Push(DirStruct dir) { JMP_STD(0x55AB70); }
#else
    virtual bool YRPP_STDCALL Push(DirStruct dir) { return false; }
#endif
    /// VA: 0x0055AB80.
#if defined(RA2_YRPP_GAME)
    virtual bool YRPP_STDCALL Shove(DirStruct dir) { JMP_STD(0x55AB80); }
#else
    virtual bool YRPP_STDCALL Shove(DirStruct dir) { return false; }
#endif
    /// VA: 0x0055AC10.
#if defined(RA2_YRPP_GAME)
    virtual void YRPP_STDCALL Force_Track(int track, CoordStruct coord) { JMP_STD(0x55AC10); }
#else
    virtual void YRPP_STDCALL Force_Track(int track, CoordStruct coord) {  }
#endif
    /// VA: implementation-defined (pure virtual).
    virtual Layer YRPP_STDCALL In_Which_Layer() = 0;
    /// VA: 0x0055AC00.
#if defined(RA2_YRPP_GAME)
    virtual void YRPP_STDCALL Force_Immediate_Destination(CoordStruct coord) { JMP_STD(0x55AC00); }
#else
    virtual void YRPP_STDCALL Force_Immediate_Destination(CoordStruct coord) {  }
#endif
    /// VA: 0x0055ACE0.
#if defined(RA2_YRPP_GAME)
    virtual void YRPP_STDCALL Force_New_Slope(int ramp) { JMP_STD(0x55ACE0); }
#else
    virtual void YRPP_STDCALL Force_New_Slope(int ramp) {  }
#endif
    /// VA: 0x004B6610.
#if defined(RA2_YRPP_GAME)
    virtual bool YRPP_STDCALL Is_Moving_Now() { JMP_STD(0x4B6610); }
#else
    virtual bool YRPP_STDCALL Is_Moving_Now() { return Is_Moving(); }
#endif
    /// VA: 0x0055AD10.
#if defined(RA2_YRPP_GAME)
    virtual int YRPP_STDCALL Apparent_Speed() { JMP_STD(0x55AD10); }
#else
    virtual int YRPP_STDCALL Apparent_Speed() { return LinkedTo->GetCurrentSpeed(); }
#endif
    /// VA: 0x0055ACF0.
#if defined(RA2_YRPP_GAME)
    virtual int YRPP_STDCALL Drawing_Code() { JMP_STD(0x55ACF0); }
#else
    virtual int YRPP_STDCALL Drawing_Code() { return 0; }
#endif
    /// VA: 0x0055AD00.
#if defined(RA2_YRPP_GAME)
    virtual FireError YRPP_STDCALL Can_Fire() { JMP_STD(0x55AD00); }
#else
    virtual FireError YRPP_STDCALL Can_Fire() { return static_cast<FireError>(0); }
#endif
    /// VA: 0x004B4C60.
#if defined(RA2_YRPP_GAME)
    virtual int YRPP_STDCALL Get_Status() { JMP_STD(0x4B4C60); }
#else
    virtual int YRPP_STDCALL Get_Status() { return 0; }
#endif
    /// VA: 0x004B4C70.
#if defined(RA2_YRPP_GAME)
    virtual void YRPP_STDCALL Acquire_Hunter_Seeker_Target() { JMP_STD(0x4B4C70); }
#else
    virtual void YRPP_STDCALL Acquire_Hunter_Seeker_Target() {  }
#endif
    /// VA: 0x004B4C80.
#if defined(RA2_YRPP_GAME)
    virtual bool YRPP_STDCALL Is_Surfacing() { JMP_STD(0x4B4C80); }
#else
    virtual bool YRPP_STDCALL Is_Surfacing() { return false; }
#endif
    /// VA: 0x004B6620.
#if defined(RA2_YRPP_GAME)
    virtual void YRPP_STDCALL Mark_All_Occupation_Bits(MarkType mark) { JMP_STD(0x4B6620); }
#else
    virtual void YRPP_STDCALL Mark_All_Occupation_Bits(MarkType mark) {  }
#endif
    /// VA: 0x004B6630.
#if defined(RA2_YRPP_GAME)
    virtual bool YRPP_STDCALL Is_Moving_Here(CoordStruct to) { JMP_STD(0x4B6630); }
#else
    virtual bool YRPP_STDCALL Is_Moving_Here(CoordStruct to) { return false; }
#endif
    /// VA: 0x004B6640.
#if defined(RA2_YRPP_GAME)
    virtual bool YRPP_STDCALL Will_Jump_Tracks() { JMP_STD(0x4B6640); }
#else
    virtual bool YRPP_STDCALL Will_Jump_Tracks() { return false; }
#endif
    /// VA: 0x004B4C50.
#if defined(RA2_YRPP_GAME)
    virtual bool YRPP_STDCALL Is_Really_Moving_Now() { JMP_STD(0x4B4C50); }
#else
    virtual bool YRPP_STDCALL Is_Really_Moving_Now() { return Is_Moving_Now(); }
#endif
    /// VA: 0x004B4C90.
#if defined(RA2_YRPP_GAME)
    virtual void YRPP_STDCALL Stop_Movement_Animation() { JMP_STD(0x4B4C90); }
#else
    virtual void YRPP_STDCALL Stop_Movement_Animation() {  }
#endif
    /// VA: 0x004B4CA0.
#if defined(RA2_YRPP_GAME)
    virtual void YRPP_STDCALL Limbo() { JMP_STD(0x4B4CA0); }
#else
    virtual void YRPP_STDCALL Limbo() {  }
#endif
    /// VA: 0x004B6650.
#if defined(RA2_YRPP_GAME)
    virtual void YRPP_STDCALL Lock() { JMP_STD(0x4B6650); }
#else
    virtual void YRPP_STDCALL Lock() {  }
#endif
    /// VA: 0x004B6660.
#if defined(RA2_YRPP_GAME)
    virtual void YRPP_STDCALL Unlock() { JMP_STD(0x4B6660); }
#else
    virtual void YRPP_STDCALL Unlock() {  }
#endif
    /// VA: 0x004B6670.
#if defined(RA2_YRPP_GAME)
    virtual int YRPP_STDCALL Get_Track_Number() { JMP_STD(0x4B6670); }
#else
    virtual int YRPP_STDCALL Get_Track_Number() { return -1; }
#endif
    /// VA: 0x004B6680.
#if defined(RA2_YRPP_GAME)
    virtual int YRPP_STDCALL Get_Track_Index() { JMP_STD(0x4B6680); }
#else
    virtual int YRPP_STDCALL Get_Track_Index() { return -1; }
#endif
    /// VA: 0x004B6690.
#if defined(RA2_YRPP_GAME)
    virtual int YRPP_STDCALL Get_Speed_Accum() { JMP_STD(0x4B6690); }
#else
    virtual int YRPP_STDCALL Get_Speed_Accum() { return -1; }
#endif

    // Non virtuals
    /// VA: 0x0045AF20.
    static HRESULT TryPiggyback(IPiggyback** Piggy, ILocomotion** Loco)
    { PUSH_VAR32(Loco); SET_REG32(ECX, Piggy); CALL(0x45AF20); }

    /// VA: 0x0041C250.
    static HRESULT CreateInstance(ILocomotion** ppv, const CLSID* rclsid, IUnknown* pUnkOuter, DWORD dwClsContext)
    { PUSH_VAR32(dwClsContext); PUSH_VAR32(pUnkOuter); PUSH_VAR32(rclsid); SET_REG32(ECX, ppv); CALL(0x41C250); }

    // these two are identical, why do they both exist...
    /// VA: 0x0045A170.
    static void AddRef1(LocomotionClass** Loco)
    { SET_REG32(ECX, Loco); CALL(0x45A170); }

    /// VA: 0x006CE270.
    static void AddRef2(LocomotionClass** Loco)
    { SET_REG32(ECX, Loco); CALL(0x6CE270); }

    // Legacy Microsoft COM convenience wrappers are not portable ABI entries.
#if defined(_MSC_VER)
    static void ChangeLocomotorTo(FootClass* Object, const CLSID& clsid)
    {
        // remember the current one
        ILocomotionPtr Original(Object->Locomotor);

        // create a new locomotor and link it
        auto NewLoco = CreateInstance(clsid);
        NewLoco->Link_To_Object(Object);

        // get piggy interface and piggy original
        IPiggybackPtr Piggy(NewLoco);
        Piggy->Begin_Piggyback(Original);

        // replace the current locomotor
        Object->Locomotor = NewLoco;
    }

    // creates a new instance by class ID. returns a pointer to ILocomotion
    static ILocomotionPtr CreateInstance(const CLSID& rclsid)
    {
        return ILocomotionPtr(rclsid, nullptr,
            CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER);
    }

    // finds out whether a locomotor is currently piggybacking and restores
    // the original locomotor. this function ignores Is_Ok_To_End().
    static bool End_Piggyback(ILocomotionPtr& pLoco)
    {
        if (!pLoco)
            _com_issue_error(E_POINTER);

        if (IPiggybackPtr pPiggy = pLoco)
        {
            if (pPiggy->Is_Piggybacking())
            {
                // _com_ptr_t releases the old pointer automatically,
                // so we just use it without resetting it
                auto res = pPiggy->End_Piggyback(&pLoco);
                if (FAILED(res))
                    _com_issue_error(res);

                return (res == S_OK);
            }
        }

        return false;
    }


#endif

    // Constructors
    /// VA: 0x0055A6C0.
#if defined(RA2_YRPP_GAME)
    LocomotionClass() { JMP_THIS(0x55A6C0); }
#else
    LocomotionClass() noexcept : Owner(nullptr), LinkedTo(nullptr), Powered(true), Dirty(true), RefCount(0) {}
#endif

protected:
    explicit __forceinline LocomotionClass(noinit_t) noexcept { }

    // Properties
public:

    FootClass* Owner;
    FootClass* LinkedTo;
    bool Powered;
    bool Dirty;
    int RefCount;
};

#if defined(_MSC_VER)
namespace detail
{
    template<typename Base>
    concept LocoIsDerived = std::derived_from<Base, LocomotionClass> && !std::is_same_v<LocomotionClass, Base>;

}

template<typename T>
concept LocoCastEligible = std::is_pointer_v<T> && detail::LocoIsDerived<std::remove_cvref_t<std::remove_const_t<std::remove_pointer_t<T>>>>;

template <LocoCastEligible T>
__forceinline T locomotion_cast(ILocomotion* iLoco)
{
    using Base = std::remove_cvref_t<std::remove_const_t<std::remove_pointer_t<T>>>;

    if (!iLoco) return nullptr;
    IPersist* persist = nullptr;
    const HRESULT queried = iLoco->QueryInterface(__uuidof(IPersist), reinterpret_cast<void**>(&persist));
    if (FAILED(queried) || !persist) return nullptr;
    CLSID clsid{};
    const HRESULT identified = persist->GetClassID(&clsid);
    persist->Release(); // Balance QueryInterface even when GetClassID fails.
    // Downcast the original ILocomotion subobject, allowing C++ pointer adjustment.
    return SUCCEEDED(identified) && clsid == __uuidof(Base) ? static_cast<T>(iLoco) : nullptr;
}

template<LocoCastEligible T>
__forceinline T locomotion_cast(ILocomotionPtr& comLoco)
{
    return locomotion_cast<T>(comLoco.GetInterfacePtr());
}

#endif

struct TurnTrackType
{
    char NormalTrackStructIndex;
    char ShortTrackStructIndex;
    int Face;
    int Flag;
};

struct TrackType
{
    Point2D Point;
    int Face;
};

struct RawTrackType
{
    TrackType* TrackPoint;
    int JumpIndex;
    int EntryIndex;
    int CellIndex;
};

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"

// this refers to the "planning mode" waypoints you place with your mouse, not mapping waypoints
class WaypointClass
{
public:
    // need to define a == operator so it can be used in array classes
    bool operator == (const WaypointClass& tWaypoint) const
    {
        return Coords == tWaypoint.Coords;
    }

    // Properties
    // Original stride is 0xC: lepton X/Y/Z, not CellStruct plus an unknown word.
    CoordStruct Coords;
};

class NOVTABLE WaypointPathClass : public AbstractClass
{
public:
    static const AbstractType AbsID = AbstractType::Waypoint;
    /// Global VA: 0x00B72608.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<WaypointPathClass*>,Array,0xB72608u)
#else
    static DynamicVectorClass<WaypointPathClass*>& Array;
#endif

    // IPersist
    /// VA: 0x00763C30
    HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    // IPersistStream
    /// VA: 0x00763C70
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: 0x00763D90
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) R0;

    // Destructor
    // Scalar deleting wrapper; native C++ supplies the allocator wrapper.
    /// VA: 0x00763E20
    ~WaypointPathClass() override;

    // AbstractClass
    /// VA: 0x00763E10
    AbstractType WhatAmI() const override {return AbsID;}
    /// VA: 0x00763E00
    int Size() const override {return sizeof(*this);}
    /// VA: 0x00763C00
    void ComputeCRC(CRCEngine& crc) const override;

    /// VA: 0x00763980.
    WaypointClass* GetWaypoint(int idx) const
        { return idx>=0 && idx<Waypoints.Count?Waypoints.Items+idx:nullptr; }
    /// VA: 0x00763BA0.
    WaypointClass* GetWaypointAfter(const WaypointClass* waypoint) const
        { int index=Waypoints.GetItemIndex(waypoint)+1;
          if(CurrentWaypointIndex!=-1 && index==Waypoints.Count)index=CurrentWaypointIndex;
          return GetWaypoint(index); }
    /// VA: 0x00763A50.
    bool WaypointExistsAt(const WaypointClass* waypoint)
        { for(int i=0;i<Waypoints.Count;++i)
            if(short(Waypoints[i].Coords.X/256)==short(waypoint->Coords.X/256)
                && short(Waypoints[i].Coords.Y/256)==short(waypoint->Coords.Y/256)) {
                CurrentWaypointIndex=i;return true;
            }
          return false; }

    // Constructor
    /// VA: 0x00763810.
    explicit WaypointPathClass(int idx) noexcept;

protected:
    explicit __forceinline WaypointPathClass(noinit_t)
        : AbstractClass(noinit_t())
    { }

    // Properties

public:

    int  CurrentWaypointIndex; //seems that way
    DynamicVectorClass<WaypointClass> Waypoints; // actual path waypoints, no *
};

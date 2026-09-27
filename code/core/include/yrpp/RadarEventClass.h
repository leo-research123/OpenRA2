/*
    Radar notifications
*/

#pragma once

#include "yrpp/GeneralDefinitions.h"
#include "yrpp/ArrayClasses.h"
#include "yrpp/Timer.h"

class RadarEventClass
{
public:
    static DynamicVectorClass<RadarEventClass*>& Array; // B04DA8.
    static CellStruct (&History)[8]; // B04D48, last-event camera locations.
    static int& HistoryIndex; // B04D88.
    static int& HistoryCursor; // B04DD8.
    // Callable C++ wrapper for 65FA70's custom ECX + stack convention.
    // Rejects invalid types/allocation failure; no exception crosses this entry.
    static bool Create(RadarEventType nType, CellStruct nMapCoords) noexcept;
    static void Clear() noexcept; // 65FD50.
    /// VA: 0x0065FDD0
    static bool UpdateAll() noexcept;
    /// VA: 0x006603B0
    static void RemoveFinished() noexcept;
    void Update() noexcept; // 65FE00, frame timers use Unsorted::CurrentFrame.
    bool GetVertices(Point2D* output, unsigned count) const noexcept; // 660730.
    void Draw() const noexcept; // 660050 via DSurface's generic gradient path.
    static void DrawAll() noexcept; // 660000.

private:
    // Constructor, Destructor
    /// VA: 0x0065FB80.
    RadarEventClass(RadarEventType nType, CellStruct nMapCoords) noexcept;

    /// VA: 0x0065FD00. The old 65B2F0 declaration pointed to RadSiteClass.
    ~RadarEventClass();

    // Properties

public:

    RadarEventType Type;
    int RadarX;	//not sure
    int RadarY;	//not sure
    float Speed; // Original +0C is current radius, despite the inherited name.
    float RotationValue;
    float RotationSpeed;
    float ColorValue;
    float ColorSpeed;
    CellStruct MapCoords;
    CDTimerClass DurationTimer;	//Rotation timer?
    CDTimerClass VisibilityTimer;	//Color timer?
    bool Rotating;
    bool Visible;
};

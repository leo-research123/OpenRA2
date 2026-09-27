#pragma once

#include "yrpp/platform/ABI.h"

// Reverse engineering and coding by: CrimRecya

#include "yrpp/ArrayClasses.h"
#include "yrpp/GeneralStructures.h"
#include "yrpp/FootClass.h"
#include "yrpp/PriorityQueueClass.h"

class TechnoClass;
struct AStarClass_PathNodeBuffer;
struct AStarClass_PathQueueBuffer;
struct AStarClass_HierarchicalBuffer;
struct AStarClass_PathQueueNode;
struct AStarClass_HierarchicalNode;
using PriorityQueueClass_PathQueueNode = PriorityQueueClass<AStarClass_PathQueueNode>;
using PriorityQueueClass_HierarchicalNode = PriorityQueueClass<AStarClass_HierarchicalNode>;

struct AStarClass_PassabilityData
{
    unsigned short Indices[500];
};
static_assert(sizeof(AStarClass_PassabilityData) == 0x3E8);

class PathFinderData
{
public:
    CellStruct StartCell;
    int TotalDistance;
    int PathLength;
    int* Directions;
    int unknown_int_10;
    int* Levels;
    CellStruct unknown_cellstruct_18;
    int unknown_int_1C;
};
#if UINTPTR_MAX == UINT32_MAX
static_assert(sizeof(PathFinderData) == 0x20);
#endif

class AStarClass
{
public:
    // Static
    /// Global VA: 0x0087E8B8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(AStarClass, Instance, 0x87E8B8u)
#else
    static AStarClass& Instance;
#endif

    /// VA: 0x0042A6D0
    AStarClass() noexcept;
    /// VA: 0x0042A900
    ~AStarClass();
    AStarClass(const AStarClass&) = delete;
    AStarClass& operator=(const AStarClass&) = delete;
    /// VA: 0x0042A5B0
    void Clear();
    /// VA: 0x0042AC00
#if defined(RA2_YRPP_GAME)
    void UpdateMapDimensions(const RectangleStruct& dimensions) { JMP_THIS(0x42AC00); }
#else
    void UpdateMapDimensions(const RectangleStruct& dimensions);
#endif
    /// VA: 0x0042C1C0
    void Reset();
    /// VA: 0x0042C290
    bool FindPathHierarchical(const CellStruct& start, const CellStruct& end, MovementZone movement, const FootClass* foot);
    /// VA: 0x0042CCD0
    void BanBlockedSubzoneEdges(const FootClass* foot);
    /// VA: 0x0042CF80
    void BanNeighbourhoodSubzoneEdges(int subzone, int level);
    /// VA: 0x00429A90
    PathFinderData* FindPathRegular(const CellStruct& start, const CellStruct& end, FootClass* foot,
        int* directions, int maxSteps, bool hierarchical);

    /// VA: 0x00429830
    double GetMovementCost(CellClass** from, CellClass** to, bool bridge, Move move, FootClass* foot);
    /// VA: 0x0042ACF0
    void ApplyPathCollisionAvoidance(FootClass* foot);
    /// VA: 0x0042B080
    static FootClass* YRPP_STDCALL FindMovingBlocker(const CellStruct& cell, int level);
    /// VA: 0x00429780
    static CellStruct* YRPP_FASTCALL FollowPath(CellStruct* output, const CellStruct* start, int count, const int* directions);

    /// VA: 0x0042B210
    void CutCorners(PathFinderData* path, FootClass* foot);
    /// VA: 0x0042B420
    static int YRPP_STDCALL TryDiagonalShortcut(FootClass* foot, int* directions, int* levels,
        int firstLength, int secondLength, CellStruct& cell);
    /// VA: 0x0042B7F0
    void OptimizeMoves(PathFinderData* path, FootClass* foot);
    /// VA: 0x0042BCA0
    static void YRPP_STDCALL SplicePath(int* directions, int start, int end, int& spliceIndex, CellStruct& cell);
    /// VA: 0x0042BE20
    static bool YRPP_STDCALL PlotStraightLine(int* directions, int count, const CellStruct& from,
        const CellStruct& delta, FootClass* foot, int level, bool fearless);

private:
    /// VA: 0x0042A460
    AStarClass_PathQueueNode* CreatePathNode(const AStarClass_PathQueueNode* parent, CellClass** cell,
        const CellStruct& destination, float cost);
    /// VA: 0x0042AA90
    static PathFinderData* YRPP_STDCALL BuildFinalPath(const AStarClass_PathQueueNode* node, int* directions);
public:

    /// VA: 0x0042D170.
    int AttemptPath(
        CellStruct* pFromMapCrd,
        CellStruct* pToMapCrd,
        FootClass* pFoot,
        bool bFromAlt,
        bool bToAlt,
        MovementZone nMovementZone = MovementZone::None);

    /// VA: 0x0042C900.
    PathFinderData* FindPath(CellStruct* start, CellStruct* end, FootClass* foot, int* directions,
        int maxSteps, MovementZone movement, int mode);

    char unknown_byte_0;
    bool FindBridgeDir;
    char unknown_byte_2;
    bool CanFindPath; // OpenTS IsAvoidPathCollision, not a path-success flag
    float PathCostFactor;
    bool IsAlt;
    PROTECTED_PROPERTY(BYTE, padding_9_B[3]);
    AStarClass_PathNodeBuffer* PathNodeBuffer;
    AStarClass_PathQueueBuffer* PathQueueBuffer;
    PriorityQueueClass_PathQueueNode* PathQueue;
    int* VisitCounts;
    int* AltVisitCounts;
    float* AltDistances;
    float* Distances;
    int SearchID;
    SpeedType FinderSpeedType;
    int StartLevel;
    int EndLevel;
    bool IsSearching; // OpenTS IsHSEnabled: permits hierarchical retries
    PROTECTED_PROPERTY(BYTE, padding_39_3B[3]);
    int FindMode;
    int* LevelVisitedMarkers[3];
    int* OpenSetMarkers[3];
    float* GCostArray[3];
    AStarClass_HierarchicalBuffer* HierarchyBuffer;
    PriorityQueueClass_HierarchicalNode* HierarchyQueue;
    int PathLength;
    CellStruct CellStructBuffer;
    DynamicVectorClass<unsigned int> ZoneIndices[3];
    AStarClass_PassabilityData PassabilityData[3];
    int PassabilityCounts[3];
};

#if UINTPTR_MAX == UINT32_MAX
static_assert(sizeof(AStarClass) == 0xC80);
#endif

// Original direction table at 0x89F688, in clockwise order N..NW.
#include "yrpp/Unsorted.h"
namespace {CellStruct cells[8]{{0,-1},{1,-1},{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1}};}
CellStruct (&Unsorted::AdjacentCell)[8]=cells;
namespace {
Point2D coords[8]{{0,-256},{256,-256},{256,0},{256,256},{0,256},{-256,256},{-256,0},{-256,-256}};
}
Point2D (&Unsorted::AdjacentCoord)[8]=coords;

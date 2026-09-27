// Standalone Theater data; original-game definitions are supplied by compat.
#include "yrpp/Theater.h"
namespace {
const Theater theaters[6] = {
    {"TEMPERATE", "Name:Temperate", "TEMPERAT", "ISOTEMP", "ISOTEM", "TEM", "MMT", "T", 1.0f, 1.600000023841858f, 0.0f, 1.875f, -1, 0},
    {"SNOW", "Name:Snow", "SNOW", "ISOSNOW", "ISOSNO", "SNO", "MMS", "A", 0.800000011920929f, 1.100000023841858f, 0.0f, 1.8125f, -1, 0},
    {"URBAN", "Name:Urban", "URBAN", "ISOURB", "ISOURB", "URB", "MMU", "U", 1.0f, 1.600000023841858f, 0.0f, 1.875f, 0, 0},
    {"DESERT", "Name:Desert", "DESERT", "ISODES", "ISODES", "DES", "MMD", "D", 1.0f, 1.600000023841858f, 0.0f, 1.875f, -1, 0},
    {"NEWURBAN", "Name:New Urban", "URBANN", "ISOUBN", "ISOUBN", "UBN", "MMT", "N", 1.0f, 1.600000023841858f, 0.0f, 1.875f, -1, 0},
    {"LUNAR", "Name:Lunar", "LUNAR", "ISOLUN", "ISOLUN", "LUN", "MML", "L", 1.0f, 1.600000023841858f, 0.0f, 1.875f, -1, 0},
};
TheaterType last_theater = TheaterType::None;
}
Theater const (&Theater::Array)[6] = theaters;
TheaterType& Theater::LastTheater = last_theater;

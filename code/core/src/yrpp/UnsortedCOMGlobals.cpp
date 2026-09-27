#include "yrpp/Unsorted.h"
// YR 0xABCD3C is shared by Bullet, Locomotion and other COM classes.
namespace { LONG references=0; }
LONG& Game::COMReferenceCount=references;

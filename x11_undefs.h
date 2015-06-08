#ifndef X11_UNDEFS_H
#define X11_UNDEFS_H

#include <Qt>

// X11 defines some stuff that gets in the way
#ifdef Status
#undef Status
#endif

#ifdef Unsorted
#undef Unsorted
#endif

#ifdef None
#undef None
#endif

#ifdef Bool
#undef Bool
#endif

#ifdef CursorShape
#undef CursorShape
#endif

#endif // X11_UNDEFS_H


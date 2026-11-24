// SPDX-License-Identifier: GPL-3.0-or-later
// <stats.h> for the native rebuild units (lane P1). conquer.cpp, mainloop.cpp and queue.cpp include
// "stats.h"; the rebuild has no such header, so VC6 found the Visual C++ 6 SDK's STATS.H (the MAPI
// folder-metrics library), of which the units use nothing. Natively the include is empty.
#ifndef RB_NATIVE_STATS_H
#define RB_NATIVE_STATS_H
#endif

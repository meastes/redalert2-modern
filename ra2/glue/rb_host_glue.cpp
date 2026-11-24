// SPDX-License-Identifier: GPL-3.0-or-later
// The RA2 game's hooks the shared host calls (native/platform/sdl/main.cpp), for the trial game
// on the rebuild tree (lane P2, branch ports-on-rebuild). Compiled by native/rb/tools/rb_trial.sh
// and native/rb/CMakeLists.txt beside the generated units; not appended to any unit.
//
// After the port has filled the initialised globals from game.exe (main.cpp, before WinMain): the
// units' static initialisers, which native/yr/tools/defer_initializers.py moved out of dyld's list
// into __DATA,__rb_init_func, in link order (the units' order: the original _initterm order,
// native/rb/data/initterm.json), as the original's _initterm ran them with its data in place.
// The same mechanism as yr-native's (native/yr/glue/yr_host_glue.cpp).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __MINGW32__
// The x86_64 Windows build (lane P13, as yr_host_glue.cpp's): defer_initializers.py --coff-section .rbini$m renamed the
// units' .ctors sections; lld groups them in link order between these two markers (sorted by the part after the $).
typedef void (*RbInitialiser)(void);
__attribute__((section(".rbini$a"), used)) static RbInitialiser rb_init_first = 0;
__attribute__((section(".rbini$z"), used)) static RbInitialiser rb_init_last = 0;

void ra2_native_original_records()
{
    unsigned long count = 0;
    RbInitialiser *volatile first = &rb_init_first;   // through volatile: the compiler may not reason about two objects' order
    RbInitialiser *volatile last = &rb_init_last;
    for (RbInitialiser *p = first + 1; p < last; ++p) {
        if (*p) {
            (*p)();
            ++count;
        }
    }
    fprintf(stderr, "rb-native: %lu static initialisers run after the original data\n", count);
}
#else
#include <dlfcn.h>
#include <mach-o/getsect.h>

void ra2_native_original_records()
{
    unsigned long size = 0;
    Dl_info self = {};
    dladdr((const void *)&ra2_native_original_records, &self);
    const struct mach_header_64 *image = (const struct mach_header_64 *)self.dli_fbase;
    uint8_t *start = getsectiondata(image, "__DATA", "__rb_init_func", &size);
    typedef void (*Initialiser)(void);
    Initialiser *list = (Initialiser *)start;
    unsigned long count = size / sizeof(Initialiser);
    bool trace = getenv("RA2_NATIVE_INIT_TRACE") != nullptr;
    for (unsigned long i = 0; i < count; ++i) {
        if (trace) {
            Dl_info at = {};
            dladdr((const void *)list[i], &at);
            fprintf(stderr, "rb-native: initialiser %lu %s\n", i, at.dli_sname ? at.dli_sname : "?");
        }
        list[i]();
    }
    fprintf(stderr, "rb-native: %lu static initialisers run after the original data\n", count);
}
#endif

// SPDX-License-Identifier: GPL-3.0-or-later
// <mbstring.h> for the native rebuild units (lane P1): the one multibyte-string call the rebuild
// makes (ownrdraw.cpp, the edit-control caret: _mbsnbcnt), which the port's C runtime defines
// (native/port/src/crt.cpp). Searched ahead of native/yr/sdk's, which declares YR's _mbsnccnt.
#ifndef RB_NATIVE_MBSTRING_H
#define RB_NATIVE_MBSTRING_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
size_t _mbsnbcnt(const unsigned char *s, size_t n);
#ifdef __cplusplus
}
#endif
#endif

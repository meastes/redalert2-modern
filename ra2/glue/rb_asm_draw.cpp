// SPDX-License-Identifier: GPL-3.0-or-later
// Lane P2 (branch ports-on-rebuild): the spot-light brighteners of main's native/link/behaviour_asm_draw.cpp,
// carried over unchanged (the rebuild declares the same C names: ovrlight.cpp). The 2x interpolating
// copies stay out until a run reaches them (the rebuild declares Asm_Interpolate with C++ linkage).
// NOT EXACT; never part of an exact build.
//
// Behaviour rewrites of the drawing helpers written in assembly (winasm.asm):
// the spot-light brighteners and the 2x interpolating copies. For the native
// tree only. NOT EXACT and not original code: portable C++ written from the
// adopted TibSun archive procedures in src/recovered/asm/winasm.asm (each one
// proven byte-identical to the pinned target in
// manifests/original_target/assembly.json; TibSun archive text, GPL-3.0-or-later
// with EA's section 7 terms, THIRD_PARTY.md). The per-format constants below
// are the BRIGHTEN_COLOR / MMX_BRIGHTEN_COLOR macro constants of that file.
// Tests: native/tests/asm_check.cpp (native-asm-check). Manifest:
// native/behaviour_rewrites.json.
#include <stdint.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Brighten_Color_565 0x0079813A, _655 0x00798217, _556 0x007982F4, _555
// 0x007983D1. For every pixel whose multiplier byte s is nonzero, each colour
// component c (expanded to 8 bits as the format's top bits) becomes
// min(255, c + (c * s >> 8)), then is packed back. Rows: the multiplier buffer
// advances by mulbuff_width bytes, the colour buffer by color_buff_width
// bytes (both pitches). Width and height count down to zero (callers pass at
// least 1; zero returns here, the original would wrap).
// ---------------------------------------------------------------------------
namespace {
struct BrightenFormat {
    // CONSTANT_0..12 of BRIGHTEN_COLOR (CONSTANT_4/5/11 are 8 in every format).
    int c0, c1, c2, c3, c6, c7, c8, c9, c10, c12;
};
BrightenFormat const Brighten565 = {8, 3, 0xF8, 0xFC, 3, 2, 11, 5, 3, 3};
BrightenFormat const Brighten655 = {8, 2, 0xFC, 0xF8, 2, 3, 10, 5, 3, 3};
BrightenFormat const Brighten556 = {8, 3, 0xF8, 0xF8, 3, 3, 11, 6, 2, 2};
BrightenFormat const Brighten555 = {7, 2, 0xF8, 0xF8, 3, 3, 10, 5, 3, 3};

inline uint32_t brighten_component(uint32_t c, uint32_t s)
{
    uint32_t v = c + ((c * s) >> 8);   // add bl,dl / setnb / dec / or: saturate the byte
    return v > 255 ? 255 : v;
}

void brighten(BrightenFormat const &f, void *sptr, void *dptr, int spitch, int dpitch, int width, int height)
{
    if (width == 0 || height == 0) return;
    unsigned char *srow = (unsigned char *)sptr;
    unsigned char *drow = (unsigned char *)dptr;
    for (uint32_t y = (uint32_t)height; y != 0; --y, srow += spitch, drow += dpitch) {
        for (uint32_t x = 0; x != (uint32_t)width; ++x) {
            uint32_t s = srow[x];
            if (s == 0) continue;
            uint16_t pixel;
            memcpy(&pixel, drow + 2 * x, 2);
            uint32_t p = pixel;
            uint32_t r = brighten_component((p >> f.c0) & (uint32_t)f.c2, s);
            uint32_t g = brighten_component((p >> f.c1) & (uint32_t)f.c3, s);
            uint32_t b = brighten_component((p << f.c10) & 0xFF, s);
            uint16_t out = (uint16_t)(((r >> f.c6) << f.c8) | ((g >> f.c7) << f.c9) | (b >> f.c12));
            memcpy(drow + 2 * x, &out, 2);
        }
    }
}

// ---------------------------------------------------------------------------
// MMX_Brighten_Color_565 0x007984C0, _555 0x00798602, _556 0x00798740, _655
// 0x00798880. The same brightening through a caller-built table of 65,536
// dwords (SpotLight's: R << 16 | G << 8 | B per 16-bit pixel): punpcklbw +
// psrlw 8 widens the entry's bytes B, G, R, 0 to words, pmullw/psrlw 8 scales
// by s (the fourth word by 0), paddusb adds the original byte with unsigned
// saturation, psrlw shifts each word by CONSTANT_2; then B >> CONSTANT_3,
// G << CONSTANT_4, R << CONSTANT_5 are combined. Formats other than 655 mask
// R with CONSTANT_6 << 8; 655 instead masks G with 0x423A0A60 (the macro's
// constant, kept as is; the archive procedure matches the target).
// ---------------------------------------------------------------------------
struct MMXFormat {
    int c2, c3, c4, c5;
    uint32_t c6;
};
MMXFormat const MMX565 = {2, 1, 5, 10, 0xF8};
MMXFormat const MMX555 = {3, 0, 5, 10, 0x7C};
MMXFormat const MMX556 = {2, 0, 6, 10, 0xF8};
MMXFormat const MMX655 = {2, 1, 4, 10, 0x423A0A60};

void mmx_brighten(MMXFormat const &f, void *sptr, void *dptr, int spitch, int dpitch, int width, int height,
                  void *table)
{
    if (width == 0 || height == 0) return;
    unsigned char const *entries = (unsigned char const *)table;
    unsigned char *srow = (unsigned char *)sptr;
    unsigned char *drow = (unsigned char *)dptr;
    for (uint32_t y = (uint32_t)height; y != 0; --y, srow += spitch, drow += dpitch) {
        for (uint32_t x = 0; x != (uint32_t)width; ++x) {
            uint32_t s = srow[x];
            if (s == 0) continue;
            uint16_t pixel;
            memcpy(&pixel, drow + 2 * x, 2);
            unsigned char const *e = entries + 4 * (uint32_t)pixel;
            uint32_t w[3];
            for (int k = 0; k < 3; ++k) {
                uint32_t c = e[k];
                uint32_t t = ((c * s) & 0xFFFF) >> 8;
                uint32_t v = t + c;            // paddusb on the low byte (high bytes are 0)
                w[k] = (v > 255 ? 255 : v) >> f.c2;
            }
            uint32_t ebx = (w[0] & 0xFF) >> f.c3;
            uint32_t eax = (w[1] & 0xFF) << f.c4;
            uint32_t ecx = (w[2] & 0xFF) << f.c5;
            if (f.c6 == 0x423A0A60) {
                ebx |= eax & f.c6;
            } else {
                ebx |= eax;
                ecx &= f.c6 << 8;
            }
            ebx |= ecx;
            uint16_t out = (uint16_t)ebx;
            memcpy(drow + 2 * x, &out, 2);
        }
    }
}
}

extern "C" {
void Brighten_Color_555(void *sptr, void *dptr, int spitch, int dpitch, int width, int height)
{ brighten(Brighten555, sptr, dptr, spitch, dpitch, width, height); }
void Brighten_Color_556(void *sptr, void *dptr, int spitch, int dpitch, int width, int height)
{ brighten(Brighten556, sptr, dptr, spitch, dpitch, width, height); }
void Brighten_Color_565(void *sptr, void *dptr, int spitch, int dpitch, int width, int height)
{ brighten(Brighten565, sptr, dptr, spitch, dpitch, width, height); }
void Brighten_Color_655(void *sptr, void *dptr, int spitch, int dpitch, int width, int height)
{ brighten(Brighten655, sptr, dptr, spitch, dpitch, width, height); }
void MMX_Brighten_Color_555(void *sptr, void *dptr, int spitch, int dpitch, int width, int height, void *buffer)
{ mmx_brighten(MMX555, sptr, dptr, spitch, dpitch, width, height, buffer); }
void MMX_Brighten_Color_556(void *sptr, void *dptr, int spitch, int dpitch, int width, int height, void *buffer)
{ mmx_brighten(MMX556, sptr, dptr, spitch, dpitch, width, height, buffer); }
void MMX_Brighten_Color_565(void *sptr, void *dptr, int spitch, int dpitch, int width, int height, void *buffer)
{ mmx_brighten(MMX565, sptr, dptr, spitch, dpitch, width, height, buffer); }
void MMX_Brighten_Color_655(void *sptr, void *dptr, int spitch, int dpitch, int width, int height, void *buffer)
{ mmx_brighten(MMX655, sptr, dptr, spitch, dpitch, width, height, buffer); }
}


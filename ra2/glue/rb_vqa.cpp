// SPDX-License-Identifier: GPL-3.0-or-later
// Lane P3 (branch ports-on-rebuild): the VQA player's assembly helpers and LCW_Comp, main's native
// bodies carried over under the rebuild's names (item 4 overlay; item 3 replaces them with the renamed
// link/): native/link/behaviour_asm_vqa.cpp (AudioUnzap, VQA_LCW_Uncompress, the SOS codec, the
// ASM_UnVQ drawers; main's VQA_Unknown_0077FD80/0077FE89 are the rebuild's ASM_UnVQ1_C1_TABLE and
// ASM_UnVQ1_C1_TABLE_ALT, the catalog's 0x0077FD80/0x0077FE89), native/link/behaviour_c16_vqa.cpp
// (main's VQA_Unknown_0070C0A0, the rebuild's UnVQ1_4x2_Table 0x0070C0A0) and
// native/link/behaviour_asm_lib.cpp (LCW_Comp 0x00534D40). The only changes: the rebuild's
// declarations are restated here (this file sees neither tree's headers), the SOS record with 32-bit
// fields (the rebuild's _VQA_SOS_COMPRESS_INFO). NOT EXACT; never part of an exact build.
// The original header of the carried text follows.
//
#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef int32_t RA2_LONG;
typedef uint32_t RA2_ULONG;
struct VQASOS {                     // the rebuild's _VQA_SOS_COMPRESS_INFO, 12 bytes on the target
    RA2_LONG Predicted;
    short Index;
    RA2_LONG Predicted2;
    short Index2;
};
// Behaviour rewrites of the VQA player's handwritten-assembly helpers, for the
// native tree only. NOT EXACT and not original code: portable C++ written from
// the adopted TibSun archive procedures in src/recovered/asm (vqa_uncomp.asm,
// vqa_sos.asm with difftb.inc/indextb.inc/nybbtb.inc, unvq_asm.asm; each one
// proven byte-identical to the pinned target in
// manifests/original_target/assembly.json). TibSun archive text is GPL-3.0-or-
// later with EA's section 7 terms (THIRD_PARTY.md). The exact build keeps the
// assembled procedures; these bodies replace the link-closure traps.
// Tests: native/tests/asm_check.cpp (native-asm-check). Manifest:
// native/behaviour_rewrites.json.

// ---------------------------------------------------------------------------
// AudioUnzap 0x00781130 (vqa_uncomp.asm). Westwood "zap" 8-bit delta audio.
// Each code byte: top two bits select the code, low six bits are its data.
//   0 (2-bit): data+1 bytes follow, four 2-bit deltas each (bits 0-1 first),
//              table {-2,-1,0,1}, every sample clamped to 0..255.
//   1 (4-bit): data+1 bytes follow, two 4-bit deltas each (low nibble first),
//              table {-9,-8,-6,-5,-4,-3,-2,-1,0,1,2,3,4,5,6,8}, clamped.
//   2 (raw):   bit 5 set: the low five bits are a signed delta added without
//              clamping (8-bit wrap), one sample; bit 5 clear: data+1 raw
//              samples follow and the last one becomes the previous sample.
//   3 (zero):  data+1 copies of the previous sample.
// The previous sample starts at 0x80. The loop runs while the remaining count
// (signed, decremented by each code's output) is positive; a code may write
// past the count, as the original does. Returns the number of SOURCE bytes
// read (the asm returns its incount local, not the output size).
// ---------------------------------------------------------------------------
namespace {
signed char const Zap2Bit[4] = {-2, -1, 0, 1};
signed char const Zap4Bit[16] = {-9, -8, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 8};

inline uint8_t zap_clamp(uint8_t prev, signed char delta)
{
    int v = (int)prev + delta;
    return (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v));
}
}

extern "C" RA2_LONG AudioUnzap(void *source, void *dest, RA2_LONG size)
{
    if (source == 0 || dest == 0 || size == 0) return 0;
    uint8_t const *src = (uint8_t const *)source;
    uint8_t *dst = (uint8_t *)dest;
    int32_t count = size;
    int32_t incount = 0;
    uint8_t prev = 0x80;
    while (count > 0) {
        uint8_t code = *src++;
        incount++;
        unsigned kind = code >> 6;
        unsigned data = code & 0x3F;
        if (kind == 2) {
            if (data & 0x20) {
                prev = (uint8_t)(prev + (int)((data & 0x1F) ^ 0x10) - 0x10); // shl al,3 / sar al,3
                *dst++ = prev;
                count--;
            } else {
                unsigned n = data + 1;
                memcpy(dst, src, n);
                src += n;
                dst += n;
                incount += (int32_t)n;
                count -= (int32_t)n;
                prev = dst[-1];
            }
        } else if (kind == 1) {
            for (unsigned n = data + 1; n != 0; --n) {
                uint8_t b = *src++;
                incount++;
                uint8_t s1 = zap_clamp(prev, Zap4Bit[b & 0x0F]);
                uint8_t s2 = zap_clamp(s1, Zap4Bit[b >> 4]);
                dst[0] = s1;
                dst[1] = s2;
                dst += 2;
                count -= 2;
                prev = s2;
            }
        } else if (kind == 0) {
            for (unsigned n = data + 1; n != 0; --n) {
                uint8_t b = *src++;
                incount++;
                for (int k = 0; k < 4; ++k) {
                    prev = zap_clamp(prev, Zap2Bit[(b >> (2 * k)) & 3]);
                    dst[k] = prev;
                }
                dst += 4;
                count -= 4;
            }
        } else {
            unsigned n = data + 1;
            memset(dst, prev, n);
            dst += n;
            count -= (int32_t)n;
        }
    }
    return incount;
}

// ---------------------------------------------------------------------------
// VQA_LCW_Uncompress 0x007813EB (vqa_uncomp.asm). LCW (format 80) with output
// clamped to length. A leading 0 byte selects relative mode (skipped): long
// and medium copies then count their offset back from the output position
// instead of from the start of the output.
//   0cccoooo oooooooo  copy c+3 bytes from (dest - o)
//   10cccccc ...       copy c literal bytes (0x80 alone ends the data)
//   11cccccc w         copy c+3 bytes from start+w (relative: dest-w)
//   11111110 w b       fill w bytes with b
//   11111111 w1 w2     copy w1 bytes from start+w2 (relative: dest-w2)
// Every count is clamped to the remaining output; copies run forward byte by
// byte (overlapping copies repeat the pattern). Returns bytes written.
// ---------------------------------------------------------------------------
extern "C" RA2_ULONG VQA_LCW_Uncompress(char const *source, char *dest, RA2_ULONG length)
{
    uint8_t const *src = (uint8_t const *)source;
    uint8_t *dst = (uint8_t *)dest;
    uint8_t *const first = dst;
    bool relative = (*src == 0);
    if (relative) src++;
    uint8_t *const last = dst + length;
    for (;;) {
        uint32_t maxlen = (uint32_t)(last - dst);
        if (maxlen == 0) break;
        uint8_t code = *src++;
        uint32_t count;
        uint8_t const *from;
        if (!(code & 0x80)) {
            count = (uint32_t)(code >> 4) + 3;
            uint32_t off = ((uint32_t)(code & 0x0F) << 8) | *src++;
            from = dst - off;
        } else if (!(code & 0x40)) {
            if (code == 0x80) break;
            count = code & 0x3F;
            if (count > maxlen) count = maxlen;
            memcpy(dst, src, count);
            src += count;
            dst += count;
            continue;
        } else if (code == 0xFE) {
            count = (uint32_t)src[0] | ((uint32_t)src[1] << 8);
            uint8_t value = src[2];
            src += 3;
            if (count > maxlen) count = maxlen;
            memset(dst, value, count);
            dst += count;
            continue;
        } else {
            count = (uint32_t)(code & 0x3F) + 3;
            if (code == 0xFF) {
                count = (uint32_t)src[0] | ((uint32_t)src[1] << 8);
                src += 2;
            }
            uint32_t off = (uint32_t)src[0] | ((uint32_t)src[1] << 8);
            src += 2;
            from = relative ? dst - off : first + off;
        }
        if (count > maxlen) count = maxlen;
        while (count--) *dst++ = *from++;
    }
    return (RA2_ULONG)(dst - first);
}

// ---------------------------------------------------------------------------
// VQA_sosCODECInitStream 0x00780560 / VQA_sosCODECDecompressData 0x00780581
// (vqa_sos.asm): HMI SOS 4:1 ADPCM, only 16-bit mono and 16-bit stereo are
// implemented by the original (any other bits/channels returns untouched).
// The tables (difftb.inc, indextb.inc) are the IMA ADPCM step tables laid out
// as [89 step indices][16 tokens]: diff = step/8 (+step if bit 2, +step/2 if
// bit 1, +step/4 if bit 0, negated if bit 3), next index = clamp(index +
// {-1,-1,-1,-1,2,4,6,8}[token & 7], 0, 88). They are generated here; the test
// compares them entry by entry with the adopted .inc text. The state keeps the
// index as the asm's byte offset into the word table (index * 32).
// Mono: size/2 samples from consecutive nybbles (low nybble first) into
// consecutive 16-bit samples, each clamped to -32768..32767 after adding.
// Stereo: size/4 samples per channel; left reads the first size/8 source
// bytes and writes even samples with Predicted/Index, right reads the next
// size/8 bytes and writes odd samples with Predicted2/Index2. The original's
// alignment prologue and unrolled tails only change how the same nybbles are
// fetched.
// ---------------------------------------------------------------------------
namespace {
struct SosTables {
    int32_t Diff[89 * 16];
    uint16_t Next[89 * 16];
    SosTables()
    {
        static short const steps[89] = {
            7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66,
            73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408,
            449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
            2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630,
            9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
            32767};
        static signed char const adjust[8] = {-1, -1, -1, -1, 2, 4, 6, 8};
        for (int i = 0; i < 89; ++i) {
            for (int t = 0; t < 16; ++t) {
                int step = steps[i];
                int d = step >> 3;
                if (t & 4) d += step;
                if (t & 2) d += step >> 1;
                if (t & 1) d += step >> 2;
                Diff[i * 16 + t] = (t & 8) ? -d : d;
                int n = i + adjust[t & 7];
                n = n < 0 ? 0 : (n > 88 ? 88 : n);
                Next[i * 16 + t] = (uint16_t)(n * 32);
            }
        }
    }
};

SosTables const &sos_tables()
{
    static SosTables const tables;
    return tables;
}

void sos_decode_channel(uint8_t const *src, uint8_t *dst, int stride, uint32_t samples, RA2_LONG &predicted,
                        short &index)
{
    SosTables const &t = sos_tables();
    uint32_t pred = (uint32_t)predicted;
    uint32_t idx = (uint16_t)index;       // the asm's dx: index * 32
    for (uint32_t i = 0; i < samples; ++i) {
        unsigned token = (src[i >> 1] >> ((i & 1) * 4)) & 0x0F;
        uint32_t e = (idx | (token * 2)) >> 1; // table element (index * 16 + token)
        pred += (uint32_t)t.Diff[e];
        idx = t.Next[e];
        int32_t s = (int32_t)pred;
        if (s > 0x7FFF) pred = 0x7FFF;
        else if (s < -0x8000) pred = (uint32_t)-0x8000;
        uint16_t out = (uint16_t)pred;
        memcpy(dst, &out, 2);
        dst += stride;
    }
    predicted = (RA2_LONG)pred;
    index = (short)idx;
}
}

extern "C" void VQA_sosCODECInitStream(VQASOS *info)
{
    info->Index = 0;
    info->Predicted = 0;
    info->Index2 = 0;
    info->Predicted2 = 0;
}

extern "C" void VQA_sosCODECDecompressData(void *source, void *dest, unsigned short bits, unsigned short channels,
                                           RA2_ULONG size, VQASOS *info)
{
    if (bits != 16) return;
    uint8_t const *src = (uint8_t const *)source;
    uint8_t *dst = (uint8_t *)dest;
    RA2_LONG pred;
    short idx;
    if (channels == 1) {
        pred = info->Predicted; idx = info->Index;
        sos_decode_channel(src, dst, 2, (uint32_t)size >> 1, pred, idx);
        info->Predicted = pred; info->Index = idx;
    } else if (channels == 2) {
        uint32_t per_channel = ((uint32_t)size >> 1) >> 1;
        pred = info->Predicted; idx = info->Index;
        sos_decode_channel(src, dst, 4, per_channel, pred, idx);
        info->Predicted = pred; info->Index = idx;
        pred = info->Predicted2; idx = info->Index2;
        sos_decode_channel(src + ((uint32_t)size >> 3), dst + 2, 4, per_channel, pred, idx);
        info->Predicted2 = pred; info->Index2 = idx;
    }
}

// Test hook (native-asm-check): the generated tables.
extern "C" void ra2asm_sos_tables(int32_t const **diff, uint16_t const **next)
{
    *diff = sos_tables().Diff;
    *next = sos_tables().Next;
}

// ---------------------------------------------------------------------------
// ASM_UnVQ_4x2 0x0077FF50, ASM_UnVQ_4x4 0x0077FFE6, ASM_UnVQ_4x4_HALF
// 0x007800A4, ASM_UnVQ1_C1_4x4 0x007801D8 (unvq_asm.asm): full-frame VQ
// drawers. The pointer table is split: low bytes first (blocksperrow*numrows
// of them), high bytes after. Pointer p = hi<<8 | lo.
//   C0 (8-bit): hi == 0xFF draws a one-colour block of colour lo; otherwise
//               codeword p (4x2: 8 bytes, 4x4: 16 bytes, rows of 4 pixels).
//   4x4_HALF:   2x2 output per block: codeword bytes 0, 2 (row 0) and 8, 10
//               (row 1); one-colour blocks as above.
//   C1 (16-bit): hi bit 7 set draws a one-colour block of the 16-bit value
//               p & 0x7FFF (no table); otherwise codeword p (32 bytes: four
//               rows of four 16-bit pixels). bufwidth is in pixels.
// bufwidth is the destination pitch in pixels; block rows advance by
// bufwidth * block height (half: * 2). The original draws whole rows of
// blocks until the pointer cursor reaches the end (a do-while: callers never
// pass zero rows or zero blocks per row; those return here without drawing).
// ---------------------------------------------------------------------------
namespace {
template <int H, bool Half>
void unvq_c0(unsigned char *codebook, unsigned char *pointers, unsigned char *buffer, uint32_t bpr, uint32_t rows,
             uint32_t pitch)
{
    if (bpr == 0 || rows == 0) return;
    uint32_t entries = bpr * rows;
    unsigned char *lo = pointers;
    unsigned char *row = buffer;
    uint32_t rowoffset = pitch * (Half ? 2 : H);
    for (uint32_t r = 0; r < rows; ++r, row += rowoffset) {
        unsigned char *d = row;
        for (uint32_t b = 0; b < bpr; ++b, ++lo) {
            uint8_t l = lo[0];
            uint8_t h = lo[entries];
            if (h == 0xFF) {
                for (int y = 0; y < (Half ? 2 : H); ++y) memset(d + y * pitch, l, Half ? 2 : 4);
            } else {
                unsigned char const *cw = codebook + ((uint32_t)((h << 8) | l) * (uint32_t)(4 * H));
                if (Half) {
                    d[0] = cw[0]; d[1] = cw[2];
                    d[pitch] = cw[8]; d[pitch + 1] = cw[10];
                } else {
                    for (int y = 0; y < H; ++y) memcpy(d + y * pitch, cw + 4 * y, 4);
                }
            }
            d += Half ? 2 : 4;
        }
    }
}
}

extern "C" void ASM_UnVQ_4x2(unsigned char *codebook, unsigned char *pointers, unsigned char *buffer,
                             RA2_ULONG blocksperrow, RA2_ULONG numrows, RA2_ULONG bufwidth)
{
    unvq_c0<2, false>(codebook, pointers, buffer, blocksperrow, numrows, bufwidth);
}

extern "C" void ASM_UnVQ_4x4(unsigned char *codebook, unsigned char *pointers, unsigned char *buffer,
                             RA2_ULONG blocksperrow, RA2_ULONG numrows, RA2_ULONG bufwidth)
{
    unvq_c0<4, false>(codebook, pointers, buffer, blocksperrow, numrows, bufwidth);
}

extern "C" void ASM_UnVQ_4x4_HALF(unsigned char *codebook, unsigned char *pointers, unsigned char *buffer,
                                  RA2_ULONG blocksperrow, RA2_ULONG numrows, RA2_ULONG bufwidth)
{
    unvq_c0<4, true>(codebook, pointers, buffer, blocksperrow, numrows, bufwidth);
}

extern "C" void ASM_UnVQ1_C1_4x4(unsigned char *codebook, unsigned char *pointers, unsigned char *buffer,
                                 RA2_ULONG blocksperrow, RA2_ULONG numrows, RA2_ULONG bufwidth)
{
    uint32_t bpr = blocksperrow, rows = numrows;
    if (bpr == 0 || rows == 0) return;
    uint32_t entries = bpr * rows;
    uint32_t pitch = bufwidth * 2;                 // bytes
    unsigned char *lo = pointers;
    unsigned char *row = buffer;
    for (uint32_t r = 0; r < rows; ++r, row += pitch * 4) {
        unsigned char *d = row;
        for (uint32_t b = 0; b < bpr; ++b, ++lo, d += 8) {
            uint32_t p = ((uint32_t)lo[entries] << 8) | lo[0];
            if (p & 0x8000) {
                uint16_t c = (uint16_t)(p & 0x7FFF);
                for (int y = 0; y < 4; ++y)
                    for (int x = 0; x < 4; ++x) memcpy(d + y * pitch + 2 * x, &c, 2);
            } else {
                unsigned char const *cw = codebook + p * 32;
                for (int y = 0; y < 4; ++y) memcpy(d + y * pitch, cw + 8 * y, 8);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// ASM_UnVQ1_C1_TABLE 0x0077FD80 and ASM_UnVQ1_C1_TABLE_ALT 0x0077FE89 (unvq_asm.asm; lane C16:
// the hi-colour drawers VQAClass installs for a colour mode 1 movie, VQAClass::Play_VQA's
// VQA_SetUnVQ calls; a mod's .VQA reached their link-closure traps). Pointers as ASM_UnVQ1_C1_4x4:
// the low bytes then the high bytes, entries apart; a pointer with bit 15 set is one colour,
// HicolorTable[p & 0x7FFF] (the screen-format pixel), otherwise codeword p (32 bytes: four rows of
// four 16-bit pixels). bufwidth is in pixels; block rows are four pixel rows apart. The ALT form
// draws the codeword's rows 0 and 2 into the block's rows 0 and 2 only (rows 1 and 3 untouched),
// as the original's interlaced variant does.
// ---------------------------------------------------------------------------
extern unsigned short *HicolorTable;                               // 0x00AD0614 (the rebuild's _vqa.h)
namespace {
template <bool Alt>
void unvq_c1_table(unsigned char *codebook, unsigned char *pointers, unsigned char *buffer, uint32_t bpr, uint32_t rows,
                   uint32_t bufwidth)
{
    if (bpr == 0 || rows == 0) return;
    uint32_t entries = bpr * rows;
    uint32_t pitch = bufwidth * 2;                 // bytes
    unsigned char *lo = pointers;
    unsigned char *row = buffer;
    for (uint32_t r = 0; r < rows; ++r, row += pitch * 4) {
        unsigned char *d = row;
        for (uint32_t b = 0; b < bpr; ++b, ++lo, d += 8) {
            uint32_t p = ((uint32_t)lo[entries] << 8) | lo[0];
            if (p & 0x8000) {
                uint16_t c = HicolorTable[p & 0x7FFF];
                for (int y = 0; y < 4; y += Alt ? 2 : 1)
                    for (int x = 0; x < 4; ++x) memcpy(d + y * pitch + 2 * x, &c, 2);
            } else {
                unsigned char const *cw = codebook + p * 32;
                for (int y = 0; y < 4; y += Alt ? 2 : 1) memcpy(d + y * pitch, cw + 8 * y, 8);
            }
        }
    }
}
}

extern "C" void ASM_UnVQ1_C1_TABLE(unsigned char *codebook, unsigned char *pointers, unsigned char *buffer,
    RA2_ULONG blocksperrow, RA2_ULONG numrows, RA2_ULONG bufwidth)        // 0x0077FD80 (main: VQA_Unknown_0077FD80)
{
    unvq_c1_table<false>(codebook, pointers, buffer, (uint32_t)blocksperrow, (uint32_t)numrows, (uint32_t)bufwidth);
}

extern "C" void ASM_UnVQ1_C1_TABLE_ALT(unsigned char *codebook, unsigned char *pointers, unsigned char *buffer,
    RA2_ULONG blocksperrow, RA2_ULONG numrows, RA2_ULONG bufwidth)        // 0x0077FE89 (main: VQA_Unknown_0077FE89)
{
    unvq_c1_table<true>(codebook, pointers, buffer, (uint32_t)blocksperrow, (uint32_t)numrows, (uint32_t)bufwidth);
}

// UnVQ1_4x2_Table 0x0070C0A0 (main's behaviour_c16_vqa.cpp body, carried here by lane P3) is the
// rebuild's own since the re-pin to b873d2b3c6 (lane P5: unvqtblc.cpp's C text), so it is dropped.



// ---------------------------------------------------------------------------------------------------
// From native/link/behaviour_asm_lib.cpp. LCW_Comp has C linkage in the rebuild (lcw.h).
// ---------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// LCW_Comp 0x00534D40 (src/recovered/lcw/LCW_Comp.cpp: TibSun
// lcw.cpp's inline assembly, Westwood's LCWCOMP.ASM). A transliteration of
// the register program: the same greedy search, so the output bytes equal the
// original's for any input.
//   - The output starts with a one-byte literal (0x81, byte).
//   - At each position: if the byte 64 ahead is the same, the run of equal
//     bytes is measured (repe scasb to the end of the data, minus one: a run
//     reaching the end is counted one short); 65 or more emits a fill code
//     FE len16 byte and the position moves past it.
//   - Otherwise every earlier position (from the start of the data) holding
//     the current byte is tried: the candidate is compared only if it also
//     matches at offset count-1 (the best length so far); the longest match
//     wins, the later one on ties (the original keeps a match unless it is
//     shorter: jb searchloop).
//   - Length <= 2: the byte is appended to the open literal (80+n), opening a
//     new one when none is open or it holds 63 bytes. 3..10 with a distance
//     <= 0xFFF: 0LLLdddd dddddddd (relative). Up to 64: 11LLLLLL abs16.
//     Longer: FF len16 abs16 (absolute offsets from the start of the data).
//   - The data ends with 0x80; returns the compressed size.
// The original compares the byte 64 ahead even when it lies past the end of
// the data; a run there is always shorter than 65 (it is bounded by the end),
// so skipping that read when it is out of range gives the same output.
// ---------------------------------------------------------------------------
extern "C" int LCW_Comp(void const *source, void *dest, int datasize)
{
    uint8_t const *esi = (uint8_t const *)source;
    uint8_t *out = (uint8_t *)dest;
    uint8_t const *const end = esi + datasize;
    uint8_t *const a1stdest = out;
    uint8_t const *const a1stsrc = esi;
    uint8_t *lenoff = out;
    bool inlen = true;
    uint8_t const *matchoff = esi;

    *out++ = 0x81;
    *out++ = *esi++;
    while (esi < end) {
        uint8_t const *search = a1stsrc;       // edi while searching
        uint32_t count = 1;
        uint8_t al;
    searchloop:
        al = esi[0];
        if (esi + 64 < end && al == esi[64]) {
            uint8_t const *p = esi;            // repe scasb over end - esi bytes
            for (ptrdiff_t n = end - esi; n != 0; ) {
                --n;
                if (*p++ != al) break;
            }
            --p;
            uint32_t run = (uint32_t)(p - esi);
            if (run >= 65) {
                inlen = false;
                esi = p;
                out[0] = 0xFE;
                out[1] = (uint8_t)run;
                out[2] = (uint8_t)(run >> 8);
                out[3] = al;
                out += 4;
                if (esi >= end) break;         // (the asm's next loop sees nothing left)
                goto searchloop;
            }
        }
        for (;;) {                             // oploop
            ptrdiff_t n = esi - search;
            if (n == 0) break;
            bool found = false;                 // repne scasb
            while (n != 0) {
                --n;
                if (*search++ == al) { found = true; break; }
            }
            if (!found) break;
            if (esi[(ptrdiff_t)count - 1] != search[(ptrdiff_t)count - 2]) continue;
            uint8_t const *saved = search;
            uint8_t const *q = search - 1;
            uint8_t const *s = esi;
            bool equal = true;                   // repe cmpsb over end - esi bytes
            for (ptrdiff_t m = end - esi; m != 0; ) {
                --m;
                equal = (*s++ == *q++);
                if (!equal) break;
            }
            if (equal) ++q;
            uint32_t length = (uint32_t)(q - saved);
            search = saved;
            if (length < count) goto searchloop;
            count = length;
            matchoff = saved - 1;
            goto searchloop;
        }
        // searchdone
        if (count <= 2) {
            if (!inlen || *lenoff == 0xBF) {
                do {
                    lenoff = out;
                    *out++ = 0x80;
                } while (*lenoff == 0xBF);
            }
            (*lenoff)++;
            *out++ = *esi++;
            inlen = true;
            continue;
        }
        uint32_t distance = (uint32_t)(esi - matchoff);
        if (count <= 10 && distance <= 0xFFF) {
            out[0] = (uint8_t)(((count - 3) << 4) + (distance >> 8));
            out[1] = (uint8_t)distance;
            out += 2;
        } else {
            if (count <= 64) {
                *out++ = (uint8_t)((count - 3) | 0xC0);
            } else {
                out[0] = 0xFF;
                out[1] = (uint8_t)count;
                out[2] = (uint8_t)(count >> 8);
                out += 3;
            }
            uint32_t offset = (uint32_t)(matchoff - a1stsrc);
            out[0] = (uint8_t)offset;
            out[1] = (uint8_t)(offset >> 8);
            out += 2;
        }
        esi += count;
        inlen = false;
    }
    *out++ = 0x80;
    return (int)(out - a1stdest);
}

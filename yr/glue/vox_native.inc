// SPDX-License-Identifier: GPL-3.0-or-later

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y3): FUN_00752760 (YR 0x00752760), the EVA speech service, is
// unmatched in the YR tree. Written from gamemd.exe's code: when audio runs, the speech stream
// is idle and its 500 ms gap since the last line has passed, it takes the next queued line (the
// two priority lists, then a held node, then the four type lists from the last), marks the
// previous and the new voice played (state 2), and streams the voice's file for the player's
// side (Files[2] side 0, Files[1] side 1, Files[0] otherwise) with ".WAV". Behaviour code for
// the native build only.
// ---------------------------------------------------------------------------------------------
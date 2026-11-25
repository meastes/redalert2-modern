// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P6, branch ports-on-rebuild): appended to the generated vanim unit
// (native/rb/fixups.json "append"). NOT EXACT; never part of an exact build.
//
// VoxelAnims (0x008399D0): the rebuild declares it (_vanim.h) but no unit defines it at the pin (main:
// src/recovered/classes/VoxelAnimClass/VoxelAnimClass_list_init.cpp, the _initterm entry 0x0040EC40), so
// ra2_data.py bound it to zero storage and its constructor never ran: the vector had no vtable, and the
// first VoxelAnimClass destroyed (Process_Deferred_Deletion, VoxelAnims.Delete) faulted at address 0x30.
// Reached by the ALL03U campaign route (a voxel debris animation ending). The definition gives it its
// constructor, run with the unit's other static initialisers after the game.exe load.
// ---------------------------------------------------------------------------------------------
DynamicVectorClass<VoxelAnimClass *> VoxelAnims;

// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P2, branch ports-on-rebuild): appended to the generated voxlib unit
// (native/rb/fixups.json "append"). NOT EXACT; never part of an exact build.
//
// The handwritten voxel drawers (winasm.asm, the first half of VoxelDrawFunctions) as the C++
// drawer of the same slot in the table's second half, as main's port does
// (native/link/behaviour_voxel.cpp: an assembly entry takes the C drawer of the same index).
// ---------------------------------------------------------------------------------------------
extern "C" {
void __cdecl Draw_Voxel_Regular_Normals_ASM(VoxelFuncArgumentStruct * state) { Draw_Voxel_Regular_Normals(state); }
void __cdecl Draw_Voxel_Reverse_Normals_ASM(VoxelFuncArgumentStruct * state) { Draw_Voxel_Reverse_Normals(state); }
void __cdecl Draw_Voxel_Regular_Lighting_Normals_ASM(VoxelFuncArgumentStruct * state) { Draw_Voxel_Regular_Normals_Lighting(state); }
void __cdecl Draw_Voxel_Reverse_Lighting_Normals_ASM(VoxelFuncArgumentStruct * state) { Draw_Voxel_Reverse_Normals_Lighting(state); }
void __cdecl Draw_Voxel_Regular_ASM(VoxelFuncArgumentStruct * state) { Draw_Voxel_Regular(state); }
void __cdecl Draw_Voxel_Reverse_ASM(VoxelFuncArgumentStruct * state) { Draw_Voxel_Reverse(state); }
}

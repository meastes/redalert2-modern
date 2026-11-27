// SPDX-License-Identifier: GPL-3.0-or-later

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y3), appended to the voxel unit: FUN_00754CB0, YR 0x00754CB0
// (3,290 bytes, unmatched in ra2-decomp; TibSun Init_Voxel_Matrices). From the RA2 tree's twin
// Unknown_00718060 (src/recovered/voxel.cpp, 0x00718060), whose body gamemd.exe repeats call for
// call with the same constants; the RA2 tables map to this unit's by the addresses gamemd.exe
// writes: ScreenSlopeMatrices/Quaternions (RA2 0x00AF73A0/0x00AF77D8), SlopeMatrices/Quaternions
// (0x00AF8608/0x00AF6618), LightDirectionVectors/LightTiltVectors (0x00AF6768/0x00AF7930),
// IsometricViewMatrix, BounceMatrices. The slopes are this unit's CELL_SLOPE_ANGLE (0x00B43F08)
// and CELL_DIAG_SLOPE_ANGLE (0x00B44310), the view angles RAD_60 and RAD_45.
// Loop counters are distinct: the tree builds with VC6 for-scope.
// Behaviour code for the native build; never part of an exact build.
// ---------------------------------------------------------------------------------------------
// BounceMatrices' entries (Matrix3D::Row is protected; the original writes the fields).
struct YRBounceMatrix : Matrix3D {
	static void Set_Entry(Matrix3D &matrix, int row, int column, float value) { ((YRBounceMatrix &)matrix).Row[row][column] = value; }
};

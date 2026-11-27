// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y11b): ParticleClass::Spark_Behavior_AI, gamemd.exe 0x0062C6E0 (1,072
// bytes), unmatched in the YR tree (a link-closure trap once weapons hit: the sparks of a hit).
// Written from behaviour: Red Alert 2's draft (src/recovered/particle.cpp, 0x00609540; TibSun's
// ParticleClass::Spark_Behavior_AI) in the YR tree's names, checked against gamemd.exe's code: the
// spark falls under gravity until it meets the ground, a bridge deck or a wall, where it stops and is
// flagged done (gamemd.exe's slope reflection there goes to a local it never reads); its colour then
// moves on along the type's colour list.
// Behaviour code for the native build; never part of an exact build.
// ---------------------------------------------------------------------------------------------
#include "building.h"
#include "builtype.h"
#include "cell.h"

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y15), appended to the particle unit: ParticleClass::Gas_Behavior_AI (gamemd.exe (particle.cpp)), unmatched in
// ra2-decomp. Red Alert 2's text of the same routine (mischa85/RedAlert2 src/particle.cpp, game.exe 0x00608BB0; SPDX
// GPL-3.0-or-later, Copyright 2026 OpenTS contributors, LICENSE.md's terms; THIRD_PARTY.md), with the
// YR tree's names where they differ (marked NATIVE). Yuri's Revenge's routine is not compared with it
// instruction by instruction. Behaviour code for the native build only.
// ---------------------------------------------------------------------------------------------
// Its helper (the same file: the deflection off a ramped cell).
static inline Vector3 Slope_Vector(Vector3 & vector, Coord const & coord, double length) // RA2: defined above Gas_Behavior_AI, its first user
{
	Matrix3D slope_matrix;
	Matrix3D slope_matrix_inv = Matrix3D::Orthogonal_Inverse(
		slope_matrix = Get_Slope_Matrix(TacticalMap->Get_Cell_Ramp(coord)));
	Vector3 vec(vector.X, -vector.Y, vector.Z);
	vec = slope_matrix_inv.Rotate_Vector(vec);
	vec *= length;
	vec.Z = -vec.Z;
	vec = slope_matrix.Rotate_Vector(vec);
	vector.X = vec.X;
	vector.Y = -vec.Y;
	vector.Z = vec.Z;
	return(vector);
}

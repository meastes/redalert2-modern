// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P2, branch ports-on-rebuild): appended to the generated alphashp unit
// (native/rb/fixups.json "append"). NOT EXACT; never part of an exact build.
//
// AlphaShapeClass::Draw_All (0x0041F5E0): unmatched in both trees (the rebuild keeps only its
// comment). Main's native draft (native/drafts/0041f5e0.cpp, Scout1000/Scout1089) with the rebuild's
// names and this unit's Draw_In_Area for the buffer calls: every shape's light blended into the alpha
// buffer; a shape without its image takes the owner's type's.
// ---------------------------------------------------------------------------------------------
void AlphaShapeClass::Draw_All(Rect const & cliprect)
{
	for (int index = 0; index < AlphaShapes.Count(); index++) {
		Rect r1 = AlphaShapes[index]->DrawRect;
		r1 -= Point2D(TacticalMap->TacPixelX, TacticalMap->TacPixelY);
		r1 += TacticalRect.Top_Left();
		Rect clip = cliprect;
		Rect r2 = Intersect(r1, clip);
		if (!r2.Is_Valid()) {
			continue;
		}
		int x1 = r1.X;
		int y1 = r1.Y;
		ShapeSet const * shape = AlphaShapes[index]->ImageData;
		if (shape == NULL) {
			AlphaShapes[index]->ImageData = (ShapeSet const *)AlphaShapes[index]->Owner->Class_Of()->AlphaImageData;
			shape = AlphaShapes[index]->ImageData;
			if (shape == NULL) {
				continue;   // native: no image in the type either (the original would read through null)
			}
		}
		Rect shape_rect = shape->Get_Rect(0);
		int top = MAX(r2.Y, shape_rect.Y + y1);
		int src_y = top - shape_rect.Y - y1;
		int bottom = MIN(clip.Y + clip.Height, shape_rect.Y + shape_rect.Height + y1);
		int left = MAX(r2.X, shape_rect.X + x1);
		int src_x = left - shape_rect.X - x1;
		int right = MIN(clip.X + clip.Width, shape_rect.X + shape_rect.Width + x1);
		int shape_skip = shape_rect.Width - right + shape_rect.X + x1 + src_x;
		int alpha_skip = AlphaBuffer->Get_Buffer_Width() - right + left;
		unsigned char * shapeptr = (unsigned char *)shape->Get_Data(0) + src_x + src_y * shape_rect.Width;
		unsigned short * alphaptr = (unsigned short *)AlphaBuffer->Get_Buffer_Offset(Point2D(left - TacticalRect.X, top - TacticalRect.Y));
		if (&alphaptr[right - left + (bottom - top) * AlphaBuffer->Get_Buffer_Width() + 2] >= (unsigned short *)AlphaBuffer->Get_Buffer_End()) {
			for (int y = top; y < bottom; y++) {
				for (int x = left; x < right; x++) {
					unsigned char pixel = *shapeptr++;
					*alphaptr = BrightnessTable[pixel][*alphaptr];
					alphaptr++;
					alphaptr = (unsigned short *)AlphaBuffer->Wrap_Overflow((uintptr_t)alphaptr);
				}
				shapeptr += shape_skip;
				alphaptr += alpha_skip;
				alphaptr = (unsigned short *)AlphaBuffer->Wrap_Overflow((uintptr_t)alphaptr);
			}
		} else {
			for (int y = top; y < bottom; y++) {
				for (int x = left; x < right; x++) {
					unsigned char pixel = *shapeptr++;
					*alphaptr = BrightnessTable[pixel][*alphaptr];
					++alphaptr;
				}
				shapeptr += shape_skip;
				alphaptr += alpha_skip;
			}
		}
	}
}

﻿//-----------------------------------------------
//
//	This file is part of the Siv3D Engine.
//
//	Copyright (c) 2008-2021 Ryo Suzuki
//	Copyright (c) 2016-2021 OpenSiv3D Project
//
//	Licensed under the MIT License.
//
//-----------------------------------------------

# pragma once
# include <Siv3D/Common.hpp>
# include <Siv3D/BitmapGlyph.hpp>
# include <Siv3D/PredefinedYesNo.hpp>

struct FT_FaceRec_;
typedef struct FT_FaceRec_* FT_Face;

namespace s3d
{
	struct FontFaceProperty;

	[[nodiscard]]
	BitmapGlyph RenderBitmapGlyph(FT_Face face, GlyphIndex glyphIndex, const FontFaceProperty& prop);
}

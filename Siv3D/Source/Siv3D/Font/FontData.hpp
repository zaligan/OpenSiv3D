﻿//-----------------------------------------------
//
//	This file is part of the Siv3D Engine.
//
//	Copyright (c) 2008-2017 Ryo Suzuki
//	Copyright (c) 2016-2017 OpenSiv3D Project
//
//	Licensed under the MIT License.
//
//-----------------------------------------------

# pragma once
# include <../Source/ThirdParty/freetype/ft2build.h>
# include FT_FREETYPE_H
# include FT_SYNTHESIS_H
# include FT_TRUETYPE_TABLES_H
# include <Siv3D/HashMap.hpp>
# include <Siv3D/Image.hpp>
# include <Siv3D/DynamicTexture.hpp>

namespace s3d
{
	struct GlyphInfo
	{
		Rect bitmapRect = { 0,0,0,0 };

		Point offset = { 0,0 };

		int32 xAdvance = 0;
	};

	class FontData
	{
	private:

		using CommonGlyphIndex = uint32;

		HashMap<char32_t, CommonGlyphIndex> m_glyphIndexTable;

		FT_Face m_faceText = nullptr;

		FT_Face m_faceEmoji = nullptr;

		Array<GlyphInfo> m_glyphs;

		Optional<CommonGlyphIndex> m_tofuIndex;

		static constexpr int32 padding = 2;

		int32 m_fontSize = 0;

		int32 m_lineSpacing = 0;

		int32 m_ascender = 0;

		int32 m_descender = 0;

		bool m_bold = false;

		bool m_italic = false;

		bool m_noBitmap = true;

		Point m_penPos = { 0, padding };

		Image m_image;

		DynamicTexture m_texture;

		bool m_initialized = false;

		//bool loadFromFile(const FilePath& path)
		//{
		//	return true;
		//}

		//bool loadFromResource(const FilePath& path)
		//{
		//	return true;
		//}

		bool render(const std::u32string& codePoints);

		bool renderGlyph(FT_Face face, FT_UInt glyphIndex);

	public:

		struct Null {};

		FontData() = default;

		FontData(Null, FT_Library library);

		FontData(FT_Library library, const FilePath& filePath, const FilePath& emojiFilePath, const int32 fontSize, FontStyle style);

		~FontData();

		bool isInitialized() const noexcept
		{
			return m_initialized;
		}

		int32 getAscender() const
		{
			return m_ascender;
		}

		int32 getDescender() const
		{
			return m_descender;
		}

		RectF getBoundingRect(const String& text, double lineSpacingScale);

		RectF getRegion(const String& text, double lineSpacingScale);

		RectF draw(const String& text, const Vec2& pos, const ColorF& color, double lineSpacingScale);
	};
}

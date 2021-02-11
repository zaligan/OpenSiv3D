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

# include <Siv3D/Font.hpp>
# include <Siv3D/Error.hpp>
# include <Siv3D/Resource.hpp>
# include <Siv3D/ShaderCommon.hpp>
# include <Siv3D/ScopedCustomShader2D.hpp>
# include <Siv3D/EngineLog.hpp>
# include "CFont.hpp"
# include "GlyphCache/IGlyphCache.hpp"
# include "FontCommon.hpp"

namespace s3d
{
	CFont::CFont()
	{
		// do nothing
	}

	CFont::~CFont()
	{
		LOG_SCOPED_TRACE(U"CFont::~CFont()");

		m_fonts.destroy();

		if (m_freeType)
		{
			FT_Done_FreeType(m_freeType);
		}
	}

	void CFont::init()
	{
		LOG_SCOPED_TRACE(U"CFont::init()");

		if (const FT_Error error = FT_Init_FreeType(&m_freeType))
		{
			throw EngineError{ U"FT_Init_FreeType() failed" };
		}

		// null Font を管理に登録
		{
			// null Font を作成
			auto nullFont = std::make_unique<FontData>(FontData::Null{});

			if (not nullFont->isInitialized()) // もし作成に失敗していたら
			{
				throw EngineError(U"Null Font initialization failed");
			}

			// 管理に登録
			m_fonts.setNullData(std::move(nullFont));
		}

		m_shaders = std::make_unique<FontShader>();

		m_shaders->bitmapFont	= HLSL{ Resource(U"engine/shader/d3d11/bitmapfont.ps") }
								| GLSL{ Resource(U"engine/shader/glsl/bitmapfont.frag"), { { U"PSConstants2D", 0 } } }
								| ESSL{ Resource(U"engine/shader/glsl/bitmapfont.frag"), { { U"PSConstants2D", 0 } } }
								| MSL{ U"PS_Shape" }; // [Siv3D Todo]
		m_shaders->sdfFont		= HLSL{ Resource(U"engine/shader/d3d11/sdffont.ps") }
								| GLSL{ Resource(U"engine/shader/glsl/sdffont.frag"), { { U"PSConstants2D", 0 } } }
								| ESSL{ Resource(U"engine/shader/glsl/sdffont.frag"), { { U"PSConstants2D", 0 } } }
								| MSL{ U"PS_Shape" }; // [Siv3D Todo]
		m_shaders->msdfFont		= HLSL{ Resource(U"engine/shader/d3d11/msdffont.ps") }
								| GLSL{ Resource(U"engine/shader/glsl/msdffont.frag"), { { U"PSConstants2D", 0 } } }
								| ESSL{ Resource(U"engine/shader/glsl/msdffont.frag"), { { U"PSConstants2D", 0 } } }
								| MSL{ U"PS_Shape" }; // [Siv3D Todo]

		if ((not m_shaders->bitmapFont)
			|| (not m_shaders->sdfFont)
			|| (not m_shaders->msdfFont))
		{
			throw EngineError(U"CFont::init(): Failed to load font shaders");
		}
	}

	Font::IDType CFont::create(const FilePathView path, const FontMethod fontMethod, const int32 fontSize, const FontStyle style)
	{
		// Font を作成
		auto font = std::make_unique<FontData>(m_freeType, path, fontMethod, fontSize, style);

		if (not font->isInitialized()) // もし作成に失敗していたら
		{
			return Font::IDType::NullAsset();
		}

		const auto& prop = font->getProperty();
		const String fontName = (prop.styleName) ? (prop.familiyName + U' ' + prop.styleName) : (prop.familiyName);
		const String info = U"(`{0}`, size: {1}, style: {2}, ascender: {3}, descender: {4})"_fmt(fontName, prop.fontPixelSize, detail::ToString(prop.style), prop.ascender, prop.descender);

		// Font を管理に登録
		return m_fonts.add(std::move(font), info);
	}

	void CFont::release(const Font::IDType handleID)
	{
		m_fonts.erase(handleID);
	}

	const FontFaceProperty& CFont::getProperty(const Font::IDType handleID)
	{
		return m_fonts[handleID]->getProperty();
	}

	FontMethod CFont::getMethod(const Font::IDType handleID)
	{
		return m_fonts[handleID]->getMethod();
	}

	void CFont::setBufferThickness(const Font::IDType handleID, const int32 thickness)
	{
		return m_fonts[handleID]->getGlyphCache().setBufferWidth(thickness);
	}

	int32 CFont::getBufferThickness(const Font::IDType handleID)
	{
		return m_fonts[handleID]->getGlyphCache().getBufferWidth();
	}

	bool CFont::hasGlyph(const Font::IDType handleID, StringView ch)
	{
		return m_fonts[handleID]->hasGlyph(ch);
	}

	GlyphIndex CFont::getGlyphIndex(const Font::IDType handleID, const StringView ch)
	{
		return m_fonts[handleID]->getGlyphIndex(ch);
	}

	Array<GlyphCluster> CFont::getGlyphClusters(const Font::IDType handleID, const StringView s)
	{
		return m_fonts[handleID]->getGlyphClusters(s);
	}

	GlyphInfo CFont::getGlyphInfo(const Font::IDType handleID, const StringView ch)
	{
		const auto& font = m_fonts[handleID];

		return font->getGlyphInfoByGlyphIndex(font->getGlyphIndex(ch));
	}

	OutlineGlyph CFont::renderOutline(const Font::IDType handleID, const StringView ch, const CloseRing closeRing)
	{
		const auto& font = m_fonts[handleID];

		return font->renderOutlineByGlyphIndex(font->getGlyphIndex(ch), closeRing);
	}

	OutlineGlyph CFont::renderOutlineByGlyphIndex(const Font::IDType handleID, const GlyphIndex glyphIndex, const CloseRing closeRing)
	{
		return m_fonts[handleID]->renderOutlineByGlyphIndex(glyphIndex, closeRing);
	}

	Array<OutlineGlyph> CFont::renderOutlines(const Font::IDType handleID, const StringView s, const CloseRing closeRing)
	{
		return m_fonts[handleID]->renderOutlines(s, closeRing);
	}

	BitmapGlyph CFont::renderBitmap(const Font::IDType handleID, const StringView s)
	{
		const auto& font = m_fonts[handleID];

		return font->renderBitmapByGlyphIndex(font->getGlyphIndex(s));
	}

	BitmapGlyph CFont::renderBitmapByGlyphIndex(const Font::IDType handleID, const GlyphIndex glyphIndex)
	{
		return m_fonts[handleID]->renderBitmapByGlyphIndex(glyphIndex);
	}

	SDFGlyph CFont::renderSDF(const Font::IDType handleID, const StringView s, const int32 buffer)
	{
		const auto& font = m_fonts[handleID];

		return font->renderSDFByGlyphIndex(font->getGlyphIndex(s), buffer);
	}

	SDFGlyph CFont::renderSDFByGlyphIndex(const Font::IDType handleID, const GlyphIndex glyphIndex, const int32 buffer)
	{
		return m_fonts[handleID]->renderSDFByGlyphIndex(glyphIndex, buffer);
	}

	MSDFGlyph CFont::renderMSDF(const Font::IDType handleID, const StringView s, const int32 buffer)
	{
		const auto& font = m_fonts[handleID];

		return font->renderMSDFByGlyphIndex(font->getGlyphIndex(s), buffer);
	}

	MSDFGlyph CFont::renderMSDFByGlyphIndex(const Font::IDType handleID, const GlyphIndex glyphIndex, const int32 buffer)
	{
		return m_fonts[handleID]->renderMSDFByGlyphIndex(glyphIndex, buffer);
	}

	bool CFont::preload(const Font::IDType handleID, const StringView chars)
	{
		const auto& font = m_fonts[handleID];

		return font->getGlyphCache().preload(*font, chars);
	}

	const Texture& CFont::getTexture(const Font::IDType handleID)
	{
		return m_fonts[handleID]->getGlyphCache().getTexture();
	}

	RectF CFont::region(const Font::IDType handleID, const StringView s, const Array<GlyphCluster>& clusters, const Vec2& pos, const double fontSize, const double lineHeightScale)
	{
		const auto& font = m_fonts[handleID];
		{
			return m_fonts[handleID]->getGlyphCache().region(*font, s, clusters, pos, fontSize, lineHeightScale);
		}
	}

	RectF CFont::regionBase(const Font::IDType handleID, const StringView s, const Array<GlyphCluster>& clusters, const Vec2& pos, const double fontSize, const double lineHeightScale)
	{
		const auto& font = m_fonts[handleID];
		{
			return m_fonts[handleID]->getGlyphCache().regionBase(*font, s, clusters, pos, fontSize, lineHeightScale);
		}
	}

	RectF CFont::draw(const Font::IDType handleID, const StringView s, const Array<GlyphCluster>& clusters, const Vec2& pos, const double fontSize, const ColorF& color, const double lineHeightScale)
	{
		const auto& font = m_fonts[handleID];
		{
			ScopedCustomShader2D ps{ m_shaders->getFontShader(font->getMethod()) };

			return m_fonts[handleID]->getGlyphCache().draw(*font, s, clusters, pos, fontSize, color, lineHeightScale);
		}
	}

	RectF CFont::drawBase(const Font::IDType handleID, const StringView s, const Array<GlyphCluster>& clusters, const Vec2& pos, const double fontSize, const ColorF& color, const double lineHeightScale)
	{
		const auto& font = m_fonts[handleID];
		{
			ScopedCustomShader2D ps{ m_shaders->getFontShader(font->getMethod()) };

			return m_fonts[handleID]->getGlyphCache().drawBase(*font, s, clusters, pos, fontSize, color, lineHeightScale);
		}
	}
}

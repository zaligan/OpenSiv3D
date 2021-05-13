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
# include <Siv3D/Wave.hpp>
# include <Siv3D/IReader.hpp>

namespace s3d
{
	class SIV3D_NOVTABLE ISiv3DAudioCodec
	{
	public:

		static ISiv3DAudioCodec* Create();

		virtual ~ISiv3DAudioCodec() = default;

		virtual void init() = 0;

		virtual Wave decode(IReader& reader) = 0;
	};
}

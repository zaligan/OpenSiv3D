//-----------------------------------------------
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
# include <Siv3D/AudioCodec/IAudioCodec.hpp>

namespace s3d
{
	class CAudioCodec final : public ISiv3DAudioCodec
	{
	public:

		CAudioCodec();

		~CAudioCodec() override;

		void init() override;
	
		Wave decode(IReader& reader) override;
	};
}

﻿//-----------------------------------------------
//
//	This file is part of the Siv3D Engine.
//
//	Copyright (C) 2008-2017 Ryo Suzuki
//	Copyright (C) 2016-2017 OpenSiv3D Project
//
//	Licensed under the MIT License.
//
//-----------------------------------------------

# pragma once
# include <mutex>
# include <Siv3D/TextWriter.hpp>
# include "ILogger.hpp"

namespace s3d
{
	class CLogger : public ISiv3DLogger
	{
	private:

		TextWriter m_writer;

		std::mutex m_mutex;

		bool m_initialized = false;

		bool m_hasImportantLog = false;

		bool m_removeFileOnExit = false;

		void outputLicenses();

	public:

		CLogger();

		~CLogger() override;

		bool init() override;

		void write(LogDescription desc, const String& str) override;

		void removeLogOnExit() override;
	};
}

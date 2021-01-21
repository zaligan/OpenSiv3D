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
# include <climits>
# include <Siv3D/DirectoryWatcher.hpp>
# include <sys/inotify.h>
# include <boost/bimap.hpp>

namespace s3d
{
	class DirectoryWatcher::DirectoryWatcherDetail
	{
	public:

		explicit DirectoryWatcherDetail(const FilePath& directory);

		~DirectoryWatcherDetail();

		bool isActive() const;

		void retrieveChanges(Array<FileChange>& fileChanges);

		void clearChanges();

		const FilePath& directory() const noexcept;

	private:

		constexpr static size_t EventSize = sizeof(inotify_event);
		constexpr static size_t EventBufferSize = ((EVENT_SIZE + NAME_MAX + 1) * 4096);
		constexpr static size_t WatchMask = (IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO | IN_ONLYDIR);

		Array<uint8_t> m_buffer;

		std::thread m_thread;

		std::atomic<bool> m_initCalled = false;

		bool m_isActive = false;

		std::atomic<bool> m_abort = false;

		std::mutex m_changesMutex;
		
		Array<FileChange> m_fileChanges;	

		static void watch(DirectoryWatcherDetail* watcher);

		bool init();

		void update();

		void dispose();
	};
}

#pragma once

#include "Memory.h"

namespace RB
{
	class FileHandle;

	enum OpenFileMode : uint32_t
	{
		kFileModeNone	= 0,
		kFileModeRead	= (1 << 0),
		kFileModeWrite	= (1 << 1),
		kFileModeBinary	= (1 << 2)
	};

	class FileLoader
	{
	public:
		static Unique<FileHandle> OpenFile(const char* file_name, uint32_t open_mode);
	};

	struct FileData;

	class FileHandle
	{
	public:
		// Error handle
		FileHandle();
		FileHandle(void* stream, uint32_t open_mode);
		~FileHandle();

		FileData ReadFull();

		bool IsValid() const { return m_IsValid; }

	private:
		void* m_Stream;
		bool  m_IsValid;
		bool  m_ReadAccess;
		bool  m_WriteAccess;
		bool  m_BinaryMode;
	};

	struct FileData
	{
		uint8_t* data;
		uint32_t size;

		~FileData();
	};
}
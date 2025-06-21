#include "RabBitCommon.h"
#include "File.h"

#include <fstream>

namespace RB
{
    Unique<FileHandle> FileLoader::OpenFile(const char* file_name, uint32_t open_mode)
    {
        std::ios_base::openmode mode = 0;

        if ((open_mode & OpenFileMode::kFileMode_Read) > 0)
            mode |= std::fstream::in;
        if ((open_mode & OpenFileMode::kFileMode_Write) > 0)
            mode |= std::fstream::out;
        if ((open_mode & OpenFileMode::kFileMode_Binary) > 0)
            mode |= std::fstream::binary;

        std::fstream* stream = new std::fstream();
        stream->open(file_name, mode);

        Unique<FileHandle> handle;

        if (stream->is_open())
        {
            handle = CreateUnique<FileHandle>(stream, open_mode);
        }
        else
        {
            RB_LOG_ERROR(LOGTAG_GRAPHICS, "Could not open file: %s", file_name);

            // Error handle
            handle = CreateUnique<FileHandle>();
        }

        return handle;
    }

    FileHandle::FileHandle()
        : m_IsValid(false)
        , m_Stream(nullptr)
        , m_ReadAccess(false)
        , m_WriteAccess(false)
        , m_BinaryMode(false)
    {

    }

    FileHandle::FileHandle(void* stream, uint32_t open_mode)
        : m_IsValid(true)
        , m_Stream(stream)
        , m_ReadAccess((open_mode& OpenFileMode::kFileMode_Read) > 0)
        , m_WriteAccess((open_mode& OpenFileMode::kFileMode_Write) > 0)
        , m_BinaryMode((open_mode& OpenFileMode::kFileMode_Binary) > 0)
    {
    }

    FileHandle::~FileHandle()
    {
        if (!m_IsValid)
        {
            return;
        }

        ((std::fstream*)m_Stream)->close();
        delete m_Stream;
    }

    FileData FileHandle::ReadFull()
    {
        std::fstream* stream = (std::fstream*)m_Stream;

        stream->seekg(0, stream->end);
        uint32_t file_length = stream->tellg();
        stream->seekg(0, stream->beg);

        uint8_t* buffer = new uint8_t[file_length];

        stream->read((char*)buffer, file_length);

        return { buffer, file_length };
    }

    FileData::~FileData()
    {
        delete[] data;
    }
}
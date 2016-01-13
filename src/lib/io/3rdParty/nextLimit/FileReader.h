#ifndef FILE_READER_H
#define FILE_READER_H

#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <new>

#define ZLIB_CONST
#include "zlib.h"

#include "../../../PartioConfig.h"

#define HandleBufferEndianness16(buf, num)
#define HandleBufferEndianness32(buf, num)
#define HandleBufferEndianness64(buf, num)

ENTER_PARTIO_NAMESPACE

class FileReader
{
public:
	FileReader();
	~FileReader();


	bool				Open(const char* path);
	void				Close();
	bool				ReadData(void* dest, int len);
	const void*			GetBuffer(uint32_t* availableBytes);
	const char*			GetResolvedPath() const { return m_resolvedPath; }


	void Skip(uint64_t numBytes)
	{
		m_readPos += numBytes;
	}

	void SetReadPtr(uint64_t newPos)
	{
		m_readPos = newPos;
	}

	int64_t GetReadPtr() const
	{
		return m_readPos;
	}

	int64_t GetSize() const
	{
		return m_fileSize;
	}

	bool Eof() const
	{
		return m_readPos >= m_fileSize;
	}

	bool ReadChar(char* x)
	{
		return ReadData(x, 1);
	}

	bool ReadInt32Buffer(int32_t* buf, int numInts)
	{
		if(!ReadData(buf, numInts*sizeof(int32_t)))
			return false;
		HandleBufferEndianness32(buf, numInts);
		return true;
	}

	bool ReadInt32(int32_t* x)
	{
		return ReadInt32Buffer(x, 1);
	}

	bool ReadUInt32(uint32_t* x)
	{
		return ReadInt32Buffer((int32_t*)x, 1);
	}

	bool ReadUInt64Buffer(uint64_t* buf, int numInts)
	{
		if(!ReadData(buf, numInts*sizeof(uint64_t)))
			return false;
		HandleBufferEndianness64(buf, numInts);
		return true;
	}

	bool ReadUInt64(uint64_t* x)
	{
		return ReadUInt64Buffer(x, 1);
	}

	bool ReadFloatBuffer(float* buf, int numFloats)
	{
		if(!ReadData(buf, numFloats*sizeof(float)))
			return false;
		HandleBufferEndianness32(buf, numFloats);
		return true;
	}

	bool ReadFloat(float* x)
	{
		return ReadFloatBuffer(x, 1);
	}

	bool ReadShort(int16_t* x)
	{
		if(!ReadData(x, sizeof(int16_t)))
			return false;

		HandleBufferEndianness16(x, 1);
		return true;
	}


private:

	char*				m_resolvedPath;
	FILE*				m_fp;
	char*				m_cache;
	int64_t				m_cacheStart;
	int					m_cacheLen;
	int64_t				m_readPos;
	int64_t				m_fileSize;
};

EXIT_PARTIO_NAMESPACE

#endif

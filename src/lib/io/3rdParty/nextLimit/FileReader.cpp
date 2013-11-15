#include "FileReader.h"

#if defined(_WIN32)
void PSetFilePointer(FILE* fp, int64_t pos, int mode) { _fseeki64(fp, pos, mode); }
int64_t PGetFilePointer(FILE* fp) { return _ftelli64(fp); }
#elif defined(__APPLE__)
static void PSetFilePointer(FILE* fp, int64_t pos, int mode) { fseeko(fp, pos, mode); }
static int64_t PGetFilePointer(FILE* fp) { return ftello(fp); }
#elif defined(__linux__)
static void PSetFilePointer(FILE* fp, int64_t pos, int mode) {	fseeko64(fp, pos, mode); }
static int64_t PGetFilePointer(FILE* fp) { return ftello64(fp); }
#else
#error Unknown platform.
#endif

#define FILE_READER_CACHE_SIZE		(1024*1024)

FileReader::FileReader()
{
	m_fp = NULL;
	m_cache = NULL;
	m_fileSize = 0;
	m_resolvedPath = NULL;
}

FileReader::~FileReader()
{
	Close();
}

void FileReader::Close()
{
	if(m_fp)
	{
		fclose(m_fp);
		m_fp = NULL;
	}

	free(m_cache);
	m_cache = NULL;

	free(m_resolvedPath);
	m_resolvedPath = NULL;
}


bool FileReader::Open(const char* path)
{
	Close();

	m_resolvedPath = strdup(path);

	m_cache = (char*)malloc(FILE_READER_CACHE_SIZE);
	if(!m_cache)
		return false;

	m_fp = fopen(m_resolvedPath, "rb");
	if(!m_fp)
	{
		free(m_cache);
		m_cache = NULL;
		return false;
	}

	// Determine size.
	PSetFilePointer(m_fp, 0, SEEK_END);
	m_fileSize = PGetFilePointer(m_fp);
	PSetFilePointer(m_fp, 0, SEEK_SET);

	// Read first chunk.
	m_cacheStart = 0;
	m_cacheLen = (int)fread(m_cache, 1, FILE_READER_CACHE_SIZE, m_fp);
	m_readPos = 0;

	return true;
}

bool FileReader::ReadData(void* dest, int len)
{
	if(len < 0)
		return false;

	for(;;)
	{
		int64_t cacheOffset = m_readPos - m_cacheStart;
		if((m_readPos >= m_cacheStart) && (cacheOffset < (int64_t)m_cacheLen))
		{
			// Copy existing data, advance read pointer, update write pointer and remaining length.
			int avail = m_cacheLen - (int)cacheOffset;
			int copyLen = len < avail ? len : avail;
			memcpy(dest, m_cache+cacheOffset, copyLen);
			dest = ((char*)dest) + copyLen;
			len -= copyLen;
			m_readPos += copyLen;

			// If we filled the buffer, we're done.
			if(len == 0)
				return true;
		}
		else
		{
			if(cacheOffset != m_cacheLen)
			{
				// The read pointer is outside the range covered by the cache. This means we need to seek.
				PSetFilePointer(m_fp, m_readPos, SEEK_SET);
			}
		}

		// Read a new chunk in the cache. If we're at the end of the file, we're screwed.
		int readLen = (int)fread(m_cache, 1, FILE_READER_CACHE_SIZE, m_fp);
		if(readLen == 0)
			return false;

		m_cacheStart = m_readPos;
		m_cacheLen = readLen;
	}
}

const void* FileReader::GetBuffer(uint32_t* availableBytes)
{
	int64_t cacheOffset = m_readPos - m_cacheStart;
	if((m_readPos < m_cacheStart) || (cacheOffset >= (int64_t)m_cacheLen))
	{
		if(cacheOffset != m_cacheLen)
			PSetFilePointer(m_fp, m_readPos, SEEK_SET);

		int readLen = (int)fread(m_cache, 1, FILE_READER_CACHE_SIZE, m_fp);
		if(readLen == 0)
		{
			*availableBytes = 0;
			return NULL;
		}

		m_cacheStart = m_readPos;
		m_cacheLen = readLen;
		cacheOffset = m_readPos - m_cacheStart;
	}

	*availableBytes = m_cacheLen - (int)cacheOffset;
	return m_cache + cacheOffset;
}

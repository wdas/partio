#include "RPCReader.h"
#include "FileReader.h"

#define RPC_SIGNATURE					0x70FABADA
#define RPC_MIN_SUPPORTED_VERSION		3
#define RPC_MAX_SUPPORTED_VERSION		3

ENTER_PARTIO_NAMESPACE

RPCFile::ChannelInfo::ChannelInfo()
{
	m_name = NULL;
	m_valueLimits = NULL;
}

RPCFile::ChannelInfo::~ChannelInfo()
{
	free(m_name);
	free(m_valueLimits);
}

RPCFile::ChannelReader::ChannelReader(FileReader* file, const ChannelInfo* info, bool newFileStream)
{
	m_chanInfo = info;
	m_readOffset = m_chanInfo->GetDataOffset();

	m_inflateCtx.zalloc = Z_NULL;
	m_inflateCtx.zfree = Z_NULL;
	m_inflateCtx.opaque = Z_NULL;
	m_inflateCtx.avail_in = 0;
	m_inflateCtx.next_in = Z_NULL;
	int ret = inflateInit(&m_inflateCtx);
	if(ret != Z_OK)
	{
		m_file = NULL;
		return;
	}

	if(newFileStream)
	{
		m_file = new FileReader;
		if(!m_file->Open(file->GetResolvedPath()))
		{
			// This really shouldn't happen, but let's be paranoid.
			delete m_file;
			m_file = NULL;
			return;
		}
		m_ownFile = true;
	}
	else
	{
		m_file = file;
		m_ownFile = false;
	}
}

RPCFile::ChannelReader::~ChannelReader()
{
	inflateEnd(&m_inflateCtx);
	if(m_ownFile)
		delete m_file;
}

void RPCFile::ChannelReader::CloseFile()
{
	if(m_ownFile)
		delete m_file;
	m_file = NULL;
}

bool RPCFile::ChannelReader::ReadData(void* dest, uint64_t numBytes)
{
	// When an error condition is encountered, m_file is set to NULL.
	if(!m_file)
		return false;

	uint64_t availOut = numBytes;
	char* writePtr = (char*)dest;

	uint64_t streamEnd = m_chanInfo->GetDataOffset() + m_chanInfo->GetCompressedSize();

	while( (availOut > 0) && (m_readOffset < streamEnd) )
	{
		// We need to position the read pointer each time to notify the file reader that we've
		// consumed the bytes returned by GetBuffer().
		m_file->SetReadPtr(m_readOffset);

		// Get a pointer to the data present in the reader's cache.
		uint32_t bufferSize;
		m_inflateCtx.next_in = (Bytef*)m_file->GetBuffer(&bufferSize);
		if(!m_inflateCtx.next_in)
		{
			CloseFile();
			return false;
		}

		// Make sure we don't run past the end of the channel.
		uint64_t availIn = streamEnd - m_readOffset;
		if(availIn > bufferSize)
			availIn = bufferSize;

		m_inflateCtx.avail_in = (uInt)availIn;

		// zlib uses 32-bit sizes, so we need to clamp the size in case somebody really wants to
		// read more than 4 GB in one go. This isn't a problem for availIn because bufferSize is 32-bit.
		m_inflateCtx.avail_out = (uInt)((availOut < UINT_MAX) ? availOut : UINT_MAX);
		m_inflateCtx.next_out = (Bytef*)writePtr;

		int inflateResult = inflate(&m_inflateCtx, Z_NO_FLUSH);
		if( (inflateResult != Z_OK) && (inflateResult != Z_STREAM_END) )
		{
			CloseFile();
			return false;
		}

		uint32_t readBytes = (uint32_t)(availIn - m_inflateCtx.avail_in);
		m_readOffset += readBytes;

		if( (m_readOffset == streamEnd) && (inflateResult != Z_STREAM_END) )
		{
			// We've reached the end of the compressed data, but zlib doesn't think so.
			CloseFile();
			return false;
		}

		if( (inflateResult == Z_STREAM_END) && (m_readOffset != streamEnd) )
		{
			// zlib says the stream ended, but we have more data in the file.
			CloseFile();
			return false;
		}

		uint64_t writtenBytes = availOut - m_inflateCtx.avail_out;
		writePtr += writtenBytes;
		availOut -= writtenBytes;
	}

	if(availOut > 0)
	{
		// If we reached the end of the stream without filling up the output buffer, it's
		// an error on the caller's side.
		CloseFile();
		return false;
	}

	return true;
}

RPCFile::RPCFile()
{
	Init();
}

RPCFile::~RPCFile()
{
	Close();
}

void RPCFile::Init()
{
	m_file = NULL;
	m_ownFile = false;
	m_channels = NULL;
	m_numChannels = 0;
	m_numParticles = 0;
	m_fileVersion = 0;
	memset(m_bboxMin, 0, sizeof(m_bboxMin));
	memset(m_bboxMax, 0, sizeof(m_bboxMax));
}

void RPCFile::Close()
{
	delete[] m_channels;
	if(m_ownFile)
		delete m_file;

	Init();
}

bool RPCFile::Open(FileReader* file)
{
	Close();

	m_file = file;
	m_file->SetReadPtr(0);

	int32_t signature;
	bool headerOK =
		m_file->ReadInt32(&signature) &&
		m_file->ReadUInt32(&m_fileVersion) &&
		m_file->ReadUInt32(&m_numParticles) &&
		m_file->ReadUInt32(&m_numChannels) &&
		m_file->ReadFloatBuffer(m_bboxMin, 3) &&
		m_file->ReadFloatBuffer(m_bboxMax, 3);

	if( !headerOK || (signature != RPC_SIGNATURE) || (m_fileVersion < RPC_MIN_SUPPORTED_VERSION) || (m_fileVersion > RPC_MAX_SUPPORTED_VERSION) )
		return false;

	m_channels = new(std::nothrow) ChannelInfo[m_numChannels];
	if(!m_channels)
		return false;

	for(uint32_t chanIdx = 0; chanIdx < m_numChannels; ++chanIdx)
	{
		ChannelInfo* channel = &m_channels[chanIdx];

		uint32_t nameLen;
		if(!m_file->ReadUInt32(&nameLen) || (nameLen < 2))
			return false;

		channel->m_name = (char*)malloc(nameLen);
		if(!channel->m_name || !m_file->ReadData(channel->m_name, nameLen))
			return false;

		channel->m_name[nameLen - 1] = 0;

		uint32_t dataType;
		if(!m_file->ReadUInt32(&dataType) || (dataType < 1) || (dataType >= ChannelDataType_NUM) || !m_file->ReadUInt64(&channel->m_dataOffset) || !m_file->ReadUInt64(&channel->m_compressedSize))
			return false;

		channel->m_dataType = (ChannelDataType)dataType;

		uint32_t elemSize = GetDataTypeSize(channel->m_dataType);
		uint32_t compSize = elemSize / GetDataTypeNumComponents(channel->m_dataType);
		uint32_t limitsSize = 3*elemSize + 2*compSize;
		channel->m_valueLimits = malloc(limitsSize);
		if(!channel->m_valueLimits || !m_file->ReadData(channel->m_valueLimits, limitsSize))
			return false;

		channel->m_decompressedSize = m_numParticles * elemSize;
	}

	return true;
}

bool RPCFile::Open(const char* path)
{
	FileReader* file = new FileReader();
	if(!file->Open(path))
	{
		delete file;
		return false;
	}

	bool ret = Open(file);
	m_ownFile = true;
	return ret;
}

uint32_t RPCFile::GetDataTypeSize(ChannelDataType type)
{
	switch(type)
	{
		case ChannelDataType_Float3:	return 12;
		case ChannelDataType_Double3:	return 24;
		case ChannelDataType_Float2:	return 8;
		case ChannelDataType_UInt64:	return 8;
		case ChannelDataType_UInt16:	return 2;
		case ChannelDataType_UInt8:		return 1;
		case ChannelDataType_Int32:		return 4;
		case ChannelDataType_Bool:		return 1;
		case ChannelDataType_Float:		return 4;
		case ChannelDataType_Double:	return 8;
		case ChannelDataType_Float4:	return 16;
		default:						return 0;
	}
}

uint32_t RPCFile::GetDataTypeNumComponents(ChannelDataType type)
{
	switch(type)
	{
		case ChannelDataType_Float3:	return 3;
		case ChannelDataType_Double3:	return 3;
		case ChannelDataType_Float2:	return 2;
		case ChannelDataType_UInt64:	return 1;
		case ChannelDataType_UInt16:	return 1;
		case ChannelDataType_UInt8:		return 1;
		case ChannelDataType_Int32:		return 1;
		case ChannelDataType_Bool:		return 1;
		case ChannelDataType_Float:		return 1;
		case ChannelDataType_Double:	return 1;
		case ChannelDataType_Float4:	return 4;
		default:						return 0;
	}
}

int RPCFile::FindChannelIndex(const char* name) const
{
	for(uint32_t idx = 0; idx < m_numChannels; ++idx)
	{
		if(strcmp(m_channels[idx].GetName(), name) == 0)
			return (int)idx;
	}

	return -1;
}

const RPCFile::ChannelInfo* RPCFile::GetChannelInfo(unsigned int index) const
{
	return (index < m_numChannels) ? &m_channels[index] : NULL;
}

const RPCFile::ChannelInfo* RPCFile::GetChannelInfo(const char* name) const
{
	int idx = FindChannelIndex(name);
	return (idx >= 0) ? &m_channels[idx] : NULL;
}

RPCFile::ChannelReader* RPCFile::OpenChannel(unsigned int index, bool newFileStream) const
{
	if(index >= m_numChannels)
		return NULL;

	return new ChannelReader(m_file, &m_channels[index], newFileStream);
}

RPCFile::ChannelReader* RPCFile::OpenChannel(const char* name, bool newFileStream) const
{
	int idx = FindChannelIndex(name);
	return (idx >= 0) ? OpenChannel(idx, newFileStream) : NULL;
}

void* RPCFile::ReadWholeChannel(unsigned int index) const
{
	ChannelReader* reader = OpenChannel(index, false);
	if(!reader)
		return NULL;

	uint64_t dataSize = reader->GetDataSize();
	void* data = malloc((size_t)dataSize);
	if(data)
	{
		if(!reader->ReadData(data, dataSize))
		{
			free(data);
			data = NULL;
		}
	}

	delete reader;
	return data;
}

void* RPCFile::ReadWholeChannel(const char* name) const
{
	int idx = FindChannelIndex(name);
	return (idx >= 0) ? ReadWholeChannel(idx) : NULL;
}

EXIT_PARTIO_NAMESPACE

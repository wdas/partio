#ifndef RPC_READER_H
#define RPC_READER_H

#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <new>
#include <limits.h>

#define ZLIB_CONST
#include "zlib.h"

class FileReader;

class RPCFile
{
public:
	RPCFile();
	~RPCFile();

	enum ChannelDataType
	{
		ChannelDataType_Float3 = 1,
		ChannelDataType_Double3,
		ChannelDataType_Float2,
		ChannelDataType_UInt64,
		ChannelDataType_UInt16,
		ChannelDataType_UInt8,
		ChannelDataType_Int32,
		ChannelDataType_Bool,
		ChannelDataType_Float,
		ChannelDataType_Double,
		ChannelDataType_Float4,
		ChannelDataType_NUM
	};

	class ChannelInfo
	{
	public:
		~ChannelInfo();

		const char*			GetName() const { return m_name; }
		ChannelDataType		GetDataType() const { return m_dataType; }
		uint64_t			GetDataOffset() const { return m_dataOffset; }
		uint64_t			GetCompressedSize() const { return m_compressedSize; }
		uint64_t			GetDecompressedSize() const { return m_decompressedSize; }
		const void*			GetMinValue() const { return m_valueLimits; }
		const void*			GetMaxValue() const { return (const char*)m_valueLimits + GetDataTypeSize(m_dataType); }
		const void*			GetAvgValue() const { return (const char*)m_valueLimits + 2*GetDataTypeSize(m_dataType); }
		const void*			GetMinMagnitude() const { return (const char*)m_valueLimits + 3*GetDataTypeSize(m_dataType); }
		const void*			GetMaxMagnitude() const { return (const char*)m_valueLimits + 3*GetDataTypeSize(m_dataType) + GetDataTypeSize(m_dataType)/GetDataTypeNumComponents(m_dataType); }

	private:
		ChannelInfo();

		char*				m_name;
		ChannelDataType		m_dataType;
		uint64_t			m_dataOffset;
		uint64_t			m_compressedSize;
		uint64_t			m_decompressedSize;
		void*				m_valueLimits;

		friend class RPCFile;
	};

	class ChannelReader
	{
	public:
		~ChannelReader();

		uint64_t			GetDataSize() const { return m_chanInfo->GetDecompressedSize(); }
		uint32_t			GetElementSize() const { return GetDataTypeSize(m_chanInfo->GetDataType()); }
		bool				ReadData(void* dest, uint64_t destSize);

	private:
		ChannelReader(FileReader* file, const ChannelInfo* info, bool newFileStream);
		void				CloseFile();

		FileReader*			m_file;
		bool				m_ownFile;
		const ChannelInfo*	m_chanInfo;
		uint64_t			m_readOffset;
		z_stream			m_inflateCtx;

		friend class RPCFile;
	};

	bool					Open(const char* path);
	bool					Open(FileReader* file);
	void					Close();

	uint32_t				GetNumParticles() const { return m_numParticles; }
	const float*			GetBBoxMin() const { return m_bboxMin; }
	const float*			GetBBoxMax() const { return m_bboxMax; }

	static uint32_t			GetDataTypeSize(ChannelDataType type);
	static uint32_t			GetDataTypeNumComponents(ChannelDataType type);

	uint32_t				GetNumChannels() const { return m_numChannels; }
	int						FindChannelIndex(const char* name) const;

	// The returned pointer is owned by the RPCFile class, so it must not be freed.
	const ChannelInfo*		GetChannelInfo(unsigned int index) const;
	const ChannelInfo*		GetChannelInfo(const char* name) const;

	// These methods pass ownership of the returned pointer to the caller.
	ChannelReader*			OpenChannel(unsigned int index, bool newFileStream) const;
	ChannelReader*			OpenChannel(const char* name, bool newFileStream) const;
	void*					ReadWholeChannel(unsigned int index) const;
	void*					ReadWholeChannel(const char* name) const;

private:
	void					Init();

	mutable FileReader*		m_file;
	bool					m_ownFile;
	uint32_t				m_fileVersion;
	uint32_t				m_numParticles;
	uint32_t				m_numChannels;
	float					m_bboxMin[3];
	float					m_bboxMax[3];
	ChannelInfo*			m_channels;
};

#endif

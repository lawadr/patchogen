#include "common.h"
#include <zlib.h>
#include <bsdiff.h>
#include <bspatch.h>
#include <vector>

class OutZlibStream
{
public:
	OutZlibStream(std::ostream& stream)
		: m_buffer(bufferSize)
		, m_outStream(stream)
	{
		m_stream.avail_out = static_cast<uInt>(m_buffer.size());
		m_stream.next_out = m_buffer.data();
		deflateInit(&m_stream, 9);
	}

	~OutZlibStream()
	{
		while (deflate(&m_stream, Z_FINISH) != Z_STREAM_END)
		{
			flush();
		}

		flush();
		deflateEnd(&m_stream);
	}

	void write(const void* buffer, std::size_t size)
	{
		m_stream.avail_in = static_cast<uInt>(size);
		m_stream.next_in = static_cast<const Bytef*>(buffer);

		while (m_stream.avail_in > 0)
		{
			deflate(&m_stream, Z_NO_FLUSH);

			if (m_stream.avail_out == 0)
			{
				flush();
			}
		}
	}

private:
	static const std::size_t bufferSize = 10 * 1024 * 1024;
	z_stream m_stream{};
	std::vector<std::uint8_t> m_buffer;
	std::ostream& m_outStream;

	void flush()
	{
		m_outStream.write(reinterpret_cast<char*>(m_buffer.data()), m_buffer.size() - m_stream.avail_out);
		m_stream.avail_out = static_cast<uInt>(m_buffer.size());
		m_stream.next_out = m_buffer.data();
	}
};

std::vector<std::uint8_t> load(std::istream& stream)
{
	stream.seekg(0, std::ios::end);
	auto size = stream.tellg();
	stream.seekg(0, std::ios::beg);

	std::vector<std::uint8_t> buffer(size);
	stream.read(reinterpret_cast<char*>(buffer.data()), size);
	return buffer;
}

std::uint32_t crc32(std::istream& stream)
{
	auto crc = crc32(0, Z_NULL, 0);

	const size_t bufferSize = 1024 * 1024;
	std::vector<std::uint8_t> buffer(bufferSize);

	while (!stream.eof())
	{
		stream.read(reinterpret_cast<char*>(buffer.data()), bufferSize);
		crc = crc32(crc, buffer.data(), static_cast<uInt>(stream.gcount()));
	}

	return crc;
}

void zip(std::istream& inStream, std::ostream& outStream)
{
	const size_t bufferSize = 10 * 1024 * 1024;
	std::vector<std::uint8_t> buffer(bufferSize);

	inStream.seekg(0, std::ios::end);
	std::uint64_t size = inStream.tellg();
	outStream.write(reinterpret_cast<char*>(&size), sizeof(size));
	inStream.seekg(0);

	OutZlibStream stream(outStream);

	while (!inStream.eof())
	{
		inStream.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
		stream.write(buffer.data(), inStream.gcount());
	}
}

void unzip(std::istream& inStream, std::ostream& outStream)
{
	auto buffer = load(inStream);
	unzip(buffer, outStream);
}

void unzip(const std::vector<std::uint8_t>& inBuffer, std::ostream& outStream)
{
	std::vector<std::uint8_t> outBuffer(*reinterpret_cast<const uint64_t*>(inBuffer.data()));

	z_stream stream{};
	inflateInit(&stream);
	stream.next_in = inBuffer.data() + sizeof(std::uint64_t);
	stream.avail_in = static_cast<uInt>(inBuffer.size() - sizeof(std::uint64_t));
	stream.next_out = outBuffer.data();
	stream.avail_out = static_cast<uInt>(outBuffer.size());

	inflate(&stream, Z_FINISH);

	outStream.write(reinterpret_cast<char*>(outBuffer.data()), outBuffer.size() - stream.avail_out);
}

int write(bsdiff_stream* bsStream, const void* buffer, int size)
{
	static_cast<OutZlibStream*>(bsStream->opaque)->write(buffer, size);
	return 0;
}

void diff(std::istream& oldStream, std::istream& newStream, std::ostream& outStream)
{
	// Load the old and new files in full
	auto oldBuffer = load(oldStream);
	auto newBuffer = load(newStream);

	// Write the size of the new file to the start of the output stream
	std::uint64_t newSize = newBuffer.size();
	outStream.write(reinterpret_cast<char*>(&newSize), sizeof(newSize));

	// Set up the zlib compression stream
	OutZlibStream zlibStream(outStream);

	// Create the diff
	bsdiff_stream bsStream{};
	bsStream.malloc = malloc;
	bsStream.free = free;
	bsStream.write = write;
	bsStream.opaque = &zlibStream;

	bsdiff(oldBuffer.data(), oldBuffer.size(), newBuffer.data(), newBuffer.size(), &bsStream);
}

int read(const bspatch_stream* bsStream, void* buffer, int size)
{
	auto zStream = static_cast<z_stream*>(bsStream->opaque);

	zStream->next_out = static_cast<unsigned char*>(buffer);
	zStream->avail_out = size;

	auto result = inflate(zStream, Z_NO_FLUSH);

	return 0;
}

void patch(const std::vector<std::uint8_t>& patchBuffer, std::istream& inStream, std::ostream& outStream)
{
	// Load the old file in full and allocate buffer for the new file
	std::vector<uint8_t> inBuffer = load(inStream);
	std::vector<uint8_t> outBuffer(*reinterpret_cast<const uint64_t*>(patchBuffer.data()));

	// Set up the zlib decompression stream
	z_stream zStream{};
	zStream.next_in = patchBuffer.data() + sizeof(uint64_t);
	zStream.avail_in = static_cast<uInt>(patchBuffer.size() - sizeof(uint64_t));
	inflateInit(&zStream);

	// Patch the old file
	bspatch_stream bsStream;
	bsStream.opaque = &zStream;
	bsStream.read = read;

	if (bspatch(inBuffer.data(), inBuffer.size(), outBuffer.data(), outBuffer.size(), &bsStream) != 0)
	{
		inflateEnd(&zStream);
		return;
	}

	inflate(&zStream, Z_FINISH);
	inflateEnd(&zStream);

	outStream.write(reinterpret_cast<char*>(outBuffer.data()), outBuffer.size());
}

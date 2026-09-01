#include "zstd_compressor.h"

#include <vector>

#include <zstd.h>

namespace zstd_comp {

bool compress(const std::string& input, std::string& output, int level)
{
	std::vector<char> buf(ZSTD_compressBound(input.size()));
	size_t result = ZSTD_compress(buf.data(), buf.size(), input.data(), input.size(), level);
	if (ZSTD_isError(result))
		return false;
	if (result >= input.size())
		return false; // not worthwhile, send raw
	output.assign(buf.data(), result);
	return true;
}

bool decompress(const std::string& input, std::string& output)
{
	unsigned long long content = ZSTD_getFrameContentSize(input.data(), input.size());
	if (content != ZSTD_CONTENTSIZE_UNKNOWN && content != ZSTD_CONTENTSIZE_ERROR)
	{
		std::vector<char> buf(content);
		size_t result = ZSTD_decompress(buf.data(), buf.size(), input.data(), input.size());
		if (ZSTD_isError(result))
			return false;
		output.assign(buf.data(), result);
		return true;
	}

	// Fall back to a streaming decode when the content size is not embedded.
	ZSTD_DStream *stream = ZSTD_createDStream();
	if (!stream)
		return false;
	ZSTD_initDStream(stream);

	ZSTD_inBuffer in{ input.data(), input.size(), 0 };
	const size_t step = ZSTD_DStreamOutSize();
	std::vector<char> tmp(step);
	ZSTD_outBuffer out{ tmp.data(), step, 0 };

	bool ok = true;
	size_t remaining = 0;
	do
	{
		remaining = ZSTD_decompressStream(stream, &out, &in);
		if (ZSTD_isError(remaining))
		{
			ok = false;
			break;
		}
		output.append(tmp.data(), out.pos);
		out.pos = 0;
	} while (remaining != 0);

	ZSTD_freeDStream(stream);
	return ok;
}

} // namespace zstd_comp
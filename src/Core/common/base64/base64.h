#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Minimal RFC 4648 base64 helpers. File bytes travel inside a JSON message
// envelope, which cannot carry raw binary, so they are base64-encoded before
// being placed in the payload and decoded on the receiving side.
namespace base64 {

inline std::string encode(const std::string& in)
{
	static const char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string out;
	out.reserve(((in.size() + 2) / 3) * 4);
	for (size_t i = 0; i < in.size(); i += 3)
	{
		uint32_t n = (static_cast<uint8_t>(in[i]) << 16);
		if (i + 1 < in.size())
			n |= (static_cast<uint8_t>(in[i + 1]) << 8);
		if (i + 2 < in.size())
			n |= static_cast<uint8_t>(in[i + 2]);

		out.push_back(tbl[(n >> 18) & 0x3F]);
		out.push_back(tbl[(n >> 12) & 0x3F]);
		out.push_back(i + 1 < in.size() ? tbl[(n >> 6) & 0x3F] : '=');
		out.push_back(i + 2 < in.size() ? tbl[n & 0x3F] : '=');
	}
	return out;
}

inline std::string decode(const std::string& in)
{
	auto val = [](char c) -> int {
		if (c >= 'A' && c <= 'Z')
			return c - 'A';
		if (c >= 'a' && c <= 'z')
			return c - 'a' + 26;
		if (c >= '0' && c <= '9')
			return c - '0' + 52;
		if (c == '+')
			return 62;
		if (c == '/')
			return 63;
		return -1;
	};

	std::string out;
	out.reserve(in.size() * 3 / 4);
	uint32_t buf = 0;
	int bits = 0;
	for (char c : in)
	{
		if (c == '=')
			break;
		int v = val(c);
		if (v < 0)
			continue;
		buf = (buf << 6) | static_cast<uint32_t>(v);
		bits += 6;
		if (bits >= 8)
		{
			bits -= 8;
			out.push_back(static_cast<char>((buf >> bits) & 0xFF));
		}
	}
	return out;
}

} // namespace base64

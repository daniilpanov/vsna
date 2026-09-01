#pragma once
#include <string>

// Thin one-shot wrappers around the zstd C library for per-chunk (de)compression
// during a transfer. Compression is best-effort: compress() reports whether the
// output is actually smaller than the input so the caller can fall back to
// sending the chunk raw.
namespace zstd_comp {
constexpr int kDefaultLevel{ 3 };
constexpr int kMinLevel{ 1 };
constexpr int kMaxLevel{ 19 };

// Compress input at the given level. Returns false (and leaves output empty) if
// compression fails or would not save space.
bool compress(const std::string& input, std::string& output, int level);

// Decompress a zstd frame. Returns false on malformed input.
bool decompress(const std::string& input, std::string& output);
} // namespace zstd_comp
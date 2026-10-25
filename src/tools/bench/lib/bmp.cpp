#include "enh/bench/bmp.hpp"

#include "enh/pipeline/core/checked_math.hpp"

#include <cstring>
#include <string>
#include <utility>

namespace enh::bench {

namespace {

using pipeline::fail;
using pipeline::PipelineErrorCode;
using pipeline::Result;

constexpr std::size_t kFileHeaderBytes = 14;
constexpr std::uint32_t kInfoHeaderBytes = 40; // BITMAPINFOHEADER
constexpr std::uint32_t kV4HeaderBytes = 108;  // BITMAPV4HEADER
constexpr std::uint32_t kV5HeaderBytes = 124;  // BITMAPV5HEADER
constexpr std::uint32_t kBiRgb = 0;
constexpr std::uint32_t kBiBitfields = 3;
constexpr std::size_t kMaskBytes = 12; // masques R, G, B apres un en-tete de 40 octets

std::uint16_t le16(std::span<const std::byte> b, std::size_t at) {
    return static_cast<std::uint16_t>(std::to_integer<unsigned>(b[at]) |
                                      (std::to_integer<unsigned>(b[at + 1]) << 8U));
}

std::uint32_t le32(std::span<const std::byte> b, std::size_t at) {
    return std::to_integer<std::uint32_t>(b[at]) |
           (std::to_integer<std::uint32_t>(b[at + 1]) << 8U) |
           (std::to_integer<std::uint32_t>(b[at + 2]) << 16U) |
           (std::to_integer<std::uint32_t>(b[at + 3]) << 24U);
}

std::int32_t les32(std::span<const std::byte> b, std::size_t at) {
    const std::uint32_t u = le32(b, at);
    std::int32_t s = 0;
    std::memcpy(&s, &u, sizeof s);
    return s;
}

void put16(std::vector<std::byte>& out, std::size_t at, std::uint16_t v) {
    out[at] = static_cast<std::byte>(v & 0xFFU);
    out[at + 1] = static_cast<std::byte>(v >> 8U);
}

void put32(std::vector<std::byte>& out, std::size_t at, std::uint32_t v) {
    for (unsigned k = 0; k < 4; ++k) {
        out[at + k] = static_cast<std::byte>((v >> (8U * k)) & 0xFFU);
    }
}

/// Octets par ligne d'un BMP (lignes alignees sur 4 octets), avec verification.
Result<std::size_t> rowStride(std::uint32_t width, std::uint32_t bits_per_pixel) {
    auto bits = pipeline::checkedMul(width, bits_per_pixel);
    if (!bits) {
        return bits;
    }
    auto padded = pipeline::checkedAdd(*bits, 31);
    if (!padded) {
        return padded;
    }
    return (*padded / 32) * 4;
}

} // namespace

Result<RgbImage> decodeBmp(std::span<const std::byte> file) {
    if (file.size() < kFileHeaderBytes + kInfoHeaderBytes) {
        return fail(PipelineErrorCode::SourceCorrupt,
                    "bmp: file too short (" + std::to_string(file.size()) + " bytes)");
    }
    if (file[0] != std::byte{'B'} || file[1] != std::byte{'M'}) {
        return fail(PipelineErrorCode::UnsupportedFormat, "bmp: missing 'BM' signature");
    }
    if (file.size() > kMaxBmpFileBytes) {
        return fail(PipelineErrorCode::UnsupportedFormat, "bmp: file too large");
    }
    const std::uint32_t data_offset = le32(file, 10);
    const std::uint32_t dib_size = le32(file, 14);
    if (dib_size != kInfoHeaderBytes && dib_size != kV4HeaderBytes && dib_size != kV5HeaderBytes) {
        return fail(PipelineErrorCode::UnsupportedFormat,
                    "bmp: unsupported DIB header size " + std::to_string(dib_size));
    }
    if (file.size() < kFileHeaderBytes + dib_size) {
        return fail(PipelineErrorCode::SourceCorrupt, "bmp: truncated DIB header");
    }
    const std::int32_t width = les32(file, 18);
    const std::int32_t height_signed = les32(file, 22);
    const std::uint16_t planes = le16(file, 26);
    const std::uint16_t bpp = le16(file, 28);
    const std::uint32_t compression = le32(file, 30);

    // Hauteur negative = lignes de haut en bas ; valeur absolue en 64 bits (INT32_MIN compris).
    const std::int64_t height_abs =
        height_signed < 0 ? -std::int64_t{height_signed} : std::int64_t{height_signed};
    if (width < 1 || std::cmp_greater(width, kMaxBmpSidePx) || height_abs < 1 ||
        std::cmp_greater(height_abs, kMaxBmpSidePx)) {
        return fail(PipelineErrorCode::SourceCorrupt,
                    "bmp: dimensions out of bounds (" + std::to_string(width) + "x" +
                        std::to_string(height_signed) + ", max 8192)");
    }
    if (planes != 1) {
        return fail(PipelineErrorCode::SourceCorrupt, "bmp: planes != 1");
    }
    if (bpp != 24 && bpp != 32) {
        return fail(PipelineErrorCode::UnsupportedFormat,
                    "bmp: " + std::to_string(bpp) + " bits per pixel (24 or 32 expected)");
    }
    std::size_t header_end = kFileHeaderBytes + dib_size;
    if (compression == kBiBitfields) {
        if (bpp != 32) {
            return fail(PipelineErrorCode::UnsupportedFormat, "bmp: BI_BITFIELDS needs 32 bits");
        }
        const std::size_t masks_at = kFileHeaderBytes + kInfoHeaderBytes;
        if (dib_size == kInfoHeaderBytes) {
            header_end += kMaskBytes;
            if (file.size() < header_end) {
                return fail(PipelineErrorCode::SourceCorrupt, "bmp: truncated color masks");
            }
        }
        if (le32(file, masks_at) != 0x00FF0000U || le32(file, masks_at + 4) != 0x0000FF00U ||
            le32(file, masks_at + 8) != 0x000000FFU) {
            return fail(PipelineErrorCode::UnsupportedFormat, "bmp: non standard color masks");
        }
    } else if (compression != kBiRgb) {
        return fail(PipelineErrorCode::UnsupportedFormat,
                    "bmp: compression " + std::to_string(compression) + " not supported");
    }

    const auto w = static_cast<std::uint32_t>(width);
    const auto h = static_cast<std::uint32_t>(height_abs);
    const bool bottom_up = height_signed > 0;
    auto stride = rowStride(w, bpp);
    if (!stride) {
        return enh::Unexpected<pipeline::PipelineError>(std::move(stride.error()));
    }
    auto data_bytes = pipeline::checkedMul(*stride, h);
    if (!data_bytes) {
        return enh::Unexpected<pipeline::PipelineError>(std::move(data_bytes.error()));
    }
    auto data_end = pipeline::checkedAdd(data_offset, *data_bytes);
    if (!data_end) {
        return enh::Unexpected<pipeline::PipelineError>(std::move(data_end.error()));
    }
    if (data_offset < header_end || *data_end > file.size()) {
        return fail(PipelineErrorCode::SourceCorrupt,
                    "bmp: pixel data out of file bounds (offset " + std::to_string(data_offset) +
                        ", " + std::to_string(*data_bytes) + " bytes, file " +
                        std::to_string(file.size()) + ")");
    }

    RgbImage img;
    img.size_px = {.width = w, .height = h};
    auto out_bytes = pipeline::checkedMul(std::size_t{w} * 3, h);
    if (!out_bytes) {
        return enh::Unexpected<pipeline::PipelineError>(std::move(out_bytes.error()));
    }
    img.rgb.resize(*out_bytes);
    const std::size_t bytes_pp = bpp / 8U;
    for (std::size_t y = 0; y < h; ++y) {
        const std::size_t src_row = bottom_up ? (h - 1 - y) : y;
        const std::span<const std::byte> row =
            file.subspan(data_offset + src_row * *stride, *stride);
        std::uint8_t* dst = img.rgb.data() + y * w * 3;
        for (std::size_t x = 0; x < w; ++x) {
            const std::size_t at = x * bytes_pp; // B, G, R (, X)
            dst[x * 3 + 0] = std::to_integer<std::uint8_t>(row[at + 2]);
            dst[x * 3 + 1] = std::to_integer<std::uint8_t>(row[at + 1]);
            dst[x * 3 + 2] = std::to_integer<std::uint8_t>(row[at + 0]);
        }
    }
    return img;
}

Result<std::vector<std::byte>> encodeBmp24(const RgbImage& image) {
    const std::uint32_t w = image.size_px.width;
    const std::uint32_t h = image.size_px.height;
    if (w < 1 || h < 1 || w > kMaxBmpSidePx || h > kMaxBmpSidePx ||
        image.rgb.size() != std::size_t{w} * h * 3) {
        return fail(PipelineErrorCode::InvalidParams, "bmp: inconsistent image to encode");
    }
    auto stride = rowStride(w, 24);
    if (!stride) {
        return enh::Unexpected<pipeline::PipelineError>(std::move(stride.error()));
    }
    const std::size_t data_offset = kFileHeaderBytes + kInfoHeaderBytes;
    const std::size_t data_bytes = *stride * h; // <= 8192 * 8192 * 3 + marge : pas de debordement
    std::vector<std::byte> out(data_offset + data_bytes, std::byte{0});
    out[0] = std::byte{'B'};
    out[1] = std::byte{'M'};
    put32(out, 2, static_cast<std::uint32_t>(out.size()));
    put32(out, 10, static_cast<std::uint32_t>(data_offset));
    put32(out, 14, kInfoHeaderBytes);
    put32(out, 18, w);
    put32(out, 22, h); // positif : de bas en haut
    put16(out, 26, 1);
    put16(out, 28, 24);
    put32(out, 30, kBiRgb);
    put32(out, 34, static_cast<std::uint32_t>(data_bytes));
    put32(out, 38, 2835); // 72 ppp
    put32(out, 42, 2835);
    for (std::size_t y = 0; y < h; ++y) {
        const std::uint8_t* src = image.rgb.data() + (h - 1 - y) * w * 3;
        std::byte* dst = out.data() + data_offset + y * *stride;
        for (std::size_t x = 0; x < w; ++x) {
            dst[x * 3 + 0] = static_cast<std::byte>(src[x * 3 + 2]);
            dst[x * 3 + 1] = static_cast<std::byte>(src[x * 3 + 1]);
            dst[x * 3 + 2] = static_cast<std::byte>(src[x * 3 + 0]);
        }
    }
    return out;
}

std::vector<std::uint32_t> toArgb8(const RgbImage& image) {
    std::vector<std::uint32_t> px(image.rgb.size() / 3);
    for (std::size_t i = 0; i < px.size(); ++i) {
        px[i] = std::uint32_t{image.rgb[i * 3]} | (std::uint32_t{image.rgb[i * 3 + 1]} << 8U) |
                (std::uint32_t{image.rgb[i * 3 + 2]} << 16U) | (0xFFU << 24U);
    }
    return px;
}

std::vector<std::uint32_t> toAbgr10(const RgbImage& image) {
    const auto up = [](std::uint8_t v) { return (std::uint32_t{v} * 1023U + 127U) / 255U; };
    std::vector<std::uint32_t> px(image.rgb.size() / 3);
    for (std::size_t i = 0; i < px.size(); ++i) {
        px[i] = up(image.rgb[i * 3]) | (up(image.rgb[i * 3 + 1]) << 10U) |
                (up(image.rgb[i * 3 + 2]) << 20U) | (3U << 30U);
    }
    return px;
}

RgbImage fromArgb8(std::span<const std::uint32_t> pixels, pipeline::Size size_px) {
    RgbImage img;
    img.size_px = size_px;
    img.rgb.resize(pixels.size() * 3);
    for (std::size_t i = 0; i < pixels.size(); ++i) {
        img.rgb[i * 3] = static_cast<std::uint8_t>(pixels[i] & 0xFFU);
        img.rgb[i * 3 + 1] = static_cast<std::uint8_t>((pixels[i] >> 8U) & 0xFFU);
        img.rgb[i * 3 + 2] = static_cast<std::uint8_t>((pixels[i] >> 16U) & 0xFFU);
    }
    return img;
}

RgbImage fromAbgr10(std::span<const std::uint32_t> pixels, pipeline::Size size_px) {
    const auto down = [](std::uint32_t v10) {
        return static_cast<std::uint8_t>(((v10 & 0x3FFU) * 255U + 511U) / 1023U);
    };
    RgbImage img;
    img.size_px = size_px;
    img.rgb.resize(pixels.size() * 3);
    for (std::size_t i = 0; i < pixels.size(); ++i) {
        img.rgb[i * 3] = down(pixels[i]);
        img.rgb[i * 3 + 1] = down(pixels[i] >> 10U);
        img.rgb[i * 3 + 2] = down(pixels[i] >> 20U);
    }
    return img;
}

} // namespace enh::bench

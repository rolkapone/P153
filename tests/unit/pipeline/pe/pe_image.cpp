#include "pe/pe_image.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <utility>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace enh::tests {

namespace {

class Image {
public:
    explicit Image(std::vector<char> bytes) : bytes_(std::move(bytes)) {}

    template <typename T> [[nodiscard]] std::optional<T> at(std::size_t offset) const {
        if (offset > bytes_.size() || bytes_.size() - offset < sizeof(T)) {
            return std::nullopt;
        }
        T v{};
        std::memcpy(&v, bytes_.data() + offset, sizeof(T));
        return v;
    }

    [[nodiscard]] std::optional<std::string> cstring(std::size_t offset) const {
        if (offset >= bytes_.size()) {
            return std::nullopt;
        }
        const char* begin = bytes_.data() + offset;
        const auto* end = static_cast<const char*>(std::memchr(begin, 0, bytes_.size() - offset));
        if (end == nullptr) {
            return std::nullopt;
        }
        return std::string(begin, end);
    }

    /// RVA -> decalage dans le fichier (table des sections).
    [[nodiscard]] std::optional<std::size_t> offsetOf(std::uint32_t rva) const {
        for (const IMAGE_SECTION_HEADER& s : sections_) {
            const std::uint32_t size = std::max(s.Misc.VirtualSize, s.SizeOfRawData);
            if (rva >= s.VirtualAddress && rva < s.VirtualAddress + size) {
                return std::size_t{s.PointerToRawData} + (rva - s.VirtualAddress);
            }
        }
        return std::nullopt;
    }

    void addSection(const IMAGE_SECTION_HEADER& s) { sections_.push_back(s); }

private:
    std::vector<char> bytes_;
    std::vector<IMAGE_SECTION_HEADER> sections_;
};

std::vector<std::string> importNames(const Image& img, const IMAGE_DATA_DIRECTORY& dir) {
    std::vector<std::string> out;
    if (dir.VirtualAddress == 0) {
        return out;
    }
    auto off = img.offsetOf(dir.VirtualAddress);
    for (std::size_t i = 0; off && i < 4096; ++i) {
        auto d = img.at<IMAGE_IMPORT_DESCRIPTOR>(*off + i * sizeof(IMAGE_IMPORT_DESCRIPTOR));
        if (!d || d->Name == 0) {
            break;
        }
        if (auto name_off = img.offsetOf(d->Name)) {
            if (auto name = img.cstring(*name_off)) {
                out.push_back(*name);
            }
        }
    }
    return out;
}

std::vector<std::string> delayImportNames(const Image& img, const IMAGE_DATA_DIRECTORY& dir) {
    std::vector<std::string> out;
    if (dir.VirtualAddress == 0) {
        return out;
    }
    auto off = img.offsetOf(dir.VirtualAddress);
    for (std::size_t i = 0; off && i < 4096; ++i) {
        auto d = img.at<IMAGE_DELAYLOAD_DESCRIPTOR>(*off + i * sizeof(IMAGE_DELAYLOAD_DESCRIPTOR));
        if (!d || d->DllNameRVA == 0) {
            break;
        }
        if (auto name_off = img.offsetOf(d->DllNameRVA)) {
            if (auto name = img.cstring(*name_off)) {
                out.push_back(*name);
            }
        }
    }
    return out;
}

std::vector<std::string> exportNames(const Image& img, const IMAGE_DATA_DIRECTORY& dir) {
    std::vector<std::string> out;
    if (dir.VirtualAddress == 0) {
        return out;
    }
    auto off = img.offsetOf(dir.VirtualAddress);
    auto e = off ? img.at<IMAGE_EXPORT_DIRECTORY>(*off) : std::nullopt;
    if (!e) {
        return out;
    }
    auto names = img.offsetOf(e->AddressOfNames);
    for (std::uint32_t i = 0; names && i < e->NumberOfNames && i < 65536; ++i) {
        auto rva = img.at<std::uint32_t>(*names + std::size_t{i} * 4);
        if (!rva) {
            break;
        }
        if (auto name_off = img.offsetOf(*rva)) {
            if (auto name = img.cstring(*name_off)) {
                out.push_back(*name);
            }
        }
    }
    return out;
}

} // namespace

std::optional<PeInfo> readPe(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        return std::nullopt;
    }
    Image img(std::vector<char>(std::istreambuf_iterator<char>(in), {}));
    auto dos = img.at<IMAGE_DOS_HEADER>(0);
    if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0) {
        return std::nullopt;
    }
    const auto nt_off = static_cast<std::size_t>(dos->e_lfanew);
    auto nt = img.at<IMAGE_NT_HEADERS64>(nt_off);
    if (!nt || nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        return std::nullopt;
    }
    const std::size_t sec_off =
        nt_off + offsetof(IMAGE_NT_HEADERS64, OptionalHeader) + nt->FileHeader.SizeOfOptionalHeader;
    for (std::size_t i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        auto s = img.at<IMAGE_SECTION_HEADER>(sec_off + i * sizeof(IMAGE_SECTION_HEADER));
        if (!s) {
            return std::nullopt;
        }
        img.addSection(*s);
    }
    const auto& dirs = nt->OptionalHeader.DataDirectory;
    PeInfo info;
    info.imports = importNames(img, dirs[IMAGE_DIRECTORY_ENTRY_IMPORT]);
    info.delay_imports = delayImportNames(img, dirs[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT]);
    info.exports = exportNames(img, dirs[IMAGE_DIRECTORY_ENTRY_EXPORT]);
    return info;
}

} // namespace enh::tests

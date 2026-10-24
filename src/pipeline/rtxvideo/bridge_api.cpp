#include "enh/pipeline/rtxvideo/bridge_api.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <utility>

namespace enh::pipeline::rtxvideo {

namespace {

// Codes NVSDK_NGX_Result (nvsdk_ngx_defs.h, SDK 1.1.0) : 0xBAD00000 | n.
constexpr std::uint32_t kNgxFail = RVB_NGX_FAIL_BASE;
constexpr std::uint32_t kFeatureNotSupported = kNgxFail | 1U;
constexpr std::uint32_t kPlatformError = kNgxFail | 2U;
constexpr std::uint32_t kFeatureNotFound = kNgxFail | 4U;
constexpr std::uint32_t kInvalidParameter = kNgxFail | 5U;
constexpr std::uint32_t kUnsupportedInputFormat = kNgxFail | 8U;
constexpr std::uint32_t kRwFlagMissing = kNgxFail | 9U;
constexpr std::uint32_t kMissingInput = kNgxFail | 10U;
constexpr std::uint32_t kOutOfDate = kNgxFail | 12U;
constexpr std::uint32_t kOutOfGpuMemory = kNgxFail | 13U;
constexpr std::uint32_t kUnsupportedFormat = kNgxFail | 14U;
constexpr std::uint32_t kUnableToWriteToAppDataPath = kNgxFail | 15U;
constexpr std::uint32_t kUnsupportedParameter = kNgxFail | 16U;
constexpr std::uint32_t kDenied = kNgxFail | 17U;

constexpr std::array<std::string_view, 19> kNgxNames = {
    "Fail",
    "FAIL_FeatureNotSupported",
    "FAIL_PlatformError",
    "FAIL_FeatureAlreadyExists",
    "FAIL_FeatureNotFound",
    "FAIL_InvalidParameter",
    "FAIL_ScratchBufferTooSmall",
    "FAIL_NotInitialized",
    "FAIL_UnsupportedInputFormat",
    "FAIL_RWFlagMissing",
    "FAIL_MissingInput",
    "FAIL_UnableToInitializeFeature",
    "FAIL_OutOfDate",
    "FAIL_OutOfGPUMemory",
    "FAIL_UnsupportedFormat",
    "FAIL_UnableToWriteToAppDataPath",
    "FAIL_UnsupportedParameter",
    "FAIL_Denied",
    "FAIL_NotImplemented",
};

std::string hex(std::uint32_t v) {
    constexpr std::string_view kDigits = "0123456789ABCDEF";
    std::string s = "0x";
    for (int shift = 28; shift >= 0; shift -= 4) {
        s += kDigits[(v >> static_cast<unsigned>(shift)) & 0xFU];
    }
    return s;
}

} // namespace

std::string_view resultName(rvb_result r) noexcept {
    switch (r) {
    case RVB_OK:
        return "Success";
    case RVB_E_INVALID_ARG:
        return "RVB_E_INVALID_ARG";
    case RVB_E_BAD_STRUCT_SIZE:
        return "RVB_E_BAD_STRUCT_SIZE";
    case RVB_E_ALREADY_INITIALIZED:
        return "RVB_E_ALREADY_INITIALIZED";
    case RVB_E_INTERNAL:
        return "RVB_E_INTERNAL";
    case RVB_E_BUSY:
        return "RVB_E_BUSY";
    case RVB_E_CUDA:
        return "RVB_E_CUDA";
    default:
        break;
    }
    if ((r & RVB_NGX_FAIL_MASK) == RVB_NGX_FAIL_BASE) {
        const std::uint32_t index = r & 0xFFFFFU;
        return index < kNgxNames.size() ? kNgxNames.at(index) : "FAIL_Unknown";
    }
    return succeeded(r) ? "Success" : "Unknown";
}

PipelineError bridgeFailure(rvb_result r, std::string_view what, std::source_location where) {
    PipelineErrorCode code = PipelineErrorCode::Internal;
    Component component = Component::None;
    switch (r) {
    case kFeatureNotSupported:
    case kPlatformError:
    case kFeatureNotFound:
    case kDenied:
        code = PipelineErrorCode::ComponentMissing;
        component = Component::RtxVideoSdk;
        break;
    case kOutOfDate:
        code = PipelineErrorCode::ComponentMissing;
        component = Component::Driver;
        break;
    case kOutOfGpuMemory:
        code = PipelineErrorCode::OutOfVram;
        break;
    case kInvalidParameter:
    case kUnsupportedInputFormat:
    case kRwFlagMissing:
    case kMissingInput:
    case kUnsupportedFormat:
    case kUnsupportedParameter:
    case RVB_E_INVALID_ARG:
        code = PipelineErrorCode::InvalidParams;
        break;
    case kUnableToWriteToAppDataPath:
        code = PipelineErrorCode::WriteDenied;
        break;
    case RVB_E_ALREADY_INITIALIZED:
    case RVB_E_BUSY:
        code = PipelineErrorCode::Busy;
        break;
    default:
        break;
    }
    std::string detail(what);
    detail += ": ";
    detail += hex(r);
    detail += " (";
    detail += resultName(r);
    detail += ')';
    PipelineError e = makeError(code, std::move(detail), static_cast<std::int64_t>(r), where);
    e.component = component;
    return e;
}

} // namespace enh::pipeline::rtxvideo

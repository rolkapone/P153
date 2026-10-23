#include "enh/pipeline/rtxvideo/ngx_guard.hpp"

#include "enh/pipeline/core/log.hpp"

#include <string>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace enh::pipeline::rtxvideo {

namespace {

/// Job Object du processus (cree et rattache une seule fois, garde jusqu'a la fin du processus).
struct ProcessJob {
    std::mutex mutex;
    HANDLE job = nullptr;
    bool attached = false;
    bool tried = false;
    int depth = 0;
    DWORD processes_at_attach = 0;
};

ProcessJob& processJob() {
    static ProcessJob instance;
    return instance;
}

bool setActiveProcessLimit(HANDLE job, bool limit) noexcept {
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
    if (limit) {
        info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
        info.BasicLimitInformation.ActiveProcessLimit = 1; // ce processus seul
    }
    return SetInformationJobObject(job, JobObjectExtendedLimitInformation, &info, sizeof info) !=
           FALSE;
}

DWORD totalProcesses(HANDLE job) noexcept {
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION acc{};
    if (QueryInformationJobObject(job, JobObjectBasicAccountingInformation, &acc, sizeof acc,
                                  nullptr) == FALSE) {
        return 0;
    }
    return acc.TotalProcesses;
}

} // namespace

ChildProcessBlocker::ChildProcessBlocker() noexcept {
    ProcessJob& pj = processJob();
    const std::scoped_lock lock(pj.mutex);
    if (!pj.tried) {
        pj.tried = true;
        pj.job = CreateJobObjectW(nullptr, nullptr);
        if (pj.job != nullptr && AssignProcessToJobObject(pj.job, GetCurrentProcess()) != FALSE) {
            pj.attached = true;
            pj.processes_at_attach = totalProcesses(pj.job);
        } else {
            logLine(LogLevel::Warn, "NgxUpdaterNotBlocked", "Job Object unavailable");
        }
    }
    if (!pj.attached) {
        return;
    }
    if (pj.depth == 0 && !setActiveProcessLimit(pj.job, true)) {
        logLine(LogLevel::Warn, "NgxUpdaterNotBlocked", "SetInformationJobObject failed");
        return;
    }
    ++pj.depth;
    active_ = true;
}

ChildProcessBlocker::~ChildProcessBlocker() {
    if (!active_) {
        return;
    }
    ProcessJob& pj = processJob();
    const std::scoped_lock lock(pj.mutex);
    if (--pj.depth == 0) {
        (void)setActiveProcessLimit(pj.job, false);
    }
}

std::uint32_t ChildProcessBlocker::childProcessesCreated() noexcept {
    ProcessJob& pj = processJob();
    const std::scoped_lock lock(pj.mutex);
    if (!pj.attached) {
        return 0;
    }
    const DWORD total = totalProcesses(pj.job);
    return total > pj.processes_at_attach ? total - pj.processes_at_attach : 0;
}

void NgxLogObserver::onLine(std::string_view line) {
    constexpr std::string_view kMarker = "feature vsr snippet: ";
    constexpr std::string_view kVersion = " version: ";
    const auto start = line.find(kMarker);
    if (start == std::string_view::npos) {
        return;
    }
    const std::string_view rest = line.substr(start + kMarker.size());
    const auto ver = rest.rfind(kVersion);
    const std::string_view path = rest.substr(0, ver);
    std::string_view version =
        ver == std::string_view::npos ? std::string_view{} : rest.substr(ver + kVersion.size());
    while (!version.empty() &&
           (version.back() == '\n' || version.back() == '\r' || version.back() == ' ')) {
        version.remove_suffix(1);
    }
    if (path.empty()) {
        return;
    }
    // Chemin en UTF-8 / ANSI : conversion par le systeme (MultiByteToWideChar, page de code ACP).
    const int n =
        MultiByteToWideChar(CP_ACP, 0, path.data(), static_cast<int>(path.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(n > 0 ? n : 0), L'\0');
    if (n > 0) {
        MultiByteToWideChar(CP_ACP, 0, path.data(), static_cast<int>(path.size()), wide.data(), n);
    }
    const std::scoped_lock lock(mutex_);
    vsr_.path = std::filesystem::path(wide).make_preferred();
    vsr_.version = std::string(version);
}

NgxSnippetInfo NgxLogObserver::vsrSnippet() const {
    const std::scoped_lock lock(mutex_);
    return vsr_;
}

} // namespace enh::pipeline::rtxvideo

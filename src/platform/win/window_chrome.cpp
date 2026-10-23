#include "enh/platform/window_chrome.hpp"

#include "enh/platform/appearance.hpp"

#include "win_include.hpp"

#include <dwmapi.h>

namespace enh::platform {
namespace {

// Valeurs documentees de dwmapi.h (Windows 11), redeclarees pour ne pas dependre de la
// version du SDK installe.
constexpr DWORD kAttrCloak = 13;              // DWMWA_CLOAK
constexpr DWORD kAttrImmersiveDarkMode = 20;  // DWMWA_USE_IMMERSIVE_DARK_MODE
constexpr DWORD kAttrCaptionColor = 35;       // DWMWA_CAPTION_COLOR
constexpr DWORD kAttrSystemBackdropType = 38; // DWMWA_SYSTEMBACKDROP_TYPE
constexpr int kBackdropNone = 1;              // DWMSBT_NONE
constexpr int kBackdropMainWindow = 2;        // DWMSBT_MAINWINDOW (Mica)
constexpr DWORD kColorDefault = 0xFFFFFFFFU;  // DWMWA_COLOR_DEFAULT

template <typename T> HRESULT setAttribute(NativeWindow window, DWORD attribute, const T& value) {
    return ::DwmSetWindowAttribute(window, attribute, &value, sizeof(T));
}

using RtlGetVersionFn = LONG(WINAPI*)(OSVERSIONINFOW*);

} // namespace

std::uint32_t windowsBuildNumber() noexcept {
    // ntdll est toujours charge : GetModuleHandleW ne charge aucune DLL.
    HMODULE ntdll = ::GetModuleHandleW(L"ntdll.dll");
    if (ntdll == nullptr) {
        return 0;
    }
    // Conversion FARPROC -> pointeur de fonction type : idiome Win32 documente.
    const auto rtlGetVersion =
        reinterpret_cast<RtlGetVersionFn>(::GetProcAddress(ntdll, "RtlGetVersion"));
    if (rtlGetVersion == nullptr) {
        return 0;
    }
    OSVERSIONINFOW info{};
    info.dwOSVersionInfoSize = sizeof(info);
    if (rtlGetVersion(&info) != 0) {
        return 0;
    }
    return info.dwBuildNumber;
}

bool applyWindowChrome(NativeWindow window, const ChromeState& state) noexcept {
    if (window == nullptr) {
        return false;
    }
    // Fond GDI de la classe de fenetre : noir. Dans un cadre DWM etendu, le noir GDI est rendu
    // transparent (le Mica apparait) ; le fond blanc par defaut masquerait le Mica et pourrait
    // flasher au redimensionnement. Pinceau de stock : rien a liberer.
    ::SetClassLongPtrW(window, GCLP_HBRBACKGROUND,
                       reinterpret_cast<LONG_PTR>(::GetStockObject(BLACK_BRUSH)));

    const BOOL dark = state.dark ? TRUE : FALSE;
    (void)setAttribute(window, kAttrImmersiveDarkMode, dark);

    bool micaAccepted = true;
    const int backdrop = state.mica ? kBackdropMainWindow : kBackdropNone;
    const HRESULT hr = setAttribute(window, kAttrSystemBackdropType, backdrop);
    if (state.mica && FAILED(hr)) {
        micaAccepted = false;
    }

    // Mica : le cadre DWM s'etend a toute la zone client (le fond transparent de Qt laisse voir
    // le Mica). Opaque : cadre normal.
    const int extend = (state.mica && micaAccepted) ? -1 : 0;
    const MARGINS margins{.cxLeftWidth = extend,
                          .cxRightWidth = extend,
                          .cyTopHeight = extend,
                          .cyBottomHeight = extend};
    (void)::DwmExtendFrameIntoClientArea(window, &margins);

    DWORD caption = kColorDefault;
    if (!state.highContrast && state.captionRgb.has_value()) {
        caption = colorrefFromRgb(*state.captionRgb);
    }
    (void)setAttribute(window, kAttrCaptionColor, caption);
    return micaAccepted;
}

WindowCloak::WindowCloak(NativeWindow window) noexcept : window_(window) {
    if (window_ != nullptr) {
        const BOOL on = TRUE;
        cloaked_ = SUCCEEDED(setAttribute(window_, kAttrCloak, on));
    }
}

WindowCloak::~WindowCloak() {
    reveal();
}

void WindowCloak::reveal() noexcept {
    if (!cloaked_) {
        return;
    }
    const BOOL off = FALSE;
    (void)setAttribute(window_, kAttrCloak, off);
    cloaked_ = false;
}

bool handleBackdropMessage(NativeWindow window, const void* message, long long* result) noexcept {
    const auto* msg = static_cast<const MSG*>(message);
    if (msg == nullptr || window == nullptr || msg->hwnd != window ||
        msg->message != WM_ERASEBKGND) {
        return false;
    }
    // WM_ERASEBKGND : wParam = HDC (documente) ; conversion entier -> handle imposee par Win32.
    auto* dc = reinterpret_cast<HDC>(msg->wParam); // NOLINT(performance-no-int-to-ptr)
    RECT rc{};
    if (dc != nullptr && ::GetClientRect(window, &rc) != FALSE) {
        ::FillRect(dc, &rc, static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
    }
    if (result != nullptr) {
        *result = 1;
    }
    return true;
}

} // namespace enh::platform

#include "overlay.hpp"
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT WINAPI OverlayWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

bool Overlay::Create(const wchar_t* targetWindowName) {
    targetHwnd = FindWindowW(nullptr, targetWindowName);
    if (!targetHwnd) {
        // CS2: "Counter-Strike 2", fallback to foreground/fullscreen
        targetHwnd = FindWindowW(L"SDL_app", nullptr);
        if (!targetHwnd) targetHwnd = GetForegroundWindow();
        if (!targetHwnd) targetHwnd = GetDesktopWindow();
    }

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"cs2_ext_overlay";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    UpdateBounds();

    hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE,
        wc.lpszClassName, L"overlay",
        WS_POPUP | WS_VISIBLE,
        0, 0, width, height,
        nullptr, nullptr, wc.hInstance, nullptr);

    if (!hwnd) return false;

    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    MARGINS m{-1, -1, -1, -1};
    DwmExtendFrameIntoClientArea(hwnd, &m);

    if (!CreateDevice()) return false;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(device, ctx);
    return true;
}

void Overlay::UpdateBounds() {
    RECT r{};
    if (targetHwnd && targetHwnd != GetDesktopWindow() && GetWindowRect(targetHwnd, &r)) {
        width = r.right - r.left;
        height = r.bottom - r.top;
        if (hwnd) SetWindowPos(hwnd, HWND_TOPMOST, r.left, r.top, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    } else {
        width = GetSystemMetrics(SM_CXSCREEN);
        height = GetSystemMetrics(SM_CYSCREEN);
    }
}

bool Overlay::CreateDevice() {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT flags = 0;
    D3D_FEATURE_LEVEL lvl;
    D3D_FEATURE_LEVEL lvls[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, lvls, 2,
                                             D3D11_SDK_VERSION, &sd, &swap, &device, &lvl, &ctx))) {
        return false;
    }

    ID3D11Texture2D* back = nullptr;
    swap->GetBuffer(0, IID_PPV_ARGS(&back));
    if (back) {
        device->CreateRenderTargetView(back, nullptr, &rtv);
        back->Release();
    }
    return rtv != nullptr;
}

void Overlay::CleanupDevice() {
    if (rtv) { rtv->Release(); rtv = nullptr; }
    if (swap) { swap->Release(); swap = nullptr; }
    if (ctx) { ctx->Release(); ctx = nullptr; }
    if (device) { device->Release(); device = nullptr; }
}

void Overlay::BeginFrame() {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void Overlay::EndFrame() {
    ImGui::Render();
    const float clear[4] = {0, 0, 0, 0};
    ctx->OMSetRenderTargets(1, &rtv, nullptr);
    ctx->ClearRenderTargetView(rtv, clear);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    swap->Present(1, 0);
}

void Overlay::Destroy() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDevice();
    if (hwnd) {
        DestroyWindow(hwnd);
        hwnd = nullptr;
    }
    UnregisterClassW(L"cs2_ext_overlay", GetModuleHandleW(nullptr));
}

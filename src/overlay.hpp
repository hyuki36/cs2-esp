#pragma once
#include <windows.h>
#include <d3d11.h>
#include "vec.hpp"

class Overlay {
public:
    HWND hwnd = nullptr;
    HWND targetHwnd = nullptr;
    int width = 0;
    int height = 0;

    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    IDXGISwapChain* swap = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;

    bool Create(const wchar_t* targetWindowName);
    void BeginFrame();
    void EndFrame();
    void Destroy();
    void UpdateBounds();
    void SetClickable(bool clickable);

private:
    bool CreateDevice();
    void CleanupDevice();
};

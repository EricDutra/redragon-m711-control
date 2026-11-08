#include <windows.h>
#include <d3d11.h>
#include <dwmapi.h>

#include <string>

#include "app_ui.h"
#include "i18n.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace {

ID3D11Device* g_device = nullptr;
ID3D11DeviceContext* g_context = nullptr;
IDXGISwapChain* g_swapChain = nullptr;
ID3D11RenderTargetView* g_renderTarget = nullptr;
UINT g_resizeWidth = 0;
UINT g_resizeHeight = 0;
App* g_app = nullptr;

void createRenderTarget() {
    ID3D11Texture2D* backBuffer = nullptr;
    g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    g_device->CreateRenderTargetView(backBuffer, nullptr, &g_renderTarget);
    backBuffer->Release();
}

void releaseRenderTarget() {
    if (g_renderTarget) {
        g_renderTarget->Release();
        g_renderTarget = nullptr;
    }
}

bool createDevice(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = 2;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferDesc.RefreshRate.Numerator = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = hwnd;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL obtained;
    HRESULT result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2,
                                                   D3D11_SDK_VERSION, &desc, &g_swapChain, &g_device, &obtained,
                                                   &g_context);
    if (result == DXGI_ERROR_UNSUPPORTED) {
        result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, levels, 2, D3D11_SDK_VERSION,
                                               &desc, &g_swapChain, &g_device, &obtained, &g_context);
    }
    if (FAILED(result)) {
        return false;
    }
    createRenderTarget();
    return true;
}

void destroyDevice() {
    releaseRenderTarget();
    if (g_swapChain) {
        g_swapChain->Release();
        g_swapChain = nullptr;
    }
    if (g_context) {
        g_context->Release();
        g_context = nullptr;
    }
    if (g_device) {
        g_device->Release();
        g_device = nullptr;
    }
}

LRESULT WINAPI windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (g_app && g_app->isRecording()) {
        bool extended = (lParam >> 24) & 1;
        bool repeat = (lParam >> 30) & 1;
        switch (msg) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (!repeat) {
                g_app->onKey(static_cast<int>(wParam), true, extended);
            }
            return 0;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            g_app->onKey(static_cast<int>(wParam), false, extended);
            return 0;
        case WM_CHAR:
        case WM_SYSCHAR:
            return 0;
        }
    }

    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) {
        return true;
    }

    switch (msg) {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            g_resizeWidth = LOWORD(lParam);
            g_resizeHeight = HIWORD(lParam);
        }
        return 0;
    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize.x = 860;
        info->ptMinTrackSize.y = 600;
        return 0;
    }
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_KEYMENU) {
            return 0;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void applyStyle() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.FramePadding = ImVec2(8, 5);
    style.ItemSpacing = ImVec2(8, 7);
    style.WindowPadding = ImVec2(14, 12);

    ImVec4* colors = style.Colors;
    ImVec4 accent(0.78f, 0.12f, 0.16f, 1.0f);
    ImVec4 accentHover(0.88f, 0.22f, 0.25f, 1.0f);
    ImVec4 accentActive(0.65f, 0.08f, 0.12f, 1.0f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.09f, 0.10f, 1.0f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.11f, 0.11f, 0.12f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.18f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.24f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.26f, 0.28f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.22f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = accentHover;
    colors[ImGuiCol_ButtonActive] = accentActive;
    colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.24f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = accentHover;
    colors[ImGuiCol_HeaderActive] = accent;
    colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.17f, 1.0f);
    colors[ImGuiCol_TabHovered] = accentHover;
    colors[ImGuiCol_TabSelected] = accent;
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = accentHover;
    colors[ImGuiCol_CheckMark] = accentHover;
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.15f, 0.15f, 0.17f, 1.0f);
}

void loadFont() {
    ImGuiIO& io = ImGui::GetIO();
    char windowsDir[MAX_PATH];
    GetWindowsDirectoryA(windowsDir, MAX_PATH);
    std::string path = std::string(windowsDir) + "\\Fonts\\segoeui.ttf";
    if (GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
        io.Fonts->AddFontFromFileTTF(path.c_str(), 17.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
    } else {
        io.Fonts->AddFontDefault();
    }
}

}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    ImGui_ImplWin32_EnableDpiAwareness();
    i18n::loadLanguage();

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_CLASSDC;
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    windowClass.lpszClassName = L"RedragonControl";
    RegisterClassExW(&windowClass);

    HWND hwnd = CreateWindowW(windowClass.lpszClassName, L"Redragon Control", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                              CW_USEDEFAULT, 980, 720, nullptr, nullptr, instance, nullptr);

    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));

    if (!createDevice(hwnd)) {
        destroyDevice();
        MessageBoxW(nullptr, i18n::language() == i18n::Language::Portuguese ? L"Não foi possível inicializar o Direct3D 11." : L"Could not initialize Direct3D 11.", L"Redragon Control", MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    applyStyle();
    loadFont();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_device, g_context);

    App app;
    g_app = &app;
    app.start();

    const float clearColor[4] = {0.09f, 0.09f, 0.10f, 1.0f};
    bool running = true;
    while (running) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) {
                running = false;
            }
        }
        if (!running) {
            break;
        }

        if (g_resizeWidth != 0 && g_resizeHeight != 0) {
            releaseRenderTarget();
            g_swapChain->ResizeBuffers(0, g_resizeWidth, g_resizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_resizeWidth = g_resizeHeight = 0;
            createRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        app.draw();
        ImGui::Render();

        g_context->OMSetRenderTargets(1, &g_renderTarget, nullptr);
        g_context->ClearRenderTargetView(g_renderTarget, clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_swapChain->Present(1, 0);
    }

    g_app = nullptr;
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    destroyDevice();
    DestroyWindow(hwnd);
    UnregisterClassW(windowClass.lpszClassName, instance);
    return 0;
}

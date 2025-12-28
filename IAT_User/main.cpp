#include <windows.h>
#include <tlhelp32.h>
#include <cwchar>
#include <iostream>
#include <vector>
#include <d3d11.h>
#include <tchar.h>
#include <winternl.h>
#include <ntstatus.h>

#include "helper.h"
#include "struct.h"
#include "draw.h"
#include "aim.h"

#include "..\IAT\Common.h"


#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")


#pragma comment(lib, "d3d11.lib")

#pragma comment(lib,"ntdll.lib")

ID3D11Device* gDevice = nullptr;
ID3D11DeviceContext* gContext = nullptr;
IDXGISwapChain* gSwapChain = nullptr;
ID3D11RenderTargetView* gRTV = nullptr;

typedef INT64(__fastcall* fNtGdiEllipse)(ULONG64);

HMODULE user32 = LoadLibraryA("user32.dll");
HMODULE win32u = LoadLibraryA("win32u.dll");

fNtGdiEllipse NtGdiEllipse = (fNtGdiEllipse)GetProcAddress
(GetModuleHandleA("win32u.dll"), "NtGdiEllipse");;


namespace driver {


    template <class T>
    T read_memory(const std::uintptr_t addr, const HANDLE pid) {
        T temp = {};


        if (!NtGdiEllipse)
            return {};


        INFORMATION info{};

        info.key = SPECIAL_CALL;
        info.operation = 0;
        info.process_id = pid;
        info.target = reinterpret_cast<PVOID>(addr);
		info.size = sizeof(T);
        info.buffer = &temp;

        NtGdiEllipse((ULONG64)&info);

        return temp;
    }


    std::uintptr_t resolve_pointer_chain(
        std::uintptr_t base,
        const std::vector<std::uintptr_t>& offsets,
		const HANDLE pid
    )
    {
        if (offsets.empty())
            return base;

        std::uintptr_t current = read_memory<std::uintptr_t>(
            base,
            pid
        );

        for (size_t i = 0; i < offsets.size() - 1; ++i)
        {
            current = read_memory<std::uintptr_t>(
                current + offsets[i],
                pid
            );

            if (!current)
                return 0;
        }

        return current + offsets.back();
    }

} // namespace driver



FVector2D get_screen_pos(DWORD pid, float window_width, float window_height) {

    const std::uintptr_t module_base = get_module_base(pid, L"FPSChess-Win64-Shipping.exe"); // FPSChess-Win64-Shipping

    if (!module_base) {
        std::cout << "Failed to get module base.\n";
        std::cin.get();
        return { 0, 0 };
    }

    // enemy position addr
    uintptr_t enemy_xyz_addr = driver::resolve_pointer_chain(
        module_base + 0x458BB50,
        enemy_pos_offset,
        (HANDLE)pid
    );

    // local pitch yaw addr
    uintptr_t local_pitch_yaw_addr = driver::resolve_pointer_chain(
        module_base + 0x458BB50,
        local_pitch_yaw_offset,
        (HANDLE)pid
    );

    // local camera position addr
    uintptr_t local_camera_addr = driver::resolve_pointer_chain(
        module_base + 0x458BB50,
        local_camera_pos_offset,
        (HANDLE)pid
    );

    // local fov addr
    uintptr_t local_fov_addr = driver::resolve_pointer_chain(
        module_base + 0x458BB50,
        local_fov_offset,
        (HANDLE)pid
    );

    // start reading
    FVector enemy_pos = driver::read_memory<FVector>(enemy_xyz_addr, (HANDLE)pid);
 
    std::cout << "enemy_pos: " << enemy_pos.X << ", "
        << enemy_pos.Y << ", "
        << enemy_pos.Z << std::endl << std::endl;

	enemy_pos.Z += 50.0f;

    //if(enemy_pos.X == 0 && enemy_pos.Y == 0)
		//return { 0, 0 };


    FRotator local_pitch_yaw = driver::read_memory<FRotator>(local_pitch_yaw_addr, (HANDLE)pid);

    NormalizePitch(local_pitch_yaw.Pitch);

    std::cout << "local_pitch_yaw: " << local_pitch_yaw.Pitch << ", "
        << local_pitch_yaw.Yaw << ", " <<  local_pitch_yaw.Roll << std::endl << std::endl;



    FVector local_camera_pos = driver::read_memory<FVector>(local_camera_addr, (HANDLE)pid);
    std::cout << "local_camera_pos: " << local_camera_pos.X << ", "
        << local_camera_pos.Y << ", "
        << local_camera_pos.Z << std::endl << std::endl;


    float aspect = window_width / window_height;
    float local_fov = driver::read_memory<float>(local_fov_addr, (HANDLE)pid);

    std::cout << "local fov: " << local_fov << std::endl << std::endl;


    FVector2D screen;

    bool w2s = WorldToScreen(enemy_pos, screen, local_camera_pos, local_pitch_yaw, local_fov, window_width/2, window_height/2);

    if (!w2s)
        screen = { 0, 0 };

    std::cout << "screen: " << screen.X << ", " << screen.Y << std::endl << std::endl;

    return screen;
}

int main() {

    const DWORD pid = get_process_id(L"FPSChess-Win64-Shipping.exe");

    if (pid == 0) {
        std::cout << "Failed to find process ID.\n";
        std::cin.get();
        return 1;
    }


    WNDCLASSEX wc = {
        sizeof(WNDCLASSEX),
        CS_CLASSDC,
        WndProc,
        0L, 0L,
        GetModuleHandle(NULL),
        NULL, NULL, NULL, NULL,
        _T("ImGuiWindow"),
        NULL
    };

    RegisterClassEx(&wc);

    HWND hwnd = CreateWindow(
        wc.lpszClassName,
        _T("ImGui DX11"),
        WS_POPUP,
        100, 100, 1280, 800,
        NULL, NULL,
        wc.hInstance,
        NULL
    );

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &sd,
        &gSwapChain,
        &gDevice,
        nullptr,
        &gContext
    );

    if (FAILED(hr))
    {
        MessageBoxA(nullptr, "DX11 init failed", "Error", MB_OK);
        return 0;
    }

    // Create render target
    ID3D11Texture2D* backBuffer = nullptr;
    gSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    gDevice->CreateRenderTargetView(backBuffer, nullptr, &gRTV);
    backBuffer->Release();


    // ---- AFTER DX11 DEVICE IS CREATED ---- IMGUI setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Initialize backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(gDevice, gContext);


    LONG ex = GetWindowLong(hwnd, GWL_EXSTYLE);
    SetWindowLong(hwnd, GWL_EXSTYLE,
        ex | WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW
    );

    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    SetProcessDpiAwarenessContext(
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
    );

    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    HWND target = FindWindowByPID(pid);

    if (!target) {
        std::cout << "cannot find FPS Chess window" << std::endl;
    }

    // GOD CODE
    static int lastW = 0, lastH = 0;

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!target)
            continue;


        RECT client;
        GetClientRect(target, &client);

        POINT topLeft = { client.left, client.top };
        ClientToScreen(target, &topLeft);

        int width = client.right - client.left;
        int height = client.bottom - client.top;

        if (width == 0 || height == 0)
            continue;



        // GOD CODE

        if (width != lastW || height != lastH)
        {
            ImGui_ImplDX11_InvalidateDeviceObjects();

            gContext->OMSetRenderTargets(0, nullptr, nullptr);
            gRTV->Release();

            gSwapChain->ResizeBuffers(
                0,
                width,
                height,
                DXGI_FORMAT_UNKNOWN,
                0
            );

            ID3D11Texture2D* backBuffer = nullptr;
            gSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
            gDevice->CreateRenderTargetView(backBuffer, nullptr, &gRTV);
            backBuffer->Release();

            // THIS WAS MISSING
            D3D11_VIEWPORT vp = {};
            vp.Width = (FLOAT)width;
            vp.Height = (FLOAT)height;
            vp.MinDepth = 0.0f;
            vp.MaxDepth = 1.0f;
            gContext->RSSetViewports(1, &vp);

            ImGui_ImplDX11_CreateDeviceObjects();

            lastW = width;
            lastH = height;
        }
        // GOD CODE



        SetWindowPos(
            hwnd,
            HWND_TOPMOST,
            topLeft.x,
            topLeft.y,
            width,
            height,
            SWP_NOACTIVATE | SWP_SHOWWINDOW
        );


        // ================== IMGUI FRAME ==================
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // ---- DRAW DOT HERE ----
        ImGui::GetForegroundDrawList()->AddText(
            ImGui::GetFont(),          // current font
            ImGui::GetFontSize() * 4.0f, // scale factor (2.0 = 2x bigger)
            ImVec2(60.0f, 60.0f),
            IM_COL32(255, 0, 0, 255),
            "Overlay"
        );


        std::cout << "width center: " << width/2 << ", height center: " << height/2 << std::endl << std::endl;
               
        //uintptr_t mod_base = driver::read_memory<uintptr_t>(get_module_base(pid, L"Notepad.exe"), (HANDLE)pid);

        FVector2D screen = get_screen_pos(pid, width, height);


        if (screen.X != 0 && screen.Y != 0) {

            ImGui::GetForegroundDrawList()->AddRect(
                ImVec2(screen.X - 30, screen.Y - 50),
                ImVec2(screen.X + 30, screen.Y + 50),
                IM_COL32(255, 0, 0, 255),
                0.0f,
                0,
                2.0f 
            );

            if ((GetAsyncKeyState(VK_RBUTTON) & 0x8000)) {
                SendInputAimbot(
                    width,
                    height,
                    screen.X,
                    screen.Y
                );
			}

        }
        // -----------------------

        ImGui::Render();

        // ================== DX11 RENDER ==================
        float clearColor[4] = { 0.f, 0.f, 0.f, 0.f }; // transparent
        gContext->OMSetRenderTargets(1, &gRTV, nullptr);
        gContext->ClearRenderTargetView(gRTV, clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        gSwapChain->Present(1, 0);

        Sleep(10);
    }


    std::cin.get();
    return 0;
}
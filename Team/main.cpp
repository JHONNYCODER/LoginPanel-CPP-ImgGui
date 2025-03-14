#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <windows.h>
#include <dwmapi.h>  // For enabling transparency and DWM effects
#pragma comment(lib, "dwmapi.lib")  // Link the DWM API library
#include "skCrypter.h"
#include "auth.hpp"
#include <dxgi.h> // Include DXGI header
#include <tchar.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // For loading images

// Link DirectX 11 libraries
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// Global variables
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static ID3D11ShaderResourceView* g_backgroundTexture = nullptr; // Background texture
float colorOffset = 0.0f;
float colorSpeed = 0.002f; // Adjust speed as needed
HWND hwnd = nullptr; // Main window handle

// Initialize KeyAuth
KeyAuth::api KeyAuthApp(
    skCrypt("Team").decrypt(),
    skCrypt("pMXlRimKPb").decrypt(),
    skCrypt("1.0").decrypt(),
    skCrypt("https://keyauth.win/api/1.3/").decrypt(),
    skCrypt("").decrypt(),
    false
);

// Helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();


void SetRoundCorners(HWND hwnd, int radius) {
    if (!hwnd) return; // Ensure valid window handle

    RECT rect;
    GetWindowRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    HRGN hRgn = CreateRoundRectRgn(0, 0, width, height, radius, radius);
    if (hRgn) {
        SetWindowRgn(hwnd, hRgn, TRUE);
        DeleteObject(hRgn); // Prevent memory leaks
    }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
ID3D11ShaderResourceView* CreateTextureFromFile(const char* filename);

// Define the required structures globally
typedef struct _ACCENT_POLICY {
    int nAccentState;
    int nFlags;
    int nColor;
    int nAnimationId;
} ACCENT_POLICY;

typedef struct _WINDOWCOMPOSITIONATTRIBDATA {
    int nAttribute;
    PVOID pData;
    SIZE_T ulDataSize;
} WINDOWCOMPOSITIONATTRIBDATA;

// Declare the function pointer type
typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);

// Function to enable blur and transparency
void EnableBlurBehind(HWND hwnd) {
    // Load user32.dll dynamically
    HMODULE hUser = LoadLibraryA("user32.dll");
    if (!hUser) return;

    // Get the address of SetWindowCompositionAttribute
    pSetWindowCompositionAttribute SetWindowCompositionAttribute =
        (pSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");

    if (SetWindowCompositionAttribute) {
        ACCENT_POLICY accent = { 3, 0, 0, 0 };  // Enable blur effect
        WINDOWCOMPOSITIONATTRIBDATA data = { 19, &accent, sizeof(accent) };

        // Apply the blur effect
        SetWindowCompositionAttribute(hwnd, &data);
    }

    // Free the library
    FreeLibrary(hUser);

    // Extend the window frame into the client area
    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, _T("ImGui Example"), nullptr };
    ::RegisterClassEx(&wc);

    // Remove the Title Bar and buttons
    hwnd = CreateWindow(wc.lpszClassName, _T("Login Page"), WS_POPUP, 100, 100, 800, 600, nullptr, nullptr, wc.hInstance, nullptr);

    // Enable transparency
    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);

 
    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);
    SetFocus(hwnd); // Ensure the window has focus

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load background texture
    g_backgroundTexture = CreateTextureFromFile("C:\\Users\\Lord\\Documents\\Team\\Team\\Team\\Team\\src\\background.png");
    if (!g_backgroundTexture) {
        MessageBox(hwnd, _T("Failed to load background image!"), _T("Error"), MB_ICONERROR);
    }

    // Initialize KeyAuth once
    KeyAuthApp.init();
    if (!KeyAuthApp.response.success) {
        MessageBox(hwnd, _T("Failed to connect to authentication server!"), _T("Error"), MB_ICONERROR);
    }

    ImFont* myLargeFont = nullptr;  // Declare font pointer

    myLargeFont = io.Fonts->AddFontFromFileTTF("C:\\Users\\Lord\\Documents\\Team\\Team\\Team\\Team\\src\\Font.ttf", 16.0f);
    if (myLargeFont == nullptr)
    {
        OutputDebugStringA("Failed to load font!\n");
    }

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Update animation offset
            colorOffset += colorSpeed;
        if (colorOffset > 1.0f) colorOffset -= 1.0f; // Loop the animation


        // Start the ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Render background image
        if (g_backgroundTexture) {
            ImGui::GetBackgroundDrawList()->AddImage(
                (ImTextureID)g_backgroundTexture,
                ImVec2(0, 0),
                ImVec2(io.DisplaySize.x, io.DisplaySize.y)
            );
        }
        
        // Draw animated border
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        ImVec2 windowPos = ImVec2(0, 0);
        ImVec2 windowSize = ImGui::GetIO().DisplaySize;
        float thickness = 3.0f; // Border thickness

        // Create a gradient color
        ImColor color1 = ImColor::HSV(colorOffset, 1.0f, 1.0f); // Hue shifts over time
        ImColor color2 = ImColor::HSV(colorOffset + 0.5f, 1.0f, 1.0f); // Complementary color

        // Draw the border
        drawList->AddRect(windowPos, windowSize, color1, 30.0f, 0, thickness); // Rounded corners
        drawList->AddRect(windowPos, windowSize, color2, 30.0f, 0, thickness); // Complementary color

        // Customize ImGui style for a modern look
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 15.0f;
        style.FrameRounding = 10.0f;
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0, 0, 0, 0);
        style.Colors[ImGuiCol_Border] = ImVec4(0, 0, 0, 0);

        // Centered Login Window
        float moveRight = 120.0f; // Adjust this value for more shift
        float loginPos = 130.0f; //Adjust the value for shift of Login button
        ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - 100) * 0.5f, (io.DisplaySize.y - 100) * 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 300));
        ImGui::Begin("Login", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        static char username[32] = "";
        static char password[11] = "";
        static bool showPassword = false;

        // Move inputs slightly to the right

        ImGui::SetCursorPosX(moveRight);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 8.0f));

        ImGui::Text("Username");
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + moveRight);
        ImGui::SetNextItemWidth(150); // Reduce width of username input
        ImGui::InputText("##Username", username, IM_ARRAYSIZE(username));

        ImGui::Spacing();  // Adds a small gap
        ImGui::Spacing();  // Adds a small gap

        ImGui::SetCursorPosX(moveRight);
        ImGui::Text("Password");
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + moveRight);
        ImGui::SetNextItemWidth(150); // Reduce width of password input
        if (showPassword) {
            ImGui::InputText("##Password", password, IM_ARRAYSIZE(password));
        }
        else {
            ImGui::InputText("##Password", password, IM_ARRAYSIZE(password), ImGuiInputTextFlags_Password);
        }
        
        ImGui::PopStyleVar(2); // Restore default styles

        // Keep "Show Password" aligned properly
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + moveRight);
        ImGui::Checkbox("Show Password", &showPassword);

        // Align the login button properly

        // Pop the styles to avoid affecting other elements
        ImVec4 buttonColor = ImVec4(169 / 255.0f, 82 / 255.0f, 235 / 255.0f, 0.8f);   // Default (Purple)
        ImVec4 hoverColor = ImVec4(217 / 255.0f, 246 / 255.0f, 63 / 255.0f, 0.62f);  // Hover (Soft Yellow-Green)


        ImGui::NewLine();  // Moves to a new line
        ImGui::SetCursorPosX(loginPos);
        ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);

        if (ImGui::Button("Sign in -->", ImVec2(120, 30))) {
            KeyAuthApp.login(username, password);

            if (KeyAuthApp.response.success) {
                MessageBox(hwnd, _T("Login successful!"), _T("Success"), MB_ICONINFORMATION);
            }
            else {
                MessageBox(hwnd, _T("Invalid username or password!"), _T("Error"), MB_ICONERROR);
            }
        }

        // Pop the styles to avoid affecting other elements
        ImGui::PopStyleColor(2);

        ImGui::End();

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.45f, 0.55f, 0.60f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

  if (g_backgroundTexture) {
        g_backgroundTexture->Release();
        g_backgroundTexture = nullptr;
    } 

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Load texture from file using stb_image
ID3D11ShaderResourceView* CreateTextureFromFile(const char* filename)
{
    int width, height, channels;
    unsigned char* image_data = stbi_load(filename, &width, &height, &channels, 4); // Force 4 channels (RGBA)
    if (!image_data) {
        return nullptr;
    }

    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData;
    initData.pSysMem = image_data;
    initData.SysMemPitch = width * 4; // 4 bytes per pixel (RGBA)
    initData.SysMemSlicePitch = 0;

    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, &initData, &texture);
    if (FAILED(hr)) {
        stbi_image_free(image_data);
        return nullptr;
    }

    ID3D11ShaderResourceView* textureView = nullptr;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    hr = g_pd3dDevice->CreateShaderResourceView(texture, &srvDesc, &textureView);
    texture->Release();
    stbi_image_free(image_data);

    if (FAILED(hr)) {
        return nullptr;
    }

    return textureView;
}

// Win32 message handler
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    if (ImGui::GetCurrentContext() != nullptr)  // Prevent access violation
    {
        ImGuiIO& io = ImGui::GetIO();

        if (!io.WantCaptureMouse)
        {
            if (msg == WM_LBUTTONDOWN)
                OutputDebugStringA("Left Mouse Click\n");
        }

        if (!io.WantCaptureKeyboard)
        {
            if (msg == WM_KEYDOWN)
                OutputDebugStringA("Key Pressed\n");
        }
    }

    switch (msg)
    {
    case WM_CREATE:
        SetRoundCorners(hWnd, 50);
        break;

    case WM_NCHITTEST:
    {
        LRESULT hit = DefWindowProc(hWnd, msg, wParam, lParam);
        if (hit == HTCLIENT && ImGui::GetCurrentContext() && !ImGui::GetIO().WantCaptureMouse)
            return HTCAPTION;
        return hit;
    }

    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        ImGui::DestroyContext();  // Destroy ImGui AFTER the window is closed
        ::PostQuitMessage(0);
        return 0;
    }

    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
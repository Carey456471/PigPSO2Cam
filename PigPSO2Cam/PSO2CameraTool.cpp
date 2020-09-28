#pragma once
#include "PSO2CameraTool.hpp"

#include <detours.h>
#include <d3d9.h>
#include <D3dx9core.h>
#include "Asm.h"
#include "imgui/settings_form.h"




bool m_bCreated = false;
bool wndproc_found = false;
D3DVIEWPORT9 viewport;
LPD3DXFONT dxFont;
HMODULE hmRendDx9Base = NULL;

HWND game_hwnd = 0;
HMODULE psoBase = 0;

typedef HRESULT(__stdcall* tEndScene)(LPDIRECT3DDEVICE9 Device);
tEndScene oEndScene;

typedef HRESULT(__stdcall* tReset)(LPDIRECT3DDEVICE9 Device, D3DPRESENT_PARAMETERS* pPresentationParameters);
tReset oReset;

WNDPROC game_wndproc = NULL;
extern IMGUI_IMPL_API LRESULT  ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static bool MENU_DISPLAYING = false;


static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

uintptr_t cameraFarCullJna;
uintptr_t cameraFarCullObjectJe;
uintptr_t cameraNearCullAddy;


LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam) || (MENU_DISPLAYING))
	{
		if (MENU_DISPLAYING && msg == WM_KEYDOWN && wParam==VK_ESCAPE) {
			MENU_DISPLAYING = false;
		}
		return true;
	}

	return CallWindowProc(game_wndproc, hWnd, msg, wParam, lParam);
}

uintptr_t getTerrainFarCullAddy()
{
	return cameraFarCullJna;
}
uintptr_t getObjectFarCullAddy()
{
	return cameraFarCullObjectJe;
}
uintptr_t getCameraNearCullAddy()
{
	return cameraNearCullAddy;
}
HRESULT __stdcall hkEndScene(LPDIRECT3DDEVICE9 Device)
{
	using namespace Asm;

	if (Device == nullptr)
		return oEndScene(Device);
	if (!m_bCreated)
	{
		m_bCreated = true;
		D3DXCreateFontA(Device, 20, 0, FW_BOLD, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas", &dxFont);
		Device->GetViewport(&viewport);

		D3DDEVICE_CREATION_PARAMETERS d3dcp;
		Device->GetCreationParameters(&d3dcp);

		game_hwnd = d3dcp.hFocusWindow;
		

		DWORD farCullScan = AobScan(cameraFarCullAob);
		if (farCullScan)
			cameraFarCullJna = farCullScan;
		DWORD objectCullScan = AobScan(cameraFarCullObjectsAob);
		if (objectCullScan) {
			cameraFarCullObjectJe = objectCullScan;
			cameraFarCullObjectJe += 0x7;
		}
		DWORD nearCullScan = AobScan(cameraNearCullAob);
		if (nearCullScan)
			cameraNearCullAddy = nearCullScan;
		
	}
	if (!wndproc_found) {
		HWND wnd = FindWindowA("Phantasy Star Online 2", NULL);
		if (wnd) {
			game_wndproc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(wnd, GWLP_WNDPROC, (LONG_PTR)WndProc));
			menu_init(game_hwnd, Device);

			wndproc_found = true;
		}
	}

	if ((GetAsyncKeyState(VK_INSERT) & 1)) { 
		MENU_DISPLAYING = !MENU_DISPLAYING;
	}
	if (MENU_DISPLAYING )
	{
		draw_menu(&MENU_DISPLAYING);
	}


	return oEndScene(Device);
}
HRESULT APIENTRY hkReset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	if (dxFont)
		dxFont->OnLostDevice();

	HRESULT result = oReset(pDevice, pPresentationParameters);
	if (SUCCEEDED(result))
	{
		if (dxFont)
			dxFont->OnResetDevice();
		m_bCreated = false;
		ImGui_ImplDX9_InvalidateDeviceObjects();
		pDevice->GetViewport(&viewport);

		ImGui_ImplDX9_CreateDeviceObjects();
	}
	return result;
}


DWORD_PTR* pVTable;
HWND tmpWnd;
bool CreateDeviceD3D(HWND hWnd)
{
	tmpWnd = CreateWindowA("BUTTON", "DX", WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, NULL, NULL, GetModuleHandle(NULL), NULL);

	if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
		return false;

	// Create the D3DDevice
	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	g_d3dpp.hDeviceWindow = tmpWnd;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
	if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
		return false;

	pVTable = (DWORD_PTR*)g_pd3dDevice;
	pVTable = (DWORD_PTR*)pVTable[0];
	return true;
}

DWORD WINAPI HookThread()
{

	CreateDeviceD3D(game_hwnd);
	if (!pVTable)
		return false;
	
	oEndScene = (tEndScene)pVTable[42];
	oReset = (tReset)pVTable[16];
	
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(LPVOID&)oEndScene, (PBYTE)hkEndScene);
	DetourAttach(&(LPVOID&)oReset, (PBYTE)hkReset);
	DetourTransactionCommit();

	g_pD3D->Release();
	g_pd3dDevice->Release();
	DestroyWindow(tmpWnd);
	return 0;
}

int Initialize() {
	/*while (hmRendDx9Base == NULL)
	{
		Sleep(200);
		hmRendDx9Base = GetModuleHandleA("d3d9.dll");
	}
	while (game_hwnd == NULL) {
		Sleep(200);
		game_hwnd = FindWindowA("Phantasy Star Online 2", NULL);
	}
	Sleep(1000); //idk it just be like this
	*/
	HookThread();
	return 1;
}
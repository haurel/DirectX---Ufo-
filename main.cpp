#include <d3d9.h>
#include <d3dx9.h>
#include <Windows.h>
#include <dshow.h>
#include <d3dx9tex.h>
#include <dinput.h>
#include "Mesh.h"
#include "Camera.h"

#pragma comment (lib, "dinput8.lib")
#pragma comment (lib, "dxguid.lib")

LPDIRECT3D9					g_pD3D = NULL; 
LPDIRECT3DDEVICE9			g_pd3dDevice = NULL; 
LPDIRECT3DTEXTURE9			*g_pTextureArray = NULL;
LPDIRECT3DVERTEXBUFFER9		g_pVertexBuffer = NULL;
IGraphBuilder				*pGraph = NULL;
IMediaControl				*pControl = NULL;
IMediaEventEx				*pEvent = NULL;
BYTE						g_Keystate[256];
LPDIRECTINPUTDEVICE8		g_pDinmouse;
DIMOUSESTATE				g_pMousestate;
LPDIRECTINPUT8				g_pDin;
LPDIRECTINPUTDEVICE8		g_pDinKeyboard;
HRESULT						result;
CXMesh						*cxmesh;
CXCamera					*cxcamera;
FLOAT						dx, dy, dz = 0;
FLOAT						movementSpeed = 0.015f;
HWND						hWnd;
HDC							hdc;
INT							pause = 1;
D3DXVECTOR2					cameraRotation;
D3DXVECTOR2					Mouse;


const char* cubeSides[6] = {
	"\\Assets\\Skybox\\left.png", "Assets\\Skybox\\right.png", "Assets\\Skybox\\bottom.png",
	"Assets\\Skybox\\top.png", "Assets\\Skybox\\front.png", "Assets\\Skybox\\back.png" };
const char* meshFile = "Assets\\Mesh\\ufo.x";
LPCWSTR audioFile = L"Assets\\Sound\\space.wav";
LPCWSTR audioFile2 = L"Assets\\Sound\\space2.wav";
struct CUSTOMVERTEX
{
	D3DXVECTOR3 position;
	D3DXVECTOR3 normal;
	DWORD    color;
	FLOAT       tu, tv;
};

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_NORMAL | D3DFVF_TEX1)
#define WM_GRAPHNOTIFY  WM_APP + 1

HRESULT InitD3D(HWND hWnd);
HRESULT InitDInput(HINSTANCE hInstance, HWND hWnd);
HRESULT InitGeometry();

VOID Cleanup();
VOID CleanDInput();
VOID SetupMatrices();
VOID Render();
VOID DetectInput();

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT);

HRESULT InitD3D(HWND hWnd)
{
	if (NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
		return E_FAIL;

	// Set up the structure used to create the D3DDevice. Since we are now
	// using more complex geometry, we will create a device with a zbuffer.
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

	// Create the D3DDevice
	if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp, &g_pd3dDevice)))
	{
		return E_FAIL;
	}
	g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);

	g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	return S_OK;
}
HRESULT InitDInput(HINSTANCE hInstance, HWND hWnd)
{
	DirectInput8Create(hInstance,
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		(void**)& g_pDin,
		NULL);

	g_pDin->CreateDevice(GUID_SysKeyboard,
		&g_pDinKeyboard,
		NULL);
	g_pDin->CreateDevice(GUID_SysMouse,
		&g_pDinmouse,
		NULL);

	g_pDinKeyboard->SetDataFormat(&c_dfDIKeyboard);
	g_pDinmouse->SetDataFormat(&c_dfDIMouse);

	g_pDinKeyboard->SetCooperativeLevel(hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	g_pDinmouse->SetCooperativeLevel(hWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);

	return S_OK;
}
HRESULT InitDirectShow(HWND hWnd)
{
	//Create Filter Graph
	HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL,
		CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&pGraph);

	//Create Media Control and Events
	hr = pGraph->QueryInterface(IID_IMediaControl, (void **)&pControl);
	hr = pGraph->QueryInterface(IID_IMediaEventEx, (void **)&pEvent);

	//Load a file
	hr = pGraph->RenderFile(audioFile2, NULL);

	
	//Set window for events
	pEvent->SetNotifyWindow((OAHWND)hWnd, WM_GRAPHNOTIFY, 0);

	//Play media control
	//pControl->Run();
	

	return S_OK;
}

void HandleGraphEvent()
{
	// Disregard if we don't have an IMediaEventEx pointer.
	if (pEvent == NULL)
	{
		return;
	}
	// Get all the events
	long evCode;
	LONG_PTR param1, param2;

	while (SUCCEEDED(pEvent->GetEvent(&evCode, &param1, &param2, 0)))
	{
		pEvent->FreeEventParams(evCode, param1, param2);
		switch (evCode)
		{
		case EC_COMPLETE:  // Fall through.
		case EC_USERABORT: // Fall through.
		case EC_ERRORABORT:
			PostQuitMessage(0);
			return;
		}
	}
}

VOID Cleanup()
{
	if (pGraph)
		pGraph->Release();

	if (pControl)
		pControl->Release();

	if (pEvent)
		pEvent->Release();
	if (g_pd3dDevice != NULL)
		g_pd3dDevice->Release();

	if (g_pD3D != NULL)
		g_pD3D->Release();
}
VOID CleanDInput()
{
	g_pDinKeyboard->Unacquire();    // make sure the keyboard is unacquired
	g_pDinmouse->Unacquire();    // make sure the mouse in unacquired
	g_pDin->Release();    // close DirectInput before exiting
}

VOID DetectInput()
{
	g_pDinKeyboard->Acquire();
	g_pDinmouse->Acquire();

	g_pDinKeyboard->GetDeviceState(256, (LPVOID)g_Keystate);
	g_pDinmouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)& g_pMousestate);
}

HRESULT InitGeometry()
{
	cxmesh = new CXMesh(g_pd3dDevice, meshFile);
	cxcamera = new CXCamera(g_pd3dDevice);

	g_pTextureArray = new LPDIRECT3DTEXTURE9[6];

	for (int i = 0; i < 6; i++) {
		if (FAILED(D3DXCreateTextureFromFile(g_pd3dDevice, cubeSides[i], &g_pTextureArray[i])))
		{
			MessageBox(NULL, "Could not find textures", "UFO - Project.exe", MB_OK);
			return E_FAIL;
		}
	}
	CUSTOMVERTEX g_pVertices[] =
	{
		// LEFT
		{D3DXVECTOR3(0,8,0),D3DXVECTOR3(0,8,0),D3DCOLOR_XRGB(255,255,255), 1, 0},		// A
		{D3DXVECTOR3(0,8,-8),D3DXVECTOR3(0,8,-8),D3DCOLOR_XRGB(255,255,255), 0, 0},		// G
		{D3DXVECTOR3(0,0,0),D3DXVECTOR3(0,0,0),D3DCOLOR_XRGB(255,255,255), 1, 1},		// C
		{D3DXVECTOR3(0,0,-8),D3DXVECTOR3(0,0,-8),D3DCOLOR_XRGB(255,255,255), 0, 1},		// H

		// RIGHT
		{D3DXVECTOR3(8,8,0),D3DXVECTOR3(8,8,0),D3DCOLOR_XRGB(255,255,255), 0, 0},		// B
		{D3DXVECTOR3(8,8,-8),D3DXVECTOR3(8,8,-8),D3DCOLOR_XRGB(255,255,255), 1, 0},		// E
		{D3DXVECTOR3(8,0,0),D3DXVECTOR3(8,0,0),D3DCOLOR_XRGB(255,255,255), 0, 1},		// D
		{D3DXVECTOR3(8,0,-8),D3DXVECTOR3(8,0,-8),D3DCOLOR_XRGB(255,255,255), 1, 1},		// F

		//BOTTOM
		{D3DXVECTOR3(0,0,0),D3DXVECTOR3(0,0,0),D3DCOLOR_XRGB(255,255,255), 0, 0},		// C
		{D3DXVECTOR3(8,0,0),D3DXVECTOR3(8,0,0),D3DCOLOR_XRGB(255,255,255), 1, 0},		// D
		{D3DXVECTOR3(0,0,-8),D3DXVECTOR3(0,0,-8),D3DCOLOR_XRGB(255,255,255), 0, 1},		// H
		{D3DXVECTOR3(8,0,-8),D3DXVECTOR3(8,0,-8),D3DCOLOR_XRGB(255,255,255), 1, 1},		// F

		//TOP
		{D3DXVECTOR3(0,8,0),D3DXVECTOR3(0,8,0),D3DCOLOR_XRGB(255,255,255), 0, 1},		//	A
		{D3DXVECTOR3(8,8,0),D3DXVECTOR3(8,8,0),D3DCOLOR_XRGB(255,255,255), 1, 1},		//  B
		{D3DXVECTOR3(0,8,-8),D3DXVECTOR3(0,8,-8),D3DCOLOR_XRGB(255,255,255), 0, 0},		//	G
		{D3DXVECTOR3(8,8,-8),D3DXVECTOR3(8,8,-8),D3DCOLOR_XRGB(255,255,255), 1, 0},		//	E

		//FRONT
		{D3DXVECTOR3(0,8,0),D3DXVECTOR3(0,8,0),D3DCOLOR_XRGB(255,255,255), 0, 0},		// A
		{D3DXVECTOR3(8,8,0),D3DXVECTOR3(8,8,0),D3DCOLOR_XRGB(255,255,255), 1, 0},		// B
		{D3DXVECTOR3(0,0,0),D3DXVECTOR3(0,0,0),D3DCOLOR_XRGB(255,255,255), 0, 1},		// C
		{D3DXVECTOR3(8,0,0),D3DXVECTOR3(8,0,0),D3DCOLOR_XRGB(255,255,255), 1, 1},		// D

		//BACK
		{D3DXVECTOR3(8,8,-8),D3DXVECTOR3(8,8,-8),D3DCOLOR_XRGB(255,255,255), 0, 0},		// E
		{D3DXVECTOR3(0,8,-8),D3DXVECTOR3(0,8,-8),D3DCOLOR_XRGB(255,255,255), 1, 0},		// G
		{D3DXVECTOR3(8,0,-8),D3DXVECTOR3(8,0,-8),D3DCOLOR_XRGB(255,255,255), 0, 1},		// F
		{D3DXVECTOR3(0,0,-8),D3DXVECTOR3(0,0,-8),D3DCOLOR_XRGB(255,255,255), 1, 1},		// H


	};
	if (FAILED(g_pd3dDevice->CreateVertexBuffer(6 * 4 * sizeof(CUSTOMVERTEX),
		0, D3DFVF_CUSTOMVERTEX,
		D3DPOOL_DEFAULT, &g_pVertexBuffer, NULL)))
	{
		return E_FAIL;
	}

	VOID* pVertices;
	if (FAILED(g_pVertexBuffer->Lock(0, sizeof(g_pVertices), (void**)& pVertices, 0)))
		return E_FAIL;
	memcpy(pVertices, g_pVertices, sizeof(g_pVertices));
	g_pVertexBuffer->Unlock();

	return S_OK;
}
VOID SetupMatrices()
{
	cxmesh->SetMeshDefaultPos(g_pd3dDevice);

	D3DXVECTOR3 vEyePt(0.0f, 1.0f, -25.0f);
	D3DXVECTOR3 vLookatPt(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
	cxcamera->LookAtPos(&vEyePt, &vLookatPt, &vUpVec);
	cxcamera->Update();

	D3DXMATRIXA16 matProj;
	D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 1.0f, 1.0f, 100.0f);
	g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);
}

VOID Render()
{
	if (NULL == g_pd3dDevice)
		return;

	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0);

	if (SUCCEEDED(g_pd3dDevice->BeginScene()))
	{
		if (pause == 1) pControl->Run();
		else pControl->Pause();

		SetupMatrices();

		cxcamera->RotateDown(cameraRotation.y);
		cxcamera->RotateRight(cameraRotation.x);
		cxcamera->Update();
		//cxcamera->MoveForward(2 * 2);
		//cxcamera->Update();

		D3DXMATRIX worldSkyBox, scalingSkyBox, translationSkyBox;
		D3DXMatrixScaling(&scalingSkyBox, (5 * 1.5), (5 * 1.5), (5 * 1.5));
		D3DXMatrixTranslation(&translationSkyBox, -15, -15, 20);
		worldSkyBox = scalingSkyBox * translationSkyBox;
		g_pd3dDevice->SetTransform(D3DTS_WORLD, &worldSkyBox);

		for (int i = 0; i < 6; i++) {
			g_pd3dDevice->SetTexture(0, g_pTextureArray[i]);
			g_pd3dDevice->SetStreamSource(0, g_pVertexBuffer, 0, sizeof(CUSTOMVERTEX));
			g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
			g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, i * 4, 2);
		}


		cxmesh->SetMeshPos(g_pd3dDevice, dx, dy, dz);
		cxmesh->DrawMesh(g_pd3dDevice);

		

		g_pd3dDevice->EndScene();
	}
	g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		Cleanup();
		CleanDInput();
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT)
{
	// Register the window class
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
					  GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
					  "D3D Tutorial", NULL };
	RegisterClassEx(&wc);

	// Create the application's window
	HWND hWnd = CreateWindow("D3D Tutorial", "D3D Tutorial 01: CreateDevice",
		WS_OVERLAPPEDWINDOW, 10, 10, 800, 600,
		GetDesktopWindow(), NULL, wc.hInstance, NULL);

	HRESULT hr = CoInitialize(NULL);

	hdc = GetDC(hWnd);

	// Initialize Direct3D
	if (SUCCEEDED(InitD3D(hWnd)))
	{
		if (FAILED(InitDirectShow(hWnd)))
			return 0;
		InitDInput(hInst, hWnd);

		if (SUCCEEDED(InitGeometry())) {
			// Show the window
			ShowWindow(hWnd, SW_SHOWDEFAULT);
			UpdateWindow(hWnd);

			MSG mssg;

			PeekMessage(&mssg, NULL, 0, 0, PM_NOREMOVE);
			// run till completed
			while (mssg.message != WM_QUIT)
			{
				// is there a message to process?
				if (PeekMessage(&mssg, NULL, 0, 0, PM_REMOVE))
				{
					// dispatch the message
					TranslateMessage(&mssg);
					DispatchMessage(&mssg);
				}
				else
				{
					DetectInput();
					Render();
					if (g_Keystate[DIK_P] & 0x80) {
						if (pause == 1) pause = 0;
						else pause = 1;
					}
					
					if (g_Keystate[DIK_UP] & 0x80) {
						dz += movementSpeed;
					}
					if (g_Keystate[DIK_DOWN] & 0x80) {
						dz -= movementSpeed;
					}
					if (g_Keystate[DIK_LEFT] & 0x80) {
						dx -= movementSpeed;
					}
					if (g_Keystate[DIK_RIGHT] & 0x80) {
						dx += movementSpeed;
					}
					if (g_Keystate[DIK_W] & 0x80) {
						dy += movementSpeed;
					}
					if (g_Keystate[DIK_S] & 0x80) {
						dy -= movementSpeed;
					}
					if (g_pMousestate.rgbButtons[0] & 0x80) {
						if (Mouse.x > 800) Mouse.x = (float)800;

						if (Mouse.y < 0) Mouse.x = 0;



						if (Mouse.x > 600) Mouse.y = (float)600;

						if (Mouse.y < 0) Mouse.y = 0;



						cameraRotation.x += g_pMousestate.lX * 0.0007;
						cameraRotation.y += g_pMousestate.lY * 0.0007;
					}

					if (g_Keystate[DIK_ESCAPE] & 0x80) {
						PostMessage(hWnd, WM_DESTROY, 0, 0);
					}
				}
			}
		}
	}

	if (FAILED(result)) return NULL;
	CoUninitialize();

	UnregisterClass("D3D Tutorial", wc.hInstance);
	return 0;
}




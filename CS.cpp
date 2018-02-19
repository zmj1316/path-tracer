//--------------------------------------------------------------------------------------
// File: EmptyProject11.cpp
//
// Empty starting point for new Direct3D 9 and/or Direct3D 11 applications
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "shader_manager.hpp"
#include "buffer_manager.hpp"
#include "scene.hpp"
#include "DXUTcamera.h"


#define  NUM_ELEMENTS   2048

struct Bound
{
	float min[3];
	float padding0;
	float max[3];
	float padding1;
};

struct TreeNode
{
	int left; // primitive index if is leaf
	int right; // == 0 if is leaf
	int parent;
	int padding;
	Bound bound;
};

struct CB_Radix
{
	int node_count;
	int pad0, pad1, pad2;
};

ShaderManager shader_manager_;
BufferManager compute_buffer_manager_;
int localBuffer[NUM_ELEMENTS];

#pragma pack(16)
struct CB_VS_PER_FRAME
{
	D3DXMATRIX m_WorldViewProj;
	D3DXMATRIX m_World;
	vec4 light_dir;
} render_cb_per_frame;

#pragma pack(16)
struct CB_VS_PER_DP
{
	D3DXVECTOR4 params;
} render_cb_per_dp;


BufferManager render_buffer_manager_;
Scene scene;
CModelViewerCamera g_Camera; // A model viewing camera


static void initScene()
{
	scene.loadObj("scene01.obj");
}

//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo* AdapterInfo, UINT Output,
                                      const CD3D11EnumDeviceInfo* DeviceInfo,
                                      DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext)
{
	return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D11 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext)
{
	return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext)
{
	ImGui_ImplDX11_Init(DXUTGetHWNDDeviceWindowed(), pd3dDevice, DXUTGetD3D11DeviceContext());

	HRESULT hr;
	{
		shader_manager_.CreateCS();
		compute_buffer_manager_.release();
		auto tmp = malloc(sizeof(TreeNode) * NUM_ELEMENTS);
		memset(tmp, 0, sizeof(TreeNode) * NUM_ELEMENTS);
		compute_buffer_manager_.addUAV(sizeof(TreeNode), NUM_ELEMENTS, tmp);
		free(tmp);
		CB_Radix rcb;
		rcb.node_count = 500;
		compute_buffer_manager_.addCB(sizeof(CB_Radix), 1, &rcb);
	}

	{
		shader_manager_.CreatePipelineShaders();
		CB_VS_PER_FRAME tmp;
		render_buffer_manager_.addCB(sizeof(CB_VS_PER_FRAME), 1, &tmp);
		CB_VS_PER_DP tmp2;
		render_buffer_manager_.addCB(sizeof(CB_VS_PER_DP), 1, &tmp2);
		scene.buildBuffers(pd3dDevice);

		// Setup the camera's view parameters
		D3DXVECTOR3 vecEye(0.0f, 5.0f, 5.0f);
		D3DXVECTOR3 vecAt(0.0f, 5.0f, 0.0f);
		FLOAT fObjectRadius = 10.0f;

		g_Camera.SetViewParams(&vecEye, &vecAt);
		g_Camera.SetRadius(10.0f, 1.5f, fObjectRadius * 30.0f);
	}
	return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
	ImGui_ImplDX11_InvalidateDeviceObjects();
	ImGui_ImplDX11_CreateDeviceObjects();

	// Setup the camera's projection parameters
	float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
	g_Camera.SetProjParams(D3DX_PI / 6, fAspectRatio, 0.005f, 100.0f);
	g_Camera.SetWindow(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);
	return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	g_Camera.FrameMove(fElapsedTime);
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
                                 double fTime, float fElapsedTime, void* pUserContext)
{
	HRESULT hr;


	{
		pd3dImmediateContext->CSSetShader(shader_manager_.compute_shader_, nullptr, 0);
		pd3dImmediateContext->CSSetShaderResources(0, compute_buffer_manager_.shader_resource_views.size(),
		                                           compute_buffer_manager_.shader_resource_views.data());
		pd3dImmediateContext->CSSetUnorderedAccessViews(0, compute_buffer_manager_.unordered_access_views.size(),
		                                                compute_buffer_manager_.unordered_access_views.data(), nullptr);
		pd3dImmediateContext->CSSetConstantBuffers(0, 1, &compute_buffer_manager_.constant_buffers_[0]);
		pd3dImmediateContext->Dispatch((500 - 1) / 64 + 1, 1, 1);

		//清空Shader和各个Shader Resource View、Unordered Access View以及一些Constant Buffer  
		pd3dImmediateContext->CSSetShader(nullptr, nullptr, 0);

		ID3D11UnorderedAccessView* ppUAViewNULL[] = {nullptr, nullptr};
		pd3dImmediateContext->CSSetUnorderedAccessViews(0, 2, ppUAViewNULL, nullptr);

		ID3D11ShaderResourceView* ppSRVNULL[2] = {nullptr,nullptr};
		pd3dImmediateContext->CSSetShaderResources(0, 2, ppSRVNULL);

		ID3D11Buffer* ppCBNULL[1] = {nullptr};
		pd3dImmediateContext->CSSetConstantBuffers(0, 1, ppCBNULL);
	}


	// Clear render target and the depth stencil 
	float ClearColor[4] = {0.176f, 0.196f, 0.667f, 0.0f};

	ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
	ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
	pd3dImmediateContext->ClearRenderTargetView(pRTV, ClearColor);
	pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0, 0);


	{
		D3DXMATRIX mWorldViewProjection;
		D3DXVECTOR3 vLightDir;
		D3DXMATRIX mWorld;
		D3DXMATRIX mView;
		D3DXMATRIX mProj;
		mWorld = *g_Camera.GetWorldMatrix();
		mProj = *g_Camera.GetProjMatrix();
		mView = *g_Camera.GetViewMatrix();

		mWorldViewProjection = mWorld * mView * mProj;


		D3D11_MAPPED_SUBRESOURCE MappedResource;
		V(pd3dImmediateContext->Map(render_buffer_manager_.constant_buffers_[0], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
		CB_VS_PER_FRAME* pVSPerObject = (CB_VS_PER_FRAME*)MappedResource.pData;
		D3DXMatrixTranspose(&pVSPerObject->m_WorldViewProj, &mWorldViewProjection);
		D3DXMatrixTranspose(&pVSPerObject->m_World, &mWorld);
		pVSPerObject->light_dir = vec4{1, 0, 0,0};
		pd3dImmediateContext->Unmap(render_buffer_manager_.constant_buffers_[0], 0);

		pd3dImmediateContext->VSSetShader(shader_manager_.vertex_shader_, nullptr, 0);
		pd3dImmediateContext->PSSetShader(shader_manager_.pixel_shader_, nullptr, 0);
		pd3dImmediateContext->IASetInputLayout(shader_manager_.vertex_layout_);
		pd3dImmediateContext->VSSetConstantBuffers(0, 1, &render_buffer_manager_.constant_buffers_[0]);
		pd3dImmediateContext->PSSetConstantBuffers(0, 1, &render_buffer_manager_.constant_buffers_[0]);
		pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		for (auto&& geo : scene.geos)
		{
			uint32_t stride = sizeof(Scene::xyzn);
			uint32_t offset = 0;
			pd3dImmediateContext->IASetVertexBuffers(0, 1, &geo.vertex_buffer_, &stride, &offset);
			for (int i = 0; i < geo.indicess.size(); ++i)
			{
				V(pd3dImmediateContext->Map(render_buffer_manager_.constant_buffers_[1], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
				CB_VS_PER_DP* pVSPerObject = (CB_VS_PER_DP*)MappedResource.pData;
				auto matid = geo.material_ids[i];
				pVSPerObject->params.x = (matid & 1) ? 1 : 0;
				pVSPerObject->params.y = (matid & 2) ? 1 : 0;
				pVSPerObject->params.z = (matid & 4) ? 1 : 0;
				pd3dImmediateContext->Unmap(render_buffer_manager_.constant_buffers_[1], 0);
				pd3dImmediateContext->PSSetConstantBuffers(1, 1, &render_buffer_manager_.constant_buffers_[1]);


				pd3dImmediateContext->IASetIndexBuffer(geo.index_buffers_[i], DXGI_FORMAT_R32_UINT, 0);
				pd3dImmediateContext->DrawIndexed(geo.indicess[i].size() , 0, 0);
			}
		}
	}

	ImGui_ImplDX11_NewFrame();
	{
		static float f = 0.0f;
		ImGui::Text("Hello, world!");
		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
		            ImGui::GetIO().Framerate);
	}
	ImGui::Render();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain(void* pUserContext)
{
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{
	ImGui_ImplDX11_Shutdown();
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
extern LRESULT ImGui_ImplDX11_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                         bool* pbNoFurtherProcessing, void* pUserContext)
{
	ImGui_ImplDX11_WndProcHandler(hWnd, uMsg, wParam, lParam);
	if (uMsg == WM_LBUTTONDOWN && ImGui::GetIO().WantCaptureMouse)
		return 0;
	if (uMsg == WM_KEYDOWN && ImGui::GetIO().WantCaptureKeyboard)
		return 0;

	// Pass all remaining windows messages to camera so it can respond to user input
	g_Camera.HandleMessages(hWnd, uMsg, wParam, lParam);

	return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext)
{
}


//--------------------------------------------------------------------------------------
// Handle mouse button presses
//--------------------------------------------------------------------------------------
void CALLBACK OnMouse(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown,
                      bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta,
                      int xPos, int yPos, void* pUserContext)
{
}


//--------------------------------------------------------------------------------------
// Call if device was removed.  Return true to find a new device, false to quit
//--------------------------------------------------------------------------------------
bool CALLBACK OnDeviceRemoved(void* pUserContext)
{
	return true;
}


//--------------------------------------------------------------------------------------
// Initialize everything and go into a render loop
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	initScene();
	// DXUT will create and use the best device (either D3D9 or D3D11) 
	// that is available on the system depending on which D3D callbacks are set below

	// Set general DXUT callbacks
	DXUTSetCallbackFrameMove(OnFrameMove);
	DXUTSetCallbackKeyboard(OnKeyboard);
	DXUTSetCallbackMouse(OnMouse);
	DXUTSetCallbackMsgProc(MsgProc);
	DXUTSetCallbackDeviceChanging(ModifyDeviceSettings);
	DXUTSetCallbackDeviceRemoved(OnDeviceRemoved);

	// Set the D3D11 DXUT callbacks. Remove these sets if the app doesn't need to support D3D11
	DXUTSetCallbackD3D11DeviceAcceptable(IsD3D11DeviceAcceptable);
	DXUTSetCallbackD3D11DeviceCreated(OnD3D11CreateDevice);
	DXUTSetCallbackD3D11SwapChainResized(OnD3D11ResizedSwapChain);
	DXUTSetCallbackD3D11FrameRender(OnD3D11FrameRender);
	DXUTSetCallbackD3D11SwapChainReleasing(OnD3D11ReleasingSwapChain);
	DXUTSetCallbackD3D11DeviceDestroyed(OnD3D11DestroyDevice);

	// Perform any application-level initialization here

	DXUTInit(true, true, nullptr); // Parse the command line, show msgboxes on error, no extra command line params
	DXUTSetCursorSettings(true, true); // Show the cursor and clip it when in full screen
	DXUTCreateWindow(L"CS11");

	// Only require 10-level hardware
	DXUTCreateDevice(D3D_FEATURE_LEVEL_11_0, true, 640, 480);
	DXUTMainLoop(); // Enter into the DXUT render loop

	// Perform any application-level cleanup here
	shader_manager_.release();
	compute_buffer_manager_.release();
	render_buffer_manager_.release();
	scene.release();
	return DXUTGetExitCode();
}

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
#include "structs.hpp"
#include "ray_tracer.hpp"


#define  NUM_ELEMENTS   2048

ShaderManager shader_manager_;
BufferManager compute_buffer_manager_;
int localBuffer[NUM_ELEMENTS];
static bool ray = false;
vec3 g_pos{0,5,17};

ID3D11RasterizerState* g_noculling_state;

#pragma pack(16)
struct CB_VS_PER_FRAME
{
	D3DXMATRIX m_WorldViewProj;
	D3DXMATRIX m_World;

} render_cb_per_frame;

#pragma pack(16)
struct CB_VS_PER_DP
{
	vec3 color;
	int matid;
} render_cb_per_dp;


BufferManager render_buffer_manager_;
Scene scene;
CModelViewerCamera g_Camera; // A model viewing camera
D3DXVECTOR3 vecEye(0.0f, 5, 23);
D3DXVECTOR3 vecAt(0, 5, 0);
FLOAT fObjectRadius = 10.0f;

RayTracer ray_tracer;

static bool save_file = false;
static bool kk_mode = false;

static void initScene()
{
	scene.loadObj("scene03.obj");
	ray_tracer.loadScene();
	ray_tracer.loadShaders();
	ray_tracer.createBuffers();
	if (kk_mode)
		ray_tracer.resize(1024, 1024);
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
	initScene();
	ImGui_ImplDX11_Init(DXUTGetHWNDDeviceWindowed(), pd3dDevice, DXUTGetD3D11DeviceContext());

	HRESULT hr;
	{
		shader_manager_.CreatePipelineShaders();

		CB_VS_PER_FRAME tmp;
		render_buffer_manager_.addCB(sizeof(CB_VS_PER_FRAME), 1, &tmp);
		CB_VS_PER_DP tmp2;
		render_buffer_manager_.addCB(sizeof(CB_VS_PER_DP), 1, &tmp2);
		for (size_t i = 0; i < 4; i++)
		{
			render_buffer_manager_.addRenderTargetView(sizeof(uint32_t), pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);
		}


		scene.buildBuffers(pd3dDevice);

		// Setup the camera's view parameters

		g_Camera.SetViewParams(&vecEye, &vecAt);
		g_Camera.SetRadius(23.0f, 1.5f, fObjectRadius * 30.0f);
	}


	{
		D3D11_RASTERIZER_DESC rasterDesc;
		rasterDesc.AntialiasedLineEnable = false;
		rasterDesc.CullMode = D3D11_CULL_NONE;
		rasterDesc.DepthBias = 0;
		rasterDesc.DepthBiasClamp = 0.0f;
		rasterDesc.DepthClipEnable = true;
		rasterDesc.FillMode = D3D11_FILL_SOLID;
		rasterDesc.FrontCounterClockwise = false;
		rasterDesc.MultisampleEnable = false;
		rasterDesc.ScissorEnable = false;
		rasterDesc.SlopeScaledDepthBias = 0.0f;
		V(pd3dDevice->CreateRasterizerState(&rasterDesc, &g_noculling_state));
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

	if (!kk_mode)
		ray_tracer.resize(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);
	{
		render_buffer_manager_.release();
		CB_VS_PER_FRAME tmp;
		render_buffer_manager_.addCB(sizeof(CB_VS_PER_FRAME), 1, &tmp);
		CB_VS_PER_DP tmp2;
		render_buffer_manager_.addCB(sizeof(CB_VS_PER_DP), 1, &tmp2);
		for (size_t i = 0; i < 4; i++)
		{
			render_buffer_manager_.addRenderTargetView(sizeof(uint32_t), pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);
		}
	}
	return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	float speed = 5;
	g_Camera.FrameMove(fElapsedTime);
	if (ImGui::GetIO().KeysDown['A'])
	{
		vecEye.x -= fElapsedTime * speed;
	}
	if (ImGui::GetIO().KeysDown['D'])
	{
		vecEye.x += fElapsedTime * speed;
	}

	if (ImGui::GetIO().KeysDown['Q'])
	{
		vecEye.y -= fElapsedTime * speed;
	}
	if (ImGui::GetIO().KeysDown['E'])
	{
		vecEye.y += fElapsedTime * speed;
	}

	if (ImGui::GetIO().KeysDown['W'])
	{
		vecEye.z -= fElapsedTime * speed;
	}
	if (ImGui::GetIO().KeysDown['S'])
	{
		vecEye.z += fElapsedTime * speed;
	}


	//


	if (ImGui::GetIO().KeysDown['H'])
	{
		vecAt.x -= fElapsedTime * speed;
	}
	if (ImGui::GetIO().KeysDown['K'])
	{
		vecAt.x += fElapsedTime * speed;
	}

	if (ImGui::GetIO().KeysDown['Y'])
	{
		vecAt.y -= fElapsedTime * speed;
	}
	if (ImGui::GetIO().KeysDown['I'])
	{
		vecAt.y += fElapsedTime * speed;
	}

	if (ImGui::GetIO().KeysDown['U'])
	{
		vecAt.z -= fElapsedTime * speed;
	}
	if (ImGui::GetIO().KeysDown['J'])
	{
		vecAt.z += fElapsedTime * speed;
	}


	//g_Camera.SetViewParams(&vecEye, &vecAt);
	//g_Camera.SetRadius(23.0f, 1.5f, fObjectRadius * 30.0f);
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
                                 double fTime, float fElapsedTime, void* pUserContext)
{
	HRESULT hr;
	// Clear render target and the depth stencil 
	float ClearColor[4] = {0, 0, 0, 0.0f};

	ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
	ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
	pd3dImmediateContext->ClearRenderTargetView(pRTV, ClearColor);
	pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0, 0);

	pd3dImmediateContext->RSSetState(g_noculling_state);

	if (!ray)
	{
		pd3dImmediateContext->ClearRenderTargetView(render_buffer_manager_.render_target_views[0], ClearColor);
		pd3dImmediateContext->ClearRenderTargetView(render_buffer_manager_.render_target_views[1], ClearColor);

		ID3D11RenderTargetView * rtv2[3] = { pRTV ,render_buffer_manager_.render_target_views[0],render_buffer_manager_.render_target_views[1]};
		pd3dImmediateContext->OMSetRenderTargets(3, rtv2, DXUTGetD3D11DepthStencilView());
		D3DXMATRIX mWorldViewProjection;
		D3DXVECTOR3 up = { 0,1,0 };
		D3DXVECTOR3 at = vecAt - vecEye;
		D3DXMATRIX mWorld;
		D3DXMATRIX mView;
		D3DXMATRIX mProj;
		D3DXMatrixTranslation(&mWorld, 0, 0, 0);
		//mWorld = *g_Camera.GetWorldMatrix();
		mProj = *g_Camera.GetProjMatrix();
		//mView = *g_Camera.GetViewMatrix();
		D3DXMatrixLookAtLH(&mView,&vecEye,&vecAt,&up);
		mWorldViewProjection = mWorld * mView * mProj;


		D3D11_MAPPED_SUBRESOURCE MappedResource;
		V(pd3dImmediateContext->Map(render_buffer_manager_.constant_buffers_[0], 0, D3D11_MAP_WRITE_DISCARD, 0, &
			MappedResource));
		CB_VS_PER_FRAME* cb_vs_per_frame = (CB_VS_PER_FRAME*)MappedResource.pData;
		D3DXMatrixTranspose(&cb_vs_per_frame->m_WorldViewProj, &mWorldViewProjection);
		D3DXMatrixTranspose(&cb_vs_per_frame->m_World, &mWorld);
		pd3dImmediateContext->Unmap(render_buffer_manager_.constant_buffers_[0], 0);

		pd3dImmediateContext->VSSetShader(shader_manager_.vertex_shader_, nullptr, 0);
		pd3dImmediateContext->PSSetShader(shader_manager_.pixel_shader_, nullptr, 0);
		pd3dImmediateContext->IASetInputLayout(shader_manager_.vertex_layout_);
		pd3dImmediateContext->VSSetConstantBuffers(0, 1, &render_buffer_manager_.constant_buffers_[0]);
		pd3dImmediateContext->PSSetConstantBuffers(0, 1, &render_buffer_manager_.constant_buffers_[0]);
		pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		for (auto&& geo : scene.geos)
		{
			uint32_t stride = sizeof(xyzn);
			uint32_t offset = 0;
			pd3dImmediateContext->IASetVertexBuffers(0, 1, &geo.vertex_buffer_, &stride, &offset);
			for (int i = 0; i < geo.indicess.size(); ++i)
			{
				V(pd3dImmediateContext->Map(render_buffer_manager_.constant_buffers_[1], 0, D3D11_MAP_WRITE_DISCARD, 0, &
					MappedResource));
				CB_VS_PER_DP* cb_vs_per_dp = (CB_VS_PER_DP*)MappedResource.pData;
				auto matid = geo.material_ids[i];
				cb_vs_per_dp->color.v[0] = scene.materials_[matid].diffuse[0];
				cb_vs_per_dp->color.v[1] = scene.materials_[matid].diffuse[1];
				cb_vs_per_dp->color.v[2] = scene.materials_[matid].diffuse[2];
				cb_vs_per_dp->matid = matid;
				pd3dImmediateContext->Unmap(render_buffer_manager_.constant_buffers_[1], 0);
				pd3dImmediateContext->PSSetConstantBuffers(1, 1, &render_buffer_manager_.constant_buffers_[1]);


				pd3dImmediateContext->IASetIndexBuffer(geo.index_buffers_[i], DXGI_FORMAT_R32_UINT, 0);
				pd3dImmediateContext->DrawIndexed(geo.indicess[i].size(), 0, 0);
			}
		}
		{
			{
				ID3D11Resource* r;
				pRTV->GetResource(&r);
				pd3dImmediateContext->CopyResource(ray_tracer.textures_[ray_tracer.gbuffer_tex_index], r);
				SAFE_RELEASE(r);
			}

			{
				ID3D11Resource* r;
				render_buffer_manager_.render_target_views[0]->GetResource(&r);
				pd3dImmediateContext->CopyResource(ray_tracer.textures_[ray_tracer.gbuffer2_tex_index], r);
				SAFE_RELEASE(r);
			}

			{
				ID3D11Resource* r;
				render_buffer_manager_.render_target_views[1]->GetResource(&r);
				pd3dImmediateContext->CopyResource(ray_tracer.textures_[ray_tracer.gbuffer3_tex_index], r);
				SAFE_RELEASE(r);
			}
		}

		pd3dImmediateContext->OMSetRenderTargets(1, rtv2, DXUTGetD3D11DepthStencilView());
	}



	if (ray)
	{
		/* create query for synchronous dispatches */
		D3D11_QUERY_DESC pQueryDesc;
		pQueryDesc.Query = D3D11_QUERY_EVENT;
		pQueryDesc.MiscFlags = 0;
		ID3D11Query* pEventQuery;
		pd3dDevice->CreateQuery(&pQueryDesc, &pEventQuery);

		D3D11_MAPPED_SUBRESOURCE MappedResource;
		V(pd3dImmediateContext->Map(ray_tracer.constant_buffers_[0], 0, D3D11_MAP_WRITE_DISCARD, 0, &
			MappedResource));
		CB_Radix* cb_vs_per_frame = (CB_Radix*)MappedResource.pData;
		cb_vs_per_frame->pos.v[0] = vecEye.x;
		cb_vs_per_frame->pos.v[1] = vecEye.y;
		cb_vs_per_frame->pos.v[2] = vecEye.z;
		cb_vs_per_frame->node_count = ray_tracer.primitive_count;

		int mat_i = 0;
		for (auto && material : scene.materials_)
		{
			auto& mat = cb_vs_per_frame->mats[mat_i++];
			mat.ke = material.emission[0] * 10;
			mat.ns = material.shininess;
			if(mat.ke <= 0)
			{
				mat.kd.v[0] = material.diffuse[0];
				mat.kd.v[1] = material.diffuse[1];
				mat.kd.v[2] = material.diffuse[2];
			}
			else
			{
				mat.kd = { 1,1,1 };
			}
			mat.tr = 0;
			mat.ni = material.ior;
		}

		pd3dImmediateContext->Unmap(ray_tracer.constant_buffers_[0], 0);

		//for (int i = 0; i < min(ImGui::GetIO().Framerate / 4,1); ++i)
		do
		{
			ray_tracer.run(pd3dImmediateContext);
			//pd3dImmediateContext->CopyResource(ray_tracer.textures_[ray_tracer.old_tex_index], ray_tracer.textures_[ray_tracer.output_tex_index]);
			if (ImGui::GetIO().KeysDown['P'] ||
				(ray_tracer.frame_count == 50) ||
				(ray_tracer.frame_count == 100) ||
				(ray_tracer.frame_count == 200) ||
				(ray_tracer.frame_count == 500) ||
				(ray_tracer.frame_count == 1000) ||
				(ray_tracer.frame_count == 1500) ||
				(ray_tracer.frame_count == 2000) ||
				(ray_tracer.frame_count == 3000) ||
				(ray_tracer.frame_count == 5000) ||
				(ray_tracer.frame_count == 8000) ||
				(ray_tracer.frame_count == 10000) ||
				(ray_tracer.frame_count == 12000) ||
				(ray_tracer.frame_count == 16000) ||
				(ray_tracer.frame_count == 20000)
			)
			{
				save_file = true;
				break;
			}

			if (ray_tracer.frame_count > 20002)
			{
				exit(0);
			}

			if (ImGui::GetIO().KeysDown['Z'])
			{
				ray = false;
				break;
			}
		}
		while (false);

		pd3dImmediateContext->End(pEventQuery);
		pEventQuery->Release();
		//Sleep(700);
	}

	if (!kk_mode && ray)
	{
		ID3D11Resource* r;
		pRTV->GetResource(&r);
		pd3dImmediateContext->CopyResource(r, ray_tracer.textures_[ray_tracer.output_tex_index]);
		SAFE_RELEASE(r);
	}

	if (save_file)
	{
		wchar_t tmp[255];
		wsprintf(tmp, L"%dX%dframe_%d.bmp", ray_tracer.width, ray_tracer.height, ray_tracer.frame_count);
		D3DX11SaveTextureToFile(pd3dImmediateContext, ray_tracer.textures_[ray_tracer.old_tex_index], D3DX11_IFF_BMP, tmp);
		save_file = false;
	}


	ImGui_ImplDX11_NewFrame();
	{
		static float f = 0.0f;
		ImGui::Checkbox("Ray?", &ray);
		ImGui::Text("Traced Frames: %d", ray_tracer.frame_count);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
		            ImGui::GetIO().Framerate);
		static bool acc = true;
		ImGui::Checkbox("ACC?", &acc);
		if (!acc)
			ray_tracer.frame_count = 0;
		if (ImGui::Button("save file"))
			save_file = true;
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

	if (!ray)
	{
		// Pass all remaining windows messages to camera so it can respond to user input
		g_Camera.HandleMessages(hWnd, uMsg, wParam, lParam);
	}

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
	ray_tracer.release();
	SAFE_RELEASE(g_noculling_state);
	return DXUTGetExitCode();
}

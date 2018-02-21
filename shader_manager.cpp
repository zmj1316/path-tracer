#include "DXUT.h"
#include "shader_manager.hpp"

#include <stdio.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <cstdint>

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d3dcompiler.lib")

ShaderManager::ShaderManager(): vertex_shader_(nullptr), vertex_layout_(nullptr), pixel_shader_(nullptr)
{
}


ShaderManager::~ShaderManager()
{
	release();
}

HRESULT CompileComputeShader(_In_ LPCWSTR srcFile, _In_ LPCSTR entryPoint,
                                  _In_                                  ID3D11Device* device,
                                  _Outptr_ ID3D10Blob** blob)
{
	if (!srcFile || !entryPoint || !device || !blob)
		return E_INVALIDARG;

	*blob = nullptr;

	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	flags |= D3DCOMPILE_DEBUG;
#endif

	// We generally prefer to use the higher CS shader profile when possible as CS 5.0 is better performance on 11-class hardware
	LPCSTR profile = (device->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0) ? "cs_5_0" : "cs_4_0";

	const D3D10_SHADER_MACRO defines[] =
	{
		"EXAMPLE_DEFINE", "1",
		nullptr, nullptr
	};

	ID3D10Blob* shaderBlob = nullptr;
	ID3D10Blob* errorBlob = nullptr;
	HRESULT hr;
	V(D3DX11CompileFromFile(srcFile, defines, NULL, entryPoint, profile, flags, 0, nullptr, &shaderBlob, &errorBlob,nullptr
	));

	if (FAILED(hr))
	{
		if (errorBlob)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}

		if (shaderBlob)
			shaderBlob->Release();

		return hr;
	}

	*blob = shaderBlob;

	return hr;
}

static bool CreateComputeShader(LPCWSTR pSrcFile, LPCSTR pFunctionName,
                                ID3D11Device* pDevice, ID3D11ComputeShader** ppShaderOut)
{
	uint32_t dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.  
	// Setting this flag improves the shader debugging experience, but still allows   
	// the shaders to be optimized and to run exactly the way they will run in   
	// the release configuration of this program.  
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	const D3D_SHADER_MACRO defines[] =
	{
		"USE_STRUCTURED_BUFFERS", "1",
		nullptr, nullptr
	};

	// We generally prefer to use the higher CS shader profile when possible as CS 5.0 is better performance on 11-class hardware  
	ID3DBlob* pErrorBlob = nullptr;
	ID3DBlob* computeShader = nullptr;

	if (D3DX11CompileFromFile(pSrcFile, defines, nullptr, pFunctionName, "cs_5_0", dwShaderFlags, 0, nullptr,
	                          &computeShader, &pErrorBlob, nullptr) < 0)
	{
		if (pErrorBlob != nullptr)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());

		if (pErrorBlob != nullptr)
			pErrorBlob->Release();
		if (computeShader != nullptr)
			computeShader->Release();

		return false;
	}

	bool result = true;
	if (pDevice->CreateComputeShader(computeShader->GetBufferPointer(),
	                                 computeShader->GetBufferSize(), nullptr, ppShaderOut))
		result = false;

	if (pErrorBlob != nullptr)
		pErrorBlob->Release();
	if (computeShader != nullptr)
		computeShader->Release();

	return result;
}

static bool CreateVertexShader(LPCWSTR pSrcFile, LPCSTR pFunctionName, LPCSTR pProfile,
                               ID3D11Device* pDevice, ID3D11VertexShader** ppShaderOut,
                               ID3D11InputLayout** pp_vertex_layout_)
{
	uint32_t dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.  
	// Setting this flag improves the shader debugging experience, but still allows   
	// the shaders to be optimized and to run exactly the way they will run in   
	// the release configuration of this program.  
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	ID3DBlob* pErrorBlob = nullptr;
	ID3DBlob* shader_blob = nullptr;
	HRESULT hr;
	if (D3DX11CompileFromFile(pSrcFile, nullptr, nullptr, pFunctionName, pProfile, dwShaderFlags, 0, nullptr,
	                          &shader_blob, &pErrorBlob, nullptr) < 0)
	{
		if (pErrorBlob != nullptr)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());

		if (pErrorBlob != nullptr)
			pErrorBlob->Release();
		if (shader_blob != nullptr)
			shader_blob->Release();

		return false;
	}

	V_RETURN(pDevice->CreateVertexShader(shader_blob->GetBufferPointer(),
		shader_blob->GetBufferSize(), nullptr, ppShaderOut));
	const D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	V_RETURN(pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), shader_blob->GetBufferPointer(),
		shader_blob->GetBufferSize(), pp_vertex_layout_));
	if (pErrorBlob != nullptr)
		pErrorBlob->Release();
	if (shader_blob != nullptr)
		shader_blob->Release();

	return hr;
}

static bool CreatePixelShader(LPCWSTR pSrcFile, LPCSTR pFunctionName, LPCSTR pProfile,
                              ID3D11Device* pDevice, ID3D11PixelShader** ppShaderOut)
{
	uint32_t dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.  
	// Setting this flag improves the shader debugging experience, but still allows   
	// the shaders to be optimized and to run exactly the way they will run in   
	// the release configuration of this program.  
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	ID3DBlob* pErrorBlob = nullptr;
	ID3DBlob* shader_blob = nullptr;

	if (D3DX11CompileFromFile(pSrcFile, nullptr, nullptr, pFunctionName, pProfile, dwShaderFlags, 0, nullptr,
	                          &shader_blob, &pErrorBlob, nullptr) < 0)
	{
		if (pErrorBlob != nullptr)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());

		if (pErrorBlob != nullptr)
			pErrorBlob->Release();
		if (shader_blob != nullptr)
			shader_blob->Release();

		return false;
	}

	bool result = true;
	if (pDevice->CreatePixelShader(shader_blob->GetBufferPointer(),
	                               shader_blob->GetBufferSize(), nullptr, ppShaderOut))
		result = false;

	if (pErrorBlob != nullptr)
		pErrorBlob->Release();
	if (shader_blob != nullptr)
		shader_blob->Release();

	return result;
}

bool ShaderManager::CreateCS(LPCWSTR pSrcFile, LPCSTR pFunctionName)
{
	ID3D11ComputeShader * shader;
	if (CreateComputeShader(pSrcFile, pFunctionName, DXUTGetD3D11Device(), &shader))
	{
		compute_shaders_.push_back(shader);
		return true;
	}
	return false;
}

void ShaderManager::CreatePipelineShaders()
{
	CreateVertexShader(L"fixed_pipeline.hlsl", "VSMain", "vs_5_0", DXUTGetD3D11Device(), &vertex_shader_,&vertex_layout_);
	CreatePixelShader(L"fixed_pipeline.hlsl", "PSMain", "ps_5_0", DXUTGetD3D11Device(), &pixel_shader_);
}

void ShaderManager::release()
{
	for (auto && compute_shader : compute_shaders_)
	{
		SAFE_RELEASE(compute_shader);
	}
	SAFE_RELEASE(vertex_shader_);
	SAFE_RELEASE(vertex_layout_);
	SAFE_RELEASE(pixel_shader_);
}

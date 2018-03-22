#include "DXUT.h"
#include "buffer_manager.hpp"
#include <cstdint>


BufferManager::BufferManager()
{
}


BufferManager::~BufferManager()
{
	release();
}

static bool CreateConstantBuffer(ID3D11Device* pDevice, uint32_t nBytes, void* pInitData, ID3D11Buffer** ppBufferOut)
{
	*ppBufferOut = nullptr;

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.ByteWidth = nBytes;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = pInitData;
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;

	return pDevice->CreateBuffer(&desc, &initData, ppBufferOut) >= 0;
}

static bool CreateStructureBuffer(ID3D11Device* pDevice, uint32_t elementSize, uint32_t uCount,
                                  void* pInitData, ID3D11Buffer** ppBufferOut)
{
	*ppBufferOut = nullptr;

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	desc.ByteWidth = elementSize * uCount;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = elementSize;

	if (pInitData != nullptr)
	{
		D3D11_SUBRESOURCE_DATA InitData = {nullptr};
		InitData.pSysMem = pInitData;
		return pDevice->CreateBuffer(&desc, &InitData, ppBufferOut) >= 0;
	}
	return pDevice->CreateBuffer(&desc, nullptr, ppBufferOut) >= 0;
}

static bool CreateByteAddressBuffer(ID3D11Device* pDevice, uint32_t elementSize, uint32_t uCount,
                                    void* pInitData, ID3D11Buffer** ppBufferOut)
{
	*ppBufferOut = nullptr;

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	desc.ByteWidth = elementSize * uCount;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	desc.StructureByteStride = elementSize;

	if (pInitData != nullptr)
	{
		D3D11_SUBRESOURCE_DATA InitData = {nullptr};
		InitData.pSysMem = pInitData;
		return pDevice->CreateBuffer(&desc, &InitData, ppBufferOut) >= 0;
	}
	return pDevice->CreateBuffer(&desc, nullptr, ppBufferOut) >= 0;
}

static bool CreateTextureBuffer(ID3D11Device* pDevice, uint32_t stride, uint32_t width, uint32_t height,
                                ID3D11Texture2D** ppBufferOut)
{
	*ppBufferOut = nullptr;

	D3D11_TEXTURE2D_DESC  desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	desc.MiscFlags = 0;
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.SampleDesc.Count = 1;


	return pDevice->CreateTexture2D(&desc, nullptr, ppBufferOut) >= 0;
}

static bool CreateTextureBufferWritable(ID3D11Device* pDevice, uint32_t stride, uint32_t width, uint32_t height,
	ID3D11Texture2D** ppBufferOut)
{
	*ppBufferOut = nullptr;

	D3D11_TEXTURE2D_DESC  desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.MiscFlags = 0;
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DYNAMIC;

	return pDevice->CreateTexture2D(&desc, nullptr, ppBufferOut) >= 0;
}

static bool CreateTextureBufferFloat4(ID3D11Device* pDevice, uint32_t stride, uint32_t width, uint32_t height,
	ID3D11Texture2D** ppBufferOut)
{
	*ppBufferOut = nullptr;

	D3D11_TEXTURE2D_DESC  desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.MiscFlags = 0;
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.CPUAccessFlags = 0;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;

	return pDevice->CreateTexture2D(&desc, nullptr, ppBufferOut) >= 0;
}

static bool CreateTextureBufferRT(ID3D11Device* pDevice, uint32_t stride, uint32_t width, uint32_t height,
	ID3D11Texture2D** ppBufferOut)
{
	*ppBufferOut = nullptr;

	D3D11_TEXTURE2D_DESC  desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.BindFlags = D3D11_BIND_RENDER_TARGET;
	desc.MiscFlags = 0;
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.CPUAccessFlags = 0;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;

	return pDevice->CreateTexture2D(&desc, nullptr, ppBufferOut) >= 0;
}

static bool CreateBufferSRV(ID3D11Device* pDevice, ID3D11Buffer* pBuffer, ID3D11ShaderResourceView** ppSRVOut)
{
	D3D11_BUFFER_DESC descBuf;
	ZeroMemory(&descBuf, sizeof(descBuf));
	pBuffer->GetDesc(&descBuf);

	D3D11_SHADER_RESOURCE_VIEW_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	desc.BufferEx.FirstElement = 0;

	//假定这是个structure buffer  
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.BufferEx.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride;

	return pDevice->CreateShaderResourceView((ID3D11Resource*)pBuffer, &desc, ppSRVOut) >= 0;
}

static bool CreateBufferUAV(ID3D11Device* pDevice, ID3D11Buffer* pBuffer, ID3D11UnorderedAccessView** ppUAVOut)
{
	D3D11_BUFFER_DESC descBuf;
	ZeroMemory(&descBuf, sizeof(descBuf));
	pBuffer->GetDesc(&descBuf);

	D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	desc.Buffer.FirstElement = 0;
	//假设这是一个structure buffer  
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.Buffer.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride;

	return pDevice->CreateUnorderedAccessView((ID3D11Resource*)pBuffer, &desc, ppUAVOut) >= 0;
}

static bool CreateByteBufferUAV(ID3D11Device* pDevice, ID3D11Buffer* pBuffer, ID3D11UnorderedAccessView** ppUAVOut)
{
	D3D11_BUFFER_DESC descBuf;
	ZeroMemory(&descBuf, sizeof(descBuf));
	pBuffer->GetDesc(&descBuf);

	D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	desc.Buffer.FirstElement = 0;
	desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;

	desc.Format = DXGI_FORMAT_R32_TYPELESS;
	desc.Buffer.NumElements = descBuf.ByteWidth / sizeof(int32_t);

	return pDevice->CreateUnorderedAccessView((ID3D11Resource*)pBuffer, &desc, ppUAVOut) >= 0;
}

static bool CreateTextureUAV(ID3D11Device* pDevice, ID3D11Texture2D* pBuffer, ID3D11UnorderedAccessView** ppUAVOut)
{
	D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Texture2D.MipSlice = 0;

	return pDevice->CreateUnorderedAccessView((ID3D11Resource*)pBuffer, &desc, ppUAVOut) >= 0;
}

static bool CreateTextureUAVFloat4(ID3D11Device* pDevice, ID3D11Texture2D* pBuffer, ID3D11UnorderedAccessView** ppUAVOut)
{
	D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.Texture2D.MipSlice = 0;

	return pDevice->CreateUnorderedAccessView((ID3D11Resource*)pBuffer, &desc, ppUAVOut) >= 0;
}

static bool CreateTextureSRV(ID3D11Device* pDevice, ID3D11Texture2D* pBuffer, ID3D11ShaderResourceView** ppUAVOut)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipLevels = 1;

	return pDevice->CreateShaderResourceView((ID3D11Resource*)pBuffer, &desc, ppUAVOut) >= 0;
}

static bool CreateTextureRTV(ID3D11Device* pDevice, ID3D11Texture2D* pBuffer, ID3D11RenderTargetView** ppUAVOut)
{
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	renderTargetViewDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	return pDevice->CreateRenderTargetView((ID3D11Resource*)pBuffer, &renderTargetViewDesc, ppUAVOut) >= 0;
}

void BufferManager::addSRV(uint32_t strip, uint32_t count, void* data)
{
	ID3D11Buffer* buffer;
	CreateStructureBuffer(DXUTGetD3D11Device(), strip, count, data, &buffer);
	buffers_.emplace_back(buffer);

	ID3D11ShaderResourceView* srv;
	CreateBufferSRV(DXUTGetD3D11Device(), buffer, &srv);
	shader_resource_views.emplace_back(srv);
}

void BufferManager::addStructUAV(uint32_t strip, uint32_t count, void* data)
{
	ID3D11Buffer* buffer;
	CreateStructureBuffer(DXUTGetD3D11Device(), strip, count, data, &buffer);
	buffers_.emplace_back(buffer);

	ID3D11UnorderedAccessView* srv;
	CreateBufferUAV(DXUTGetD3D11Device(), buffer, &srv);
	unordered_access_views.emplace_back(srv);
}

void BufferManager::addByteUAV(uint32_t strip, uint32_t count, void* data)
{
	ID3D11Buffer* buffer;
	CreateByteAddressBuffer(DXUTGetD3D11Device(), strip, count, data, &buffer);
	buffers_.emplace_back(buffer);

	ID3D11UnorderedAccessView* srv;
	CreateByteBufferUAV(DXUTGetD3D11Device(), buffer, &srv);
	unordered_access_views.emplace_back(srv);
}

void BufferManager::addTextureUAV(uint32_t strip, uint32_t width, uint32_t height)
{
	ID3D11Texture2D* buffer;
	CreateTextureBuffer(DXUTGetD3D11Device(), strip, width, height, &buffer);
	textures_.emplace_back(buffer);

	ID3D11UnorderedAccessView* srv;
	CreateTextureUAV(DXUTGetD3D11Device(), buffer, &srv);
	unordered_access_views.emplace_back(srv);
}

void BufferManager::addTextureUAVFloat4(uint32_t strip, uint32_t width, uint32_t height)
{
	ID3D11Texture2D* buffer;
	CreateTextureBufferFloat4(DXUTGetD3D11Device(), strip, width, height, &buffer);
	textures_.emplace_back(buffer);

	ID3D11UnorderedAccessView* srv;
	CreateTextureUAVFloat4(DXUTGetD3D11Device(), buffer, &srv);
	unordered_access_views.emplace_back(srv);
}

void BufferManager::addTextureSRV(uint32_t strip, uint32_t width, uint32_t height)
{
	ID3D11Texture2D* buffer;
	CreateTextureBuffer(DXUTGetD3D11Device(), strip, width, height, &buffer);
	textures_.emplace_back(buffer);

	ID3D11ShaderResourceView* srv;
	CreateTextureSRV(DXUTGetD3D11Device(), buffer, &srv);
	shader_resource_views.emplace_back(srv);
}

void BufferManager::addTextureSRVFloat4(uint32_t strip, uint32_t width, uint32_t height)
{
	ID3D11Texture2D* buffer;
	CreateTextureBufferFloat4(DXUTGetD3D11Device(), strip, width, height, &buffer);
	textures_.emplace_back(buffer);

	ID3D11ShaderResourceView* srv;
	CreateTextureSRV(DXUTGetD3D11Device(), buffer, &srv);
	shader_resource_views.emplace_back(srv);
}

void BufferManager::addTextureSRVWritable(uint32_t strip, uint32_t width, uint32_t height)
{
	ID3D11Texture2D* buffer;
	CreateTextureBufferWritable(DXUTGetD3D11Device(), strip, width, height, &buffer);
	textures_.emplace_back(buffer);

	ID3D11ShaderResourceView* srv;
	CreateTextureSRV(DXUTGetD3D11Device(), buffer, &srv);
	shader_resource_views.emplace_back(srv);
}

void BufferManager::addRenderTargetView(uint32_t strip, uint32_t width, uint32_t height)
{
	ID3D11Texture2D* buffer;
	CreateTextureBufferRT(DXUTGetD3D11Device(), strip, width, height, &buffer);
	textures_.emplace_back(buffer);

	ID3D11RenderTargetView* srv;
	CreateTextureRTV(DXUTGetD3D11Device(), buffer, &srv);
	render_target_views.emplace_back(srv);
}

void BufferManager::addCB(uint32_t strip, uint32_t count, void* data)
{
	ID3D11Buffer* buffer;
	CreateConstantBuffer(DXUTGetD3D11Device(), strip * count, data, &buffer);
	DXUT_SetDebugName(buffer, "CB_in_manager");
	constant_buffers_.push_back(buffer);
}

#define VECTOR_RELEASE(vec) for (auto&& i : vec) { if (i) { (i)->Release(); (i)=NULL; } } vec.clear();

void BufferManager::release()
{
	VECTOR_RELEASE(shader_resource_views);
	VECTOR_RELEASE(unordered_access_views);
	VECTOR_RELEASE(render_target_views);
	VECTOR_RELEASE(constant_buffers_);
	VECTOR_RELEASE(buffers_);
	VECTOR_RELEASE(textures_);
}

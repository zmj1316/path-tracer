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
	*ppBufferOut = NULL;

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
	*ppBufferOut = NULL;

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	desc.ByteWidth = elementSize*uCount;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = elementSize;

	if (pInitData != NULL)
	{
		D3D11_SUBRESOURCE_DATA InitData = { 0 };
		InitData.pSysMem = pInitData;
		return pDevice->CreateBuffer(&desc, &InitData, ppBufferOut) >= 0;
	}
	else
		return pDevice->CreateBuffer(&desc, NULL, ppBufferOut) >= 0;
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


void BufferManager::addSRV(uint32_t strip, uint32_t count, void* data)
{
	ID3D11Buffer* buffer;
	CreateStructureBuffer(DXUTGetD3D11Device(), strip, count, data,&buffer);
	buffers_.emplace_back(buffer);

	ID3D11ShaderResourceView* srv;
	CreateBufferSRV(DXUTGetD3D11Device(), buffer, &srv);
	shader_resource_views.emplace_back(srv);
}

void BufferManager::addUAV(uint32_t strip, uint32_t count, void* data)
{
	ID3D11Buffer* buffer;
	CreateStructureBuffer(DXUTGetD3D11Device(), strip, count, data, &buffer);
	buffers_.emplace_back(buffer);

	ID3D11UnorderedAccessView* srv;
	CreateBufferUAV(DXUTGetD3D11Device(), buffer, &srv);
	unordered_access_views.emplace_back(srv);
}

void BufferManager::addCB(uint32_t strip, uint32_t count, void* data)
{
	ID3D11Buffer* buffer;
	CreateConstantBuffer(DXUTGetD3D11Device(), strip * count, data, &buffer);
	constant_buffers_.push_back(buffer);
}


void BufferManager::release()
{
	for (auto && shader_resource_view : shader_resource_views)
	{
		SAFE_RELEASE(shader_resource_view);
	}
	shader_resource_views.clear();

	for (auto && unordered_access_view : unordered_access_views)
	{
		SAFE_RELEASE(unordered_access_view);
	}
	unordered_access_views.clear();

	for (auto && buffer : buffers_)
	{
		SAFE_RELEASE(buffer);
	}
	buffers_.clear();

	for (auto && buffer : constant_buffers_)
	{
		SAFE_RELEASE(buffer);
	}
	constant_buffers_.clear();

}

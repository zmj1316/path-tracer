#pragma once
#include <vector>


class BufferManager
{
public:
	BufferManager();
	~BufferManager();

	std::vector<ID3D11ShaderResourceView*> shader_resource_views;
	std::vector<ID3D11UnorderedAccessView*> unordered_access_views;

	std::vector<ID3D11Buffer*> buffers_;

	std::vector<ID3D11Buffer*> constant_buffers_;


	void addSRV(uint32_t strip, uint32_t count, void* data);
	void addUAV(uint32_t strip, uint32_t count, void* data);
	void addCB(uint32_t strip, uint32_t count, void* data);
	void release();
};


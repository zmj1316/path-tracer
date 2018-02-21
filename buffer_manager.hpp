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
	std::vector<ID3D11Texture2D*> textures_;

	std::vector<ID3D11Buffer*> constant_buffers_;


	void addSRV(uint32_t strip, uint32_t count, void* data);
	void addStructUAV(uint32_t strip, uint32_t count, void* data);
	void addByteUAV(uint32_t strip, uint32_t count, void* data);
	void addTextureUAV(uint32_t strip, uint32_t width, uint32_t height);
	void addCB(uint32_t strip, uint32_t count, void* data);
	void release();
};

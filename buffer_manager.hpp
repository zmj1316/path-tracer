#pragma once
#include <vector>


class BufferManager
{
public:
	BufferManager();
	~BufferManager();

	std::vector<ID3D11ShaderResourceView*> shader_resource_views;
	std::vector<ID3D11UnorderedAccessView*> unordered_access_views;
	std::vector<ID3D11RenderTargetView*> render_target_views;

	std::vector<ID3D11Buffer*> buffers_;
	std::vector<ID3D11Texture2D*> textures_;

	std::vector<ID3D11Buffer*> constant_buffers_;


	void addSRV(uint32_t strip, uint32_t count, void* data);
	void addStructUAV(uint32_t strip, uint32_t count, void* data);
	void addByteUAV(uint32_t strip, uint32_t count, void* data);
	void addTextureUAV(uint32_t strip, uint32_t width, uint32_t height);
	void addTextureUAVFloat4(uint32_t strip, uint32_t width, uint32_t height);
	void addTextureSRV(uint32_t strip, uint32_t width, uint32_t height);
	void addTextureSRVFloat4(uint32_t strip, uint32_t width, uint32_t height);
	void addTextureSRVWritable(uint32_t strip, uint32_t width, uint32_t height);
	void addRenderTargetView(uint32_t strip, uint32_t width, uint32_t height);
	void addCB(uint32_t strip, uint32_t count, void* data);
	void release();
};


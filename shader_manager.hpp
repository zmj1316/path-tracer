#pragma once
#include <vector>

class ShaderManager
{
public:
	ShaderManager();
	~ShaderManager();

	bool ShaderManager::CreateCS(LPCWSTR pSrcFile, LPCSTR pFunctionName);

	void CreatePipelineShaders();
	std::vector<ID3D11ComputeShader *> compute_shaders_;
	ID3D11VertexShader* vertex_shader_;
	ID3D11InputLayout* vertex_layout_;
	ID3D11PixelShader* pixel_shader_;
	void release();
private:
};

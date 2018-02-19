#pragma once
class ShaderManager
{
public:
	ShaderManager();
	~ShaderManager();

	void CreateCS();

	void CreatePipelineShaders();
	ID3D11ComputeShader * compute_shader_;
	ID3D11VertexShader * vertex_shader_;
	ID3D11InputLayout*          vertex_layout_;
	ID3D11PixelShader * pixel_shader_;
	void release();
private:
};


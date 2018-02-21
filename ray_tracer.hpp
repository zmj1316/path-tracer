#pragma once
#include "shader_manager.hpp"
#include "buffer_manager.hpp"
#include "scene.hpp"

class RayTracer : public ShaderManager, public BufferManager
{
public:
	RayTracer();
	~RayTracer();

	int primitive_count = 0;

	int radix_build_shader_index = -1;
	int bvh_build_shader_index = -1;
	int ray_shader_index = -1;

	int tree_uav_index = -1;
	int radix_cb_index = -1;

	int primitive_ray_srv_index = -1;
	int bvh_build_lock_uav_index = -1;

	int primitive_render_srv_index = -1;
	int output_tex_uav_index = -1;
	int rt_cb_index = -1;

	const LPCWSTR RADIX_SHADER_NAME = L"radix_build_cs.hlsl";
	const LPCWSTR BVH_SHADER_NAME = L"bvh_build_cs.hlsl";
	const LPCWSTR RAY_SHADER_NAME = L"ray_tracing.hlsl";

	Scene scene;
	Bound bb_;
	std::vector<PrimitiveRender> renderer_primitives;
	std::vector<PrimitiveRay> ray_primitives;
	std::vector<PrimitiveRayCS> ray_cs_primitives;

	void loadScene();
	void loadShaders();
	void createBuffers();
	ID3D11DeviceContext* pd3dImmediateContext_;
	void RayTracer::run(ID3D11DeviceContext* pd3dImmediateContext);

	void buildRadixTree();
	void buildBVH();
	void trace();

	void finish();

	void release();
};


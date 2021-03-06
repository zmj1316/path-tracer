#pragma once
#include "shader_manager.hpp"
#include "buffer_manager.hpp"
#include "scene.hpp"
#include <unordered_map>

class RayTracer : public ShaderManager, public BufferManager
{
public:
	RayTracer();
	~RayTracer();
	int frame_count = 0;
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
	int output_unorm_tex_uav_index = -1;
	int rt_cb_index = -1;

	int old_tex_index = -1;
	int old_tex_srv_index = -1;

	int halton_tex_index = -1;
	int halton_tex_srv_index = -1;

	const LPCWSTR RADIX_SHADER_NAME = L"radix_build_cs.hlsl";
	const LPCWSTR BVH_SHADER_NAME = L"bvh_build_cs.hlsl";
	const LPCWSTR RAY_SHADER_NAME = L"ray_tracing.hlsl";

	Scene scene;
	Bound bb_;
	std::vector<PrimitiveRender> renderer_primitives;
	std::vector<PrimitiveRay> ray_primitives;
	std::vector<PrimitiveRayCS> ray_cs_primitives;
	int output_tex_index = -1;
	int output_unorm_tex_index = -1;

	int gbuffer_tex_index = -1;
	int gbuffer_srv_index = -1;

	int gbuffer2_tex_index = -1;
	int gbuffer2_srv_index = -1;

	int gbuffer3_tex_index = -1;
	int gbuffer3_srv_index = -1;

	int gbuffer4_tex_index = -1;
	int gbuffer4_srv_index = -1;

	void loadScene(const char * filename);
	void loadShaders();
	void createBuffers();
	ID3D11DeviceContext* pd3dImmediateContext_;
	void RayTracer::run(ID3D11DeviceContext* pd3dImmediateContext);
	void resize(int width, int height);

	void buildRadixTree();
	void buildBVH();
	void trace();

	void finish();

	void release();

	int width = 640;
	int height = 480;
	std::unordered_map<std::string, std::string> shader_define_values;
};


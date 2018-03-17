#include "DXUT.h"
#include "ray_tracer.hpp"
#include "structs.hpp"
#include <algorithm>    // std::sort
#include <random>
#include "halton_helper.hpp"


RayTracer::RayTracer(): pd3dImmediateContext_(nullptr)
{
}


RayTracer::~RayTracer()
{
}


void RayTracer::loadScene()
{
	scene.loadObj("scene03.obj");


	renderer_primitives.resize(scene.triangle_count);
	ray_primitives.resize(scene.triangle_count);

	primitive_count = 0;
	for (auto&& geo : scene.geos)
	{
		for (int pg_index = 0; pg_index < geo.indicess.size(); ++pg_index)
		{
			const auto& indices = geo.indicess[pg_index];
			for (int triangle_index = 0; triangle_index < indices.size() / 3; ++triangle_index)
			{
				PrimitiveRender& prd = renderer_primitives[primitive_count + triangle_index];
				prd.matid = geo.material_ids[pg_index];
				prd.pad = -1;
				prd.vertices[0] = geo.vertices_xyzn[indices[triangle_index * 3 + 0]];
				prd.vertices[1] = geo.vertices_xyzn[indices[triangle_index * 3 + 1]];
				prd.vertices[2] = geo.vertices_xyzn[indices[triangle_index * 3 + 2]];

				PrimitiveRay& prr = ray_primitives[primitive_count + triangle_index];
				prr.index = primitive_count + triangle_index;
				prr.bound.min = bound_min(prd.vertices[0].pos, prd.vertices[1].pos, prd.vertices[2].pos);
				prr.bound.max = bound_max(prd.vertices[0].pos, prd.vertices[1].pos, prd.vertices[2].pos);
			}
			primitive_count += indices.size() / 3;
		}
	}

	bb_ = ray_primitives[0].bound;
	for (auto&& ray_primitive : ray_primitives)
	{
		mergeBound(bb_, ray_primitive.bound);
	}

	for (auto&& ray_primitive : ray_primitives)
	{
		ray_primitive.morton_code = genMortonCode(ray_primitive.bound, bb_);
	}

	std::sort(ray_primitives.begin(), ray_primitives.end(), [](auto x, auto y) { return x.morton_code < y.morton_code; });
	ray_cs_primitives.reserve(ray_primitives.size());
	for (auto&& ray_primitive : ray_primitives)
	{
		ray_cs_primitives.push_back({ray_primitive.bound,ray_primitive.index});
	}

	shader_define_values["MAT_COUNT"] = std::to_string(scene.materials_.size());
	shader_define_values["MAT_ITR"] = "5";
}

void RayTracer::loadShaders()
{
	const D3D10_SHADER_MACRO defines[] = {
		nullptr,nullptr
	};
	if (CreateCS(RADIX_SHADER_NAME, "CSMain", defines))
	{
		radix_build_shader_index = compute_shaders_.size() - 1;
	}
	if (CreateCS(BVH_SHADER_NAME, "CSMain", defines))
	{
		bvh_build_shader_index = compute_shaders_.size() - 1;
	}
	if (CreateCS(RAY_SHADER_NAME, "CSMain", defines))
	{
		ray_shader_index = compute_shaders_.size() - 1;
	}
}

void RayTracer::createBuffers()
{
	{
		CB_Radix rcb;
		rcb.node_count = primitive_count;
		addCB(sizeof(CB_Radix), 1, &rcb);
		radix_cb_index = constant_buffers_.size() - 1;
	}

	{
		auto tmp = malloc(sizeof(TreeNode) * primitive_count * 2);
		memset(tmp, 0, sizeof(TreeNode) * primitive_count * 2);
		addStructUAV(sizeof(TreeNode), primitive_count * 2, tmp);
		free(tmp);
		tree_uav_index = unordered_access_views.size() - 1;
	}

	{
		addSRV(sizeof(PrimitiveRayCS), primitive_count, ray_cs_primitives.data());
		primitive_ray_srv_index = shader_resource_views.size() - 1;
	}

	{
		auto tmp = malloc(sizeof(int32_t) * primitive_count);
		memset(tmp, 0, sizeof(int32_t) * (primitive_count));
		addByteUAV(sizeof(int32_t), primitive_count, tmp);
		free(tmp);
		bvh_build_lock_uav_index = unordered_access_views.size() - 1;
	}

	{
		CB_RT cbrt;
		//D3DXMatrixIdentity(&cbrt.invModel);
		//D3DXMatrixIdentity(&cbrt.invView);
		cbrt.tanHalfFovY = 0;
		cbrt.viewportDims.x = width;
		cbrt.viewportDims.y = height;
		addCB(sizeof(CB_RT), 1, &cbrt);
		rt_cb_index = constant_buffers_.size() - 1;
	}

	{
		addTextureUAV(sizeof(uint32_t), width, height);
		output_tex_index = textures_.size() - 1;
		output_tex_uav_index = unordered_access_views.size() - 1;
	}

	{
		addSRV(sizeof(PrimitiveRender), renderer_primitives.size(), renderer_primitives.data());
		primitive_render_srv_index = shader_resource_views.size() - 1;
	}

	{
		addTextureSRV(sizeof(uint32_t), width, height);
		old_tex_index = textures_.size() - 1;
		old_tex_srv_index = shader_resource_views.size() - 1;
	}

	{
		addTextureSRV(sizeof(uint32_t), width, height);
		gbuffer_tex_index = textures_.size() - 1;
		gbuffer_srv_index = shader_resource_views.size() - 1;
	}

	{
		addTextureSRVFloat4(sizeof(uint32_t), width, height);
		gbuffer2_tex_index = textures_.size() - 1;
		gbuffer2_srv_index = shader_resource_views.size() - 1;
	}

	{
		addTextureSRVFloat4(sizeof(uint32_t), width, height);
		gbuffer3_tex_index = textures_.size() - 1;
		gbuffer3_srv_index = shader_resource_views.size() - 1;
	}
	//{
	//	addTextureSRVWritable(sizeof(uint32_t), 256, 256);
	//	halton_tex_index = textures_.size() - 1;
	//	halton_tex_srv_index = shader_resource_views.size() - 1;

	//	D3D11_MAPPED_SUBRESOURCE MappedResource;
	//	DXUTGetD3D11DeviceContext()->Map(textures_[halton_tex_index], 0, D3D11_MAP_WRITE_DISCARD, 0, &
	//		MappedResource);
	//	memcpy(MappedResource.pData, createHaltonTexture().data(), sizeof(uint32_t) * 256 * 256);
	//	DXUTGetD3D11DeviceContext()->Unmap(textures_[halton_tex_index], 0);
	//}
}

void RayTracer::run(ID3D11DeviceContext* pd3dImmediateContext)
{
	pd3dImmediateContext_ = pd3dImmediateContext;
	static bool has_built = false;
	if(!has_built)
	{
		buildRadixTree();
		buildBVH();
		has_built = true;
	}
	trace();
	finish();
}

void RayTracer::resize(int width, int height)
{
	this->width = width;
	this->height = height;
	frame_count = 0;
	if(output_tex_index >= 0 && old_tex_index >= 0)
	{
		SAFE_RELEASE(unordered_access_views[output_tex_uav_index]);
		SAFE_RELEASE(shader_resource_views[old_tex_srv_index]);
		SAFE_RELEASE(textures_[output_tex_index]);
		SAFE_RELEASE(textures_[old_tex_index]);
	}
	{
		addTextureUAV(sizeof(uint32_t), width, height);
		output_tex_index = textures_.size() - 1;
		output_tex_uav_index = unordered_access_views.size() - 1;
	}
	{
		addTextureSRV(sizeof(uint32_t), width, height);
		old_tex_index = textures_.size() - 1;
		old_tex_srv_index = shader_resource_views.size() - 1;
	}
	if (gbuffer_srv_index >= 0 && gbuffer_tex_index >= 0)
	{
		SAFE_RELEASE(shader_resource_views[gbuffer_srv_index]);
		SAFE_RELEASE(textures_[gbuffer_tex_index]);
	}
	if (gbuffer2_srv_index >= 0 && gbuffer2_tex_index >= 0)
	{
		SAFE_RELEASE(shader_resource_views[gbuffer2_srv_index]);
		SAFE_RELEASE(textures_[gbuffer2_tex_index]);
	}
	if (gbuffer3_srv_index >= 0 && gbuffer3_tex_index >= 0)
	{
		SAFE_RELEASE(shader_resource_views[gbuffer3_srv_index]);
		SAFE_RELEASE(textures_[gbuffer3_tex_index]);
	}

	if (gbuffer4_srv_index >= 0 && gbuffer4_tex_index >= 0)
	{
		SAFE_RELEASE(shader_resource_views[gbuffer4_srv_index]);
		SAFE_RELEASE(textures_[gbuffer4_tex_index]);
	}

	{
		addTextureSRV(sizeof(uint32_t), width, height);
		gbuffer_tex_index = textures_.size() - 1;
		gbuffer_srv_index = shader_resource_views.size() - 1;
	}
	{
		addTextureSRVFloat4(sizeof(uint32_t), width, height);
		gbuffer2_tex_index = textures_.size() - 1;
		gbuffer2_srv_index = shader_resource_views.size() - 1;
	}
	{
		addTextureSRVFloat4(sizeof(uint32_t), width, height);
		gbuffer3_tex_index = textures_.size() - 1;
		gbuffer3_srv_index = shader_resource_views.size() - 1;
	}

	{
		addTextureSRVFloat4(sizeof(uint32_t), width, height);
		gbuffer4_tex_index = textures_.size() - 1;
		gbuffer4_srv_index = shader_resource_views.size() - 1;
	}
}

void RayTracer::buildRadixTree()
{
	pd3dImmediateContext_->CSSetShader(compute_shaders_[radix_build_shader_index], nullptr, 0);
	pd3dImmediateContext_->CSSetUnorderedAccessViews(0, 1, &unordered_access_views[tree_uav_index], nullptr);
	pd3dImmediateContext_->CSSetConstantBuffers(0, 1, &constant_buffers_[radix_cb_index]);
	pd3dImmediateContext_->Dispatch(primitive_count / 64 + 1, 1, 1);
}

void RayTracer::buildBVH()
{
	pd3dImmediateContext_->CSSetShader(compute_shaders_[bvh_build_shader_index], nullptr, 0);
	pd3dImmediateContext_->CSSetShaderResources(0, 1, &shader_resource_views[primitive_ray_srv_index]);
	pd3dImmediateContext_->CSSetUnorderedAccessViews(0, 1, &unordered_access_views[tree_uav_index], nullptr);
	pd3dImmediateContext_->CSSetUnorderedAccessViews(1, 1, &unordered_access_views[bvh_build_lock_uav_index], nullptr);
	pd3dImmediateContext_->CSSetConstantBuffers(0, 1, &constant_buffers_[radix_cb_index]);
	pd3dImmediateContext_->Dispatch(primitive_count / 64 + 1, 1, 1);
}

void RayTracer::trace()
{
	pd3dImmediateContext_->CSSetShader(compute_shaders_[ray_shader_index], nullptr, 0);
	pd3dImmediateContext_->CSSetShaderResources(0, 1, &shader_resource_views[primitive_render_srv_index]);
	pd3dImmediateContext_->CSSetShaderResources(1, 1, &shader_resource_views[old_tex_srv_index]);
	pd3dImmediateContext_->CSSetShaderResources(2, 1, &shader_resource_views[gbuffer_srv_index]);
	pd3dImmediateContext_->CSSetShaderResources(3, 1, &shader_resource_views[gbuffer2_srv_index]);
	pd3dImmediateContext_->CSSetShaderResources(4, 1, &shader_resource_views[gbuffer3_srv_index]);

	pd3dImmediateContext_->CSSetUnorderedAccessViews(0, 1, &unordered_access_views[tree_uav_index], nullptr);
	pd3dImmediateContext_->CSSetUnorderedAccessViews(1, 1, &unordered_access_views[output_tex_uav_index], nullptr);
	static std::random_device rd;
	static std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);

	D3D11_MAPPED_SUBRESOURCE MappedResource;

#define TILEX 1
#define TILEY 1
#define INT_FRAM 1
#define PATCHZ 1

	for (int w = 0; w < width; w += width / TILEX)
	{
		for (int h = 0; h < height; h += height / TILEY)
		{
			for (int internal_fram = 0; internal_fram < INT_FRAM; ++internal_fram)
			{
				pd3dImmediateContext_->Map(constant_buffers_[rt_cb_index], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
				CB_RT* data = (CB_RT*)MappedResource.pData;
				data->viewportDims.x = width;
				data->viewportDims.y = height;
				data->framecount = frame_count + internal_fram;
				data->tanHalfFovY = 0.1;

				for (int i = 0; i < sizeof(data->random); ++i)
				{
					data->random[i] = dist(rd);
				}
				data->offset[0] = w;
				data->offset[1] = h;
				data->offset[2] = w + width / TILEX - 1;
				data->offset[3] = h + height / TILEY - 1;
				pd3dImmediateContext_->Unmap(constant_buffers_[rt_cb_index], 0);

				pd3dImmediateContext_->CSSetConstantBuffers(0, 1, &constant_buffers_[radix_cb_index]);
				pd3dImmediateContext_->CSSetConstantBuffers(1, 1, &constant_buffers_[rt_cb_index]);

				pd3dImmediateContext_->Dispatch((width / TILEX - 1) / 16 + 1, (height / TILEY - 1) / 16 + 1, PATCHZ);
				pd3dImmediateContext_->CopyResource(textures_[old_tex_index], textures_[output_tex_index]);
			}
		}
	}

	frame_count+= INT_FRAM * PATCHZ;
}

void RayTracer::finish()
{
	pd3dImmediateContext_->CSSetShader(nullptr, nullptr, 0);

	ID3D11UnorderedAccessView* ppUAViewNULL[] = {nullptr, nullptr};
	pd3dImmediateContext_->CSSetUnorderedAccessViews(0, 2, ppUAViewNULL, nullptr);

	ID3D11ShaderResourceView* ppSRVNULL[2] = {nullptr,nullptr};
	pd3dImmediateContext_->CSSetShaderResources(0, 2, ppSRVNULL);

	ID3D11Buffer* ppCBNULL[1] = {nullptr};
	pd3dImmediateContext_->CSSetConstantBuffers(0, 1, ppCBNULL);
}

void RayTracer::release()
{
	BufferManager::release();
	ShaderManager::release();
}

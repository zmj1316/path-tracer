#pragma once
#include "math3d.h"
#include <vector>
#include "tiny_obj_loader.h"

class Scene
{
public:
	Scene();
	~Scene();


	void Scene::loadObj(const char* filename);
	void buildBuffers(ID3D11Device* pDevice);
	void release();
	struct xyz
	{
		vec3 pos;
	};

	struct xyzn
	{
		vec3 pos;
		vec3 normal;
	};

	struct geometry
	{
		std::vector<xyzn> vertices_xyzn;
		std::vector<xyz> vertices_pos;
		std::vector<std::vector<uint32_t>> indicess;
		std::vector<uint32_t> material_ids;

		ID3D11Buffer* vertex_buffer_ = nullptr;
		std::vector<ID3D11Buffer*> index_buffers_;

		~geometry(){
			SAFE_RELEASE(vertex_buffer_);

			for (auto && index_buffer : index_buffers_)
			{
				SAFE_RELEASE(index_buffer);
			}
			index_buffers_.clear();
		}
	};

	std::vector<geometry> geos;
	std::vector<tinyobj::material_t> materials_;

};


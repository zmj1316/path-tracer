#include "DXUT.h"
#include "scene.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <iostream>
#include <unordered_map>

Scene::Scene()
{
}


Scene::~Scene()
{
}

static bool hasSmoothingGroup(const tinyobj::shape_t& shape)
{
	for (size_t i = 0; i < shape.mesh.smoothing_group_ids.size(); i++)
	{
		if (shape.mesh.smoothing_group_ids[i] > 0)
		{
			return true;
		}
	}
	return false;
}

static void CalcNormal(float N[3], float v0[3], float v1[3], float v2[3])
{
	float v10[3];
	v10[0] = v1[0] - v0[0];
	v10[1] = v1[1] - v0[1];
	v10[2] = v1[2] - v0[2];

	float v20[3];
	v20[0] = v2[0] - v0[0];
	v20[1] = v2[1] - v0[1];
	v20[2] = v2[2] - v0[2];

	N[0] = v20[1] * v10[2] - v20[2] * v10[1];
	N[1] = v20[2] * v10[0] - v20[0] * v10[2];
	N[2] = v20[0] * v10[1] - v20[1] * v10[0];

	float len2 = N[0] * N[0] + N[1] * N[1] + N[2] * N[2];
	if (len2 > 0.0f)
	{
		float len = sqrtf(len2);

		N[0] /= len;
		N[1] /= len;
		N[2] /= len;
	}
}

void normalizeVector(vec3& v)
{
	float len2 = v.v[0] * v.v[0] + v.v[1] * v.v[1] + v.v[2] * v.v[2];
	if (len2 > 0.0f)
	{
		float len = sqrtf(len2);

		v.v[0] /= len;
		v.v[1] /= len;
		v.v[2] /= len;
	}
}


static void computeSmoothingNormals(const tinyobj::attrib_t& attrib, const tinyobj::shape_t& shape,
                                    std::map<int, vec3>& smoothVertexNormals)
{
	smoothVertexNormals.clear();
	std::map<int, vec3>::iterator iter;

	for (size_t f = 0; f < shape.mesh.indices.size() / 3; f++)
	{
		// Get the three indexes of the face (all faces are triangular)
		tinyobj::index_t idx0 = shape.mesh.indices[3 * f + 0];
		tinyobj::index_t idx1 = shape.mesh.indices[3 * f + 1];
		tinyobj::index_t idx2 = shape.mesh.indices[3 * f + 2];

		// Get the three vertex indexes and coordinates
		int vi[3]; // indexes
		float v[3][3]; // coordinates

		for (int k = 0; k < 3; k++)
		{
			vi[0] = idx0.vertex_index;
			vi[1] = idx1.vertex_index;
			vi[2] = idx2.vertex_index;
			assert(vi[0] >= 0);
			assert(vi[1] >= 0);
			assert(vi[2] >= 0);

			v[0][k] = attrib.vertices[3 * vi[0] + k];
			v[1][k] = attrib.vertices[3 * vi[1] + k];
			v[2][k] = attrib.vertices[3 * vi[2] + k];
		}

		// Compute the normal of the face
		float normal[3];
		CalcNormal(normal, v[0], v[1], v[2]);

		// Add the normal to the three vertexes
		for (size_t i = 0; i < 3; ++i)
		{
			iter = smoothVertexNormals.find(vi[i]);
			if (iter != smoothVertexNormals.end())
			{
				// add
				iter->second.v[0] += normal[0];
				iter->second.v[1] += normal[1];
				iter->second.v[2] += normal[2];
			}
			else
			{
				smoothVertexNormals[vi[i]].v[0] = normal[0];
				smoothVertexNormals[vi[i]].v[1] = normal[1];
				smoothVertexNormals[vi[i]].v[2] = normal[2];
			}
		}
	} // f

	// Normalize the normals, that is, make them unit vectors
	for (iter = smoothVertexNormals.begin(); iter != smoothVertexNormals.end();
	     ++iter)
	{
		normalizeVector(iter->second);
	}
} // computeSmoothingNormals


void Scene::loadObj(const char* filename)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::map<std::string, int> textures;
	std::string err;
	bool ret = LoadObj(&attrib, &shapes, &materials_, &err, filename);
	assert(ret);
	materials_.push_back(tinyobj::material_t());

	for (size_t s = 0; s < shapes.size(); s++)
	{
		geometry geo;
		int pg_count = 0;
		std::unordered_map<int, int> mat_to_pg;
		for (int i = 0; i < shapes[s].mesh.indices.size() / 3; ++i)
		{
			int current_material_id = shapes[s].mesh.material_ids[i];
			if ((current_material_id < 0) ||
				(current_material_id >= static_cast<int>(materials_.size())))
			{
				// Invaid material ID. Use default material.
				current_material_id =
					materials_.size() -
					1; // Default material is added to the last item in `materials`.
			}
			if (mat_to_pg.find(current_material_id) == mat_to_pg.end())
			{
				mat_to_pg.insert_or_assign(current_material_id, pg_count++);
				geo.material_ids.push_back(current_material_id);
			}
		}
		geo.indicess.resize(pg_count);
		assert(pg_count > 0);

		// Check for smoothing group and compute smoothing normals
		std::map<int, vec3> smoothVertexNormals;
		if (hasSmoothingGroup(shapes[s]))
		{
			std::cout << "Compute smoothingNormal for shape [" << s << "]" << std::endl;
			computeSmoothingNormals(attrib, shapes[s], smoothVertexNormals);
		}

		for (size_t f = 0; f < shapes[s].mesh.indices.size() / 3; f++)
		{
			vec3 pos[3];
			vec3 normal[3];

			tinyobj::index_t idx0 = shapes[s].mesh.indices[3 * f + 0];
			tinyobj::index_t idx1 = shapes[s].mesh.indices[3 * f + 1];
			tinyobj::index_t idx2 = shapes[s].mesh.indices[3 * f + 2];

			int current_material_id = shapes[s].mesh.material_ids[f];

			if ((current_material_id < 0) ||
				(current_material_id >= static_cast<int>(materials_.size())))
			{
				// Invaid material ID. Use default material.
				current_material_id =
					materials_.size() -
					1; // Default material is added to the last item in `materials`.
			}
			for (int k = 0; k < 3; k++)
			{
				int f0 = idx0.vertex_index;
				int f1 = idx1.vertex_index;
				int f2 = idx2.vertex_index;
				assert(f0 >= 0);
				assert(f1 >= 0);
				assert(f2 >= 0);

				pos[0].v[k] = attrib.vertices[3 * f0 + k];
				pos[1].v[k] = attrib.vertices[3 * f1 + k];
				pos[2].v[k] = attrib.vertices[3 * f2 + k];
			}

			{
				bool invalid_normal_index = false;
				if (attrib.normals.size() > 0)
				{
					int nf0 = idx0.normal_index;
					int nf1 = idx1.normal_index;
					int nf2 = idx2.normal_index;

					if ((nf0 < 0) || (nf1 < 0) || (nf2 < 0))
					{
						// normal index is missing from this face.
						invalid_normal_index = true;
					}
					else
					{
						for (int k = 0; k < 3; k++)
						{
							assert(size_t(3 * nf0 + k) < attrib.normals.size());
							assert(size_t(3 * nf1 + k) < attrib.normals.size());
							assert(size_t(3 * nf2 + k) < attrib.normals.size());
							normal[0].v[k] = attrib.normals[3 * nf0 + k];
							normal[1].v[k] = attrib.normals[3 * nf1 + k];
							normal[2].v[k] = attrib.normals[3 * nf2 + k];
						}
					}
				}
				else
				{
					invalid_normal_index = true;
				}

				if (invalid_normal_index && !smoothVertexNormals.empty())
				{
					// Use smoothing normals
					int f0 = idx0.vertex_index;
					int f1 = idx1.vertex_index;
					int f2 = idx2.vertex_index;

					if (f0 >= 0 && f1 >= 0 && f2 >= 0)
					{
						normal[0].v[0] = smoothVertexNormals[f0].v[0];
						normal[0].v[1] = smoothVertexNormals[f0].v[1];
						normal[0].v[2] = smoothVertexNormals[f0].v[2];

						normal[1].v[0] = smoothVertexNormals[f1].v[0];
						normal[1].v[1] = smoothVertexNormals[f1].v[1];
						normal[1].v[2] = smoothVertexNormals[f1].v[2];

						normal[2].v[0] = smoothVertexNormals[f2].v[0];
						normal[2].v[1] = smoothVertexNormals[f2].v[1];
						normal[2].v[2] = smoothVertexNormals[f2].v[2];

						invalid_normal_index = false;
					}
				}

				if (invalid_normal_index)
				{
					// compute geometric normal
					CalcNormal(normal[0].v, pos[0].v, pos[1].v, pos[2].v);
					normal[1] = normal[0];
					normal[2] = normal[0];
				}
			}

			const int pg_index = mat_to_pg[current_material_id];
			for (int i = 0; i < 3; ++i)
			{
				geo.indicess[pg_index].push_back(geo.vertices_pos.size());
				geo.vertices_pos.push_back({pos[i]});
				geo.vertices_xyzn.push_back({pos[i],normal[i]});
			}
			triangle_count++;
		}
		geos.push_back(geo);
	}
}

void Scene::buildBuffers(ID3D11Device* pd3dDevice)
{
	for (auto && geo : geos)
	{
		HRESULT hr;
		D3D11_BUFFER_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(bufferDesc));
		bufferDesc.ByteWidth = sizeof(xyzn) * geo.vertices_xyzn.size();
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = geo.vertices_xyzn.data();
		V(pd3dDevice->CreateBuffer(&bufferDesc, &InitData, &geo.vertex_buffer_));
		DXUT_SetDebugName(geo.vertex_buffer_, "VB");

		for (auto && indices : geo.indicess)
		{
			ID3D11Buffer* buffer;

			bufferDesc.ByteWidth = sizeof(UINT32) * indices.size();
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufferDesc.MiscFlags = 0;

			ZeroMemory(&InitData, sizeof(InitData));

			InitData.pSysMem = indices.data();
			hr = pd3dDevice->CreateBuffer(&bufferDesc, &InitData, &buffer);
			DXUT_SetDebugName(buffer, "IB");
			geo.index_buffers_.push_back(buffer);
		}
	}
}

void Scene::release()
{
	for (auto && geo : geos)
	{
		SAFE_RELEASE(geo.vertex_buffer_);

		for (auto && index_buffer : geo.index_buffers_)
		{
			SAFE_RELEASE(index_buffer);
		}
		geo.index_buffers_.clear();
	}
}

#pragma once
struct Bound
{
	vec3 min;
	float padding0;
	vec3 max;
};

// CS use
struct TreeNode
{
	int left; // primitive index if is leaf
	int right; // == 0 if is leaf
	int parent;
	int index;
	Bound bound;
	int padding;
};

struct xyz
{
	vec3 pos;
};

// CS
struct xyzn
{
	vec3 pos;
	vec3 normal;
};

// CS
struct PrimitiveRender
{
	xyzn vertices[3];
	int matid;
	int pad;
};

struct PrimitiveRay
{
	uint64_t morton_code;
	Bound bound;
	int index;
};

// CS use
struct PrimitiveRayCS
{
	Bound bound;
	int index;
};

// CS use
struct CB_Radix
{
	int node_count;
	vec3 pos;
};

struct CB_RT
{
	D3DXVECTOR2 viewportDims;
	float tanHalfFovY = 0;
	int framecount;
	int offset[16] = {0};
};

constexpr int MORTON_CODE_LENGTH_EACH = 21;

static uint64_t genMortonCode(Bound& small_bound, const Bound& big_bound)
{
	uint64_t morton_code = 0;
	auto max_x = big_bound.max.v[0];
	auto min_x = big_bound.min.v[0];
	auto max_y = big_bound.max.v[1];
	auto min_y = big_bound.min.v[1];
	auto max_z = big_bound.max.v[2];
	auto min_z = big_bound.min.v[2];

	float midx = (small_bound.min.v[0] + small_bound.max.v[0]) / 2;
	float midy = (small_bound.min.v[1] + small_bound.max.v[1]) / 2;
	float midz = (small_bound.min.v[2] + small_bound.max.v[2]) / 2;
	uint64_t a = (uint64_t)(((midx - min_x) / (max_x - min_x)) * MORTON_CODE_LENGTH_EACH);
	uint64_t b = (uint64_t)(((midy - min_y) / (max_y - min_y)) * MORTON_CODE_LENGTH_EACH);
	uint64_t c = (uint64_t)(((midz - min_z) / (max_z - min_z)) * MORTON_CODE_LENGTH_EACH);
	// combine into 63 bits morton code
	for (unsigned int j = 0; j < MORTON_CODE_LENGTH_EACH; j++) {
		morton_code |=
			(((((a >> (MORTON_CODE_LENGTH_EACH - 1 - j))) & 1) << ((MORTON_CODE_LENGTH_EACH - j) * 3 - 1)) |
			((((b >> (MORTON_CODE_LENGTH_EACH - 1 - j))) & 1) << ((MORTON_CODE_LENGTH_EACH - j) * 3 - 2)) |
				((((c >> (MORTON_CODE_LENGTH_EACH - 1 - j))) & 1) << ((MORTON_CODE_LENGTH_EACH - j) * 3 - 3)));
	}
	return morton_code;
}

static void mergeBound(Bound& dst, const Bound& src)
{
	for (int i = 0; i < 3; ++i)
	{
		dst.min.v[i] = min(dst.min.v[i], src.min.v[i]);
		dst.max.v[i] = max(dst.max.v[i], src.max.v[i]);
	}


}
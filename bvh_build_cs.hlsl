#include "structs.hlsli"

// 对应于主机端的constant buffer  
cbuffer RadixCB : register(b0)
{
	int node_count;
};

// 对应于主机端的Shader Resource View  
StructuredBuffer<PrimitiveRayCS> primitive_rays : register(t0);

// 对应于主机端的Unordered Access View  
RWStructuredBuffer<TreeNode> tree_out : register(u0);
RWByteAddressBuffer locks : register(u1);


[numthreads(64, 1, 1)]
void CSMain( uint3 tid : SV_DispatchThreadID )
{
	const int i = tid.x;

	if (i >= node_count)
		return;

	tree_out[node_count + i].index = primitive_rays[i].index;
	tree_out[node_count + i].bound = primitive_rays[i].bound;

	int parent = tree_out[node_count + i].parent;

	int childs_done = 0;

	// parents has to be updated after childs updated
	locks.InterlockedExchange(parent, 1, childs_done);
	while (1) {
		if (childs_done == 0)
			return;

		//merge aabb
		[unroll]
		for (int i = 0; i < 3; ++i) {
			tree_out[parent].bound.min[i] =
				min(tree_out[tree_out[parent].left].bound.min[i],
					tree_out[tree_out[parent].right].bound.min[i]);
			tree_out[parent].bound.max[i] =
				max(tree_out[tree_out[parent].left].bound.max[i],
					tree_out[tree_out[parent].right].bound.max[i]);
		}
		// stop on root
		if (parent == 0)
			return;

		parent = tree_out[parent].parent;
		locks.InterlockedExchange(parent, 1, childs_done);
	}
}
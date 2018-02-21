#include "structs.hlsli"

StructuredBuffer<PrimitiveRender> primitives : register(t0);
RWStructuredBuffer<TreeNode> tree : register(u0);

struct Bound {
	float3 min;
	float padding0;
	float3 max;
};

struct TreeNode {
	int left;   // primitive index if is leaf
	int right;  // == 0 if is leaf
	int parent;
	int index;
	Bound bound;
	int padding;
};

struct PrimitiveRayCS
{
	Bound bound;
	int index;
};

struct xyzn
{
	float3 pos;
	float3 normal;
};

struct PrimitiveRender 
{
	xyzn vertices[3];
	int matid;
	int pad;
};

struct Ray {
	float3 origin;
	float t_min;
	float3 direction;
	float t_max;
};
#include "structs.hlsli"

StructuredBuffer<PrimitiveRender> primitives : register(t0);

RWStructuredBuffer<TreeNode> tree : register(u0);
RWTexture2D<unorm float4> output : register(u1);

cbuffer RTCB : register(b0)
{
	float4x4 invView;
	float4x4 invModel;
	float2 viewportDims;
	float tanHalfFovY;
};


#define STACK_SIZE 50
#define kEpsilon 1e-5

bool intersection_bound(Ray ray, Bound bound) {
	float t_min, t_max, t_xmin, t_xmax, t_ymin, t_ymax, t_zmin, t_zmax;
	float x_a = 1.0 / ray.direction.x, y_a = 1.0 / ray.direction.y, z_a = 1.0 / ray.direction.z;
	float  x_e = ray.origin.x, y_e = ray.origin.y, z_e = ray.origin.z;

	// calculate t interval in x-axis
	if (x_a >= 0) {
		t_xmin = (bound.min.x - x_e) * x_a;
		t_xmax = (bound.max.x - x_e) * x_a;
	}
	else {
		t_xmin = (bound.max.x - x_e) * x_a;
		t_xmax = (bound.min.x - x_e) * x_a;
	}

	// calculate t interval in y-axis
	if (y_a >= 0) {
		t_ymin = (bound.min.y - y_e) * y_a;
		t_ymax = (bound.max.y - y_e) * y_a;
	}
	else {
		t_ymin = (bound.max.y - y_e) * y_a;
		t_ymax = (bound.min.y - y_e) * y_a;
	}

	// calculate t interval in z-axis
	if (z_a >= 0) {
		t_zmin = (bound.min.z - z_e) * z_a;
		t_zmax = (bound.max.z - z_e) * z_a;
	}
	else {
		t_zmin = (bound.max.z - z_e) * z_a;
		t_zmax = (bound.min.z - z_e) * z_a;
	}

	// find if there an intersection among three t intervals
	t_min = max(t_xmin, max(t_ymin, t_zmin));
	t_max = min(t_xmax, min(t_ymax, t_zmax));

	return (t_min <= t_max);

}

bool calc_barycentrics(int index, Ray ray, out float t, out float3 barycentrics) {
	float u, v;

	// compute plane's normal
	float3 v0 = primitives[index].vertices[0].pos;
	float3 v1 = primitives[index].vertices[1].pos;
	float3 v2 = primitives[index].vertices[2].pos;
	float3 v0v1 = v1 - v0;
	float3 v0v2 = v2 - v0;
	// no need to normalize
	float3 N = cross(v0v1, v0v2); // N 
	float denom = dot(N, N);

	// Step 1: finding P

	// check if ray and plane are parallel ?
	float NdotRayDirection = dot(N, ray.direction);
	if (abs(NdotRayDirection) < kEpsilon) // almost 0 
		return false; // they are parallel so they don't intersect ! 

					  // compute d parameter using equation 2
	float d = dot(N, v0);

	// compute t (equation 3)
	t = (dot(N, ray.origin) + d) / NdotRayDirection;
	// check if the triangle is in behind the ray
	if (t < 0) return false; // the triangle is behind 

							 // compute the intersection point using equation 1
	float3 P = ray.origin + t * ray.direction;

	// Step 2: inside-outside test
	float3 C; // vector perpendicular to triangle's plane 

			  // edge 0
	float3 edge0 = v1 - v0;
	float3 vp0 = P - v0;
	C = cross(edge0, vp0);
	if (dot(N, C) < 0) return false; // P is on the right side 

									 // edge 1
	float3 edge1 = v2 - v1;
	float3 vp1 = P - v1;
	C = cross(edge1, vp1);
	if ((u = dot(N, C)) < 0)  return false; // P is on the right side 

											// edge 2
	float3 edge2 = v0 - v2;
	float3 vp2 = P - v2;
	C = cross(edge2, vp2);
	if ((v = dot(N, C)) < 0) return false; // P is on the right side; 

	u /= denom;
	v /= denom;

	barycentrics = float3(u, v, 1 - (u + v));
	return true; // this ray hits the triangle 
}

bool intersect(in Ray ray, out int primitive_index, out float3 barycentrics, out float current_t) {
	int stack[STACK_SIZE];
	int stack_top = STACK_SIZE;
	stack[--stack_top] = 0;

	bool intersected = false;

	current_t = 1000;

	[allow_uav_condition]
	while (stack_top != STACK_SIZE) {
		int current = stack[stack_top++];
		if (intersection_bound(ray, tree[current].bound)) {
			if (tree[current].left = 0) {// leaf
				float t;
				if (calc_barycentrics(tree[current].index, ray, t, barycentrics)) {
					if (current_t > t) {
						current_t = t;
						primitive_index = current;
						intersected = true;
					}
				}
			}
			else {
				stack[--stack_top] = tree[current].right;
				stack[--stack_top] = tree[current].left;

				if (stack_top < 0) {
					return false;
				}
			}
		}
	}
	return intersected;
}


float3 tracing(Ray ray) {
	int primitive_index;
	float3 barycentrics;
	float t;
	if (intersect(ray, primitive_index, barycentrics, t)) {
		return float3(1, 0, 1);
	}
	return float3(0.1, 0.2, 0.3);
}



[numthreads(1, 1, 1)]
void CSMain( uint3 launchIndex : SV_DispatchThreadID )
{
	float2 d = ((launchIndex.xy / viewportDims) * 2.f - 1.f);
	float aspectRatio = viewportDims.x / viewportDims.y;

	Ray ray;
	float4x4 transInvView = invView;

	ray.origin = transInvView[3].xyz;
	ray.direction = normalize((d.x * transInvView[0].xyz * tanHalfFovY * aspectRatio) - (d.y * transInvView[1].xyz * tanHalfFovY) + transInvView[2].xyz);

	ray.t_min = 0;
	ray.t_max = 100000;
	output[launchIndex.xy] = float4(tracing(ray), 1);
}
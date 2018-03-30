#include "structs.hlsli"
#include "helpers.hlsli"

// The below texture contains faure permutations, stored in an integer texture:

StructuredBuffer<PrimitiveRender> primitives : register(t0);
Texture2D<float4> old_texture : register(t1);

RWStructuredBuffer<TreeNode> tree : register(u0);
RWTexture2D<float4> output : register(u1);
RWTexture2D<unorm float4> output_u : register(u2);

Texture2D<float4> diffuseTex : register(t2);
Texture2D<float4> normalTex : register(t3);
Texture2D<float4> worldTex : register(t4);

struct Material {
	float ke;
	float ns;
	float tr;
	float ni;
	float4 kd;
	float4 ks;
};

// 对应于主机端的constant buffer  
cbuffer RadixCB : register(b0)
{
	int node_count;
	float3 g_pos;
	Material mats[16];
};
cbuffer RTCB : register(b1)
{
	float2 viewportDims;
	float tanHalfFovY;
	uint framecount;
	int4 g_tid_offset;
	int4 g_random;
};


#define STACK_SIZE 15
#define kEpsilon 1e-4

bool intersection_bound(Ray ray, Bound bound ,float current_t ,out float tout) {
	float t_min, t_max, t_xmin, t_xmax, t_ymin, t_ymax, t_zmin, t_zmax;
	float x_a = 1.0 / ray.direction.x, y_a = 1.0 / ray.direction.y, z_a = 1.0 / ray.direction.z;
	float x_e = ray.origin.x, y_e = ray.origin.y, z_e = ray.origin.z;

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
	tout = t_min;
	return (t_min <= t_max && t_min <= current_t);
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

bool calc_barycentrics2(int index, Ray ray, out float t, out float3 barycentrics) {
	float3 P, T, Q;
	float3 A = primitives[index].vertices[0].pos;
	float3 B = primitives[index].vertices[1].pos;
	float3 C = primitives[index].vertices[2].pos;
	float3 E1 = B - A;
	float3 E2 = C - A;
	P = cross(ray.direction, E2);
	float det = 1.0f / dot(E1, P);
	T = ray.origin - A;
	barycentrics.x = dot(T, P) * det;
	Q = cross(T, E1);
	barycentrics.y = dot(ray.direction, Q)*det;
	t = dot(E2, Q)*det;
	barycentrics.z = (1 - barycentrics.x - barycentrics.y);
	if (barycentrics.x >= -kEpsilon && barycentrics.y >= -kEpsilon && barycentrics.z >= -kEpsilon)
		return true;
	return false;
}

bool calc_barycentrics3(int index, Ray ray, out float t, out float3 barycentrics) {
	float3 a = primitives[index].vertices[0].pos;
	float3 b = primitives[index].vertices[1].pos;
	float3 c = primitives[index].vertices[2].pos;
	float3 e_1 = b - a, e_2 = c - a;
	float3 n = cross(e_1, e_2);
	float3 q = cross(ray.direction, e_2);
	float a1 = dot(e_1, q);
	if (/*(dot(n, ray.direction)) <= 0 || */abs(a1) <= kEpsilon) {
		t = -1;
		return false;
	}

	float3 s = (ray.origin - a) / a1;
	float3 r = cross(s, e_1);
	t = dot(e_2,r);
	barycentrics.x = dot(s,q);
	barycentrics.y = dot(r,ray.direction);
	barycentrics.z = 1 - barycentrics.x - barycentrics.y;
	if (barycentrics.x >= -kEpsilon && barycentrics.y >= -kEpsilon && barycentrics.z >= -kEpsilon)
		return t >= kEpsilon;
	return false;
}

bool intersect(in Ray ray, out int primitive_index, out float3 barycentrics, out float current_t) {
	//{
	//	if (ray.direction.x == 0) {
	//		ray.direction.x += kEpsilon;
	//		ray.direction = normalize(ray.direction);
	//	}
	//	if (ray.direction.y == 0) {
	//		ray.direction.y += kEpsilon;
	//		ray.direction = normalize(ray.direction);

	//	}
	//	if (ray.direction.z == 0) {
	//		ray.direction.z += kEpsilon;
	//		ray.direction = normalize(ray.direction);

	//	}
	//}
	int stack[STACK_SIZE];
	int stack_top = STACK_SIZE;
	stack[--stack_top] = 0;

	bool intersected = false;
	current_t = ray.t_max;

	[allow_uav_condition]
	while (stack_top != STACK_SIZE) {
		int current = stack[stack_top++];
		float t;
		if (intersection_bound(ray, tree[current].bound, current_t, t)) {
			if (current >= node_count) {// leaf
				float3 tmp;
				if (calc_barycentrics3(tree[current].index, ray, t, tmp)) {
					if (current_t > t && t > ray.t_min) {
						barycentrics = tmp;
						current_t = t;
						primitive_index = tree[current].index;
						intersected = true;
					}
				}
				
			}
			else {
				stack[--stack_top] = tree[current].right;
				stack[--stack_top] = tree[current].left;
				if (stack_top < 0) {
					//barycentrics = float3(1, 1, 1);
					return intersected;
				}
			}
		}
	}
	return intersected;
}

#ifndef MAX_ITR
#define MAX_ITR 5
#endif

bool getBounce(float3 view_dir, float3 normal, int id, inout float3 multi, out float3 direction,inout uint seed) {
	direction = float3(0, 0, 0);
	Material mat = mats[id];
	float rand_mat = rand_next(seed);
	float base = mat.tr + mat.ks[0] + mat.kd[0];
	if (mat.ke > 0) { // emit
		multi = multi * float3(mat.ke, mat.ke, mat.ke);
		return false;
	}
	else {
		if (mat.tr > 0) {
			float cos = dot(normal, view_dir);

			float nc = 1.f;
			float nt = mat.ni;
			float nnt = nc / nt;
			if (cos > 0)
				nnt = nt / nc;
			float ddn = abs(cos);
			float cos2t = 1.f - nnt * nnt * (1.f - ddn * ddn);

			if (cos2t < 0.0f) {
				direction = reflect(view_dir, normal);
				return true;
			}

			if (cos < 0) {
				direction = normalize(nnt * view_dir - (cos * nnt + sqrt(cos2t)) * normal);
			}
			else {
				direction = normalize(nnt * view_dir + (-cos * nnt + sqrt(cos2t)) * normal);
			}

			float a = nt - nc;
			float b = nt + nc;
			float R0 = a * a / (b * b);
			float c = 1 - ddn;

			float Re = R0 + (1 - R0) * c * c * c * c * c;
			float Tr = 1 - Re;
			float P = .25f + .5f * Re;
			float RP = Re / P;
			float TP = Tr / (1.f - P);

			if (rand_next(seed) < P) { /* R.R. */
				multi *= RP;
				direction = reflect(view_dir, normal);
				return true;
			}
			else
			{
				multi *= TP;
				return true;
			}
		}
		else if ((mat.ns > 1&& rand_mat < mat.ks[0] / base) /*|| mat.tr > 0*/) {
			direction = reflect(view_dir, normal);
			float r1 = 2 * MY_PI * ((framecount & 1) / 2.0 + rand_next(seed) / 2);
			float height = pow((((framecount >> 1) & 3) / 4.0 + rand_next(seed) / 4), 1 / (mat.ns));
			float r2s = sqrt(1 - height * height);
			float3 w = direction;
			float3 u = cross(w,view_dir);
			//if (abs(w.x) > 0.1) {
			//	u = cross(w, float3(0, 1, 0));
			//}
			//else {
			//	u = cross(w, float3(1, 0, 0));
			//}
			u = normalize(u);
			float3 v = (cross(w, u)); // 乘以一个系数来假装有各向异性
			direction = normalize(u*cos(r1)*r2s + v * sin(r1)*r2s + w * height);
			multi *= mat.ks;
		}
		else {
			multi *= mat.kd;

			float r1 = 2 * MY_PI * (((framecount >>1 ) & 3) / 4.0 + rand_next(seed) / 4.0);
			float r2 = (((framecount >> 3) & 3) / 4.0 + rand_next(seed) / 4.0);

			//float r1 = 2 * MY_PI * (rand_next(seed));
			//float r2 = rand_next(seed);

			float r2s = sqrt(r2);
			float3 w = normal;
			float3 u;
			if (abs(w.x) > 0.1) {
				u = cross(w, float3(0, 1, 0));
			}
			else {
				u = cross(w, float3(1, 0, 0));
			}
			u = normalize(u);
			float3 v = (cross(w, u));
			direction = normalize(u*cos(r1)*r2s + v * sin(r1)*r2s + w * sqrt(1 - r2));
		}
	}
	return true;
}

float3 second_tracing(uint2 launchIndex, uint seed) {
	float4 diffuse = diffuseTex[launchIndex.xy];
	float4 normal = normalTex[launchIndex.xy];
	float4 hitPos = worldTex[launchIndex.xy];
	int itr = 1;
	int primitive_index;
	float3 barycentrics;
	float t;
	Ray this_ray;
	this_ray.t_min = kEpsilon;
	this_ray.t_max = 10000;
	float3 multi = diffuse.xyz;
	int matid = asint(normal.w);
	if (matid == 0)
		return float3(0, 0, 0);
	matid -= 1;
	if (mats[matid].ns > 1)
		multi = float3(1, 1, 1);// spec
	float3 direction;
	if (!getBounce(normalize(hitPos.xyz - g_pos), normalize(normal.xyz), matid, multi, direction, seed))
		return multi;
	this_ray.origin = hitPos.xyz;
	this_ray.direction = direction;
	[allow_uav_condition]
	while (intersect(this_ray, primitive_index, barycentrics, t)) {
		this_ray.origin = this_ray.origin + t * this_ray.direction;
		itr++;
		matid = primitives[primitive_index].matid;
		if (itr > MAX_ITR) {
			float p = max(max(multi.x, multi.y), multi.z);
			if (p > rand_next(seed)) {
				multi = multi * (1 / p);
			}
			else {
				return float3(0, 0, 0);
			}
		}
		if (itr >= MAX_ITR * 3) {
			return float3(0, 0, 0);
		}
		float3 A = primitives[primitive_index].vertices[0].normal;
		float3 B = primitives[primitive_index].vertices[1].normal;
		float3 C = primitives[primitive_index].vertices[2].normal;
		float3 new_normal = A * barycentrics.z + B * barycentrics.x + C * barycentrics.y;
		if (!getBounce(this_ray.direction, new_normal, matid, multi, direction, seed))
			return multi;
		this_ray.direction = direction;
	}
	return float3(0,0,0);
}

[numthreads(16, 16, 1)]
void CSMain( uint3 launchIndex : SV_DispatchThreadID )
{
	launchIndex.x += g_tid_offset[0];
	launchIndex.y += g_tid_offset[1];

	if (launchIndex.x > g_tid_offset[2] || launchIndex.y > g_tid_offset[3])
		return;
	float2 d = ((launchIndex.xy / viewportDims) * 2.f - 1.f);
	float aspectRatio = viewportDims.x / viewportDims.y;

	uint seed = rand_init(uint(launchIndex.x + launchIndex.y * viewportDims.x), g_random[0]);

	float4 this_color = float4(second_tracing(launchIndex.xy, seed/*, hState*/),1);
	int2 sampleid = launchIndex.xy;

	float old_factor = 1.0f * framecount / (framecount + 1);
	float new_factor = 1.0f / (framecount + 1);
	float4 acced_color = pow(pow(old_texture[sampleid], 2.2) * old_factor + this_color * new_factor, 1 / 2.2);
	output[sampleid.xy] = acced_color;
	output_u[sampleid.xy] = acced_color;
}
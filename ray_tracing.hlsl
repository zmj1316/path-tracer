#include "structs.hlsli"
#include "helpers.hlsli"

StructuredBuffer<PrimitiveRender> primitives : register(t0);
Texture2D<unorm float4> old_texture : register(t1);

RWStructuredBuffer<TreeNode> tree : register(u0);
RWTexture2D<unorm float4> output : register(u1);

// 对应于主机端的constant buffer  
cbuffer RadixCB : register(b0)
{
	int node_count;
	float3 g_pos;
};
cbuffer RTCB : register(b1)
{
	float2 viewportDims;
	float tanHalfFovY;
	uint framecount;
	int4 g_tid_offset;
};


#define STACK_SIZE 4000
#define kEpsilon 1e-4

bool intersection_bound(Ray ray, Bound bound ,out float tout) {
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
	tout = t_min;
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
	barycentrics.x = dot(s,q);
	barycentrics.y = dot(r,ray.direction);
	barycentrics.z = 1 - barycentrics.x - barycentrics.y;
	t = dot(e_2,r);
	if (barycentrics.x >= -kEpsilon && barycentrics.y >= -kEpsilon && barycentrics.z >= -kEpsilon)
		return t >= kEpsilon;
	return false;


	//float d00 = Dot(v0, v0);
	//float d01 = Dot(v0, v1);
	//float d11 = Dot(v1, v1);
	//float d20 = Dot(v2, v0);
	//float d21 = Dot(v2, v1);
	//float denom = d00 * d11 - d01 * d01;
	//barycentrics.x = (d11 * d20 - d01 * d21) / denom;
	//barycentrics.y = (d00 * d21 - d01 * d20) / denom;
	//barycentrics.z = 1.0f - v - w;
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
		float t;
		if (intersection_bound(ray, tree[current].bound, t)) {
			if (current >= node_count) {// leaf
				float3 tmp;
				if (calc_barycentrics3(tree[current].index, ray, t, tmp)) {
					if (current_t > t) {
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
					barycentrics = float3(1, 1, 1);
					return true;
				}
			}
		}
	}
	return intersected;
}

#define MAX_ITR 5
float3 tracing(Ray ray, int2 rand2) {
	int primitive_index;
	float3 barycentrics;
	float t;
	uint seed = rand_init(rand2.x + framecount,rand2.y + g_tid_offset[rand2.x&3]);
	//uint seed = framecount;
	Ray this_ray = ray;
	float3 color = float3(0, 0, 0);
	int itr = 0;
	float3 multi = float3(1,1,1);

	[allow_uav_condition]
	while (intersect(this_ray, primitive_index, barycentrics, t)) {
		int id = primitives[primitive_index].matid;

		if (itr >= MAX_ITR) {
			break;
			//if (rand_next(seed) < p) {
			//	multi = multi * (1 / p);
			//}
			//else {
			//	break;
			//}
		}
		itr++;

		if ( id == 1) {
			color += multi * float3(20, 20, 20);
			break;
		}
		else{
			float3 A = primitives[primitive_index].vertices[0].normal;
			float3 B = primitives[primitive_index].vertices[1].normal;
			float3 C = primitives[primitive_index].vertices[2].normal;
			float3 normal = A * barycentrics.z + B * barycentrics.x + C * barycentrics.y;

			float3 hitPoint = this_ray.origin + t * this_ray.direction;

			this_ray.origin = hitPoint;
			if (id == 0) {
				if (itr > 2) continue;
				float cos = dot(normal, this_ray.direction);

				float nc = 1.f;
				float nt = 1.5f;
				float nnt = nc / nt;
				if (cos > 0)
					nnt = nt / nc;
				float ddn = abs(cos);
				float cos2t = 1.f - nnt * nnt * (1.f - ddn * ddn);

				if (cos2t < 0.0f) {
					this_ray.direction = reflect(this_ray.direction, -normal);
					continue;
				}

				if (cos < 0) {
					this_ray.direction = normalize(nnt * this_ray.direction - (cos * nnt + sqrt(cos2t)) * normal);
				}
				else {
					this_ray.direction = normalize(nnt * this_ray.direction + (-cos * nnt + sqrt(cos2t)) * normal);
				}

					 
				float a = nt - nc;
				float b = nt + nc;
				float R0 = a * a / (b * b);
				float c = 1 - ddn;

				float Re = R0 + (1 - R0) * c * c * c * c*c;
				float Tr = 1.f - Re;
				float P = .25f + .5f * Re;
				float RP = Re / P;
				float TP = Tr / (1.f - P);

				if (rand_next(seed) < P) { /* R.R. */
					multi *= RP;
					this_ray.direction = reflect(this_ray.direction, normal);
					continue;
				}
				else {
					multi *= TP;
					continue;
				}
			}
			else if (id == 6) {
				this_ray.direction = reflect(this_ray.direction, normal);
			}
			else {
				if (id == 4)
					multi *= float3(0.1, 0.1, 0.9);
				else if (id == 3)
					multi *= float3(0.9, 0.1, 0.1);

				//float r1 = 2 * MY_PI * ((1.0f * (framecount & 0x1F) / 0x1F) + rand_next(seed) / 0x20);
				float r1 = 2 * MY_PI * ( rand_next(seed));
				float r2 = rand_next(seed);
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
				this_ray.direction = normalize(u*cos(r1)*r2s + v * sin(r1)*r2s + w * sqrt(1 - r2));
			}
		}
	}
	return color;
}

//color = primitives[primitive_index].vertices[0].pos * 0.1;
//// gamma correction
//color.x = pow(color.x, 0.45);
//color.y = pow(color.y, 0.45);
//color.z = pow(color.z, 0.45);
//return color;

//uint pack(float3 rgb) {
//	return (int(0xFF * rgb.x) << 16) | int(0xFF * rgb.y) << 8 | int(0xFF * rgb.z);
//}

[numthreads(8, 8, 1)]
void CSMain( uint3 launchIndex : SV_DispatchThreadID )
{
	//launchIndex.x += g_tid_offset[0];
	//launchIndex.y += g_tid_offset[1];
	float2 d = ((launchIndex.xy / viewportDims) * 2.f - 1.f);
	float aspectRatio = viewportDims.x / viewportDims.y;

	uint seed = rand_init(g_tid_offset[0], g_tid_offset[1]);

	float r1 = rand_next(seed);
	float r2 = rand_next(seed);
	r1 = (r1*r1 - 0.5) / viewportDims.x / 2;
	r2 = (r2*r2 - 0.5) / viewportDims.y / 2;
	Ray ray;
	ray.origin = g_pos;
	float3 pixel = g_pos + float3(0, 0, -1);
	pixel.x += -d.x * aspectRatio * 0.4 + r1;
	pixel.y += -d.y * 0.4 + r2;
	//ray.direction = normalize((d.x * transInvView[0].xyz * 0.3 * aspectRatio) + (d.y * transInvView[1].xyz * 0.3) + transInvView[2].xyz);
	ray.direction = normalize(pixel - ray.origin);
	ray.t_min = 0;
	ray.t_max = 100000;
	float4 this_color = float4(tracing(ray, launchIndex.xy),1);
	//if (g_tid_offset[2] < 1000)
	//	output[launchIndex.xy] = this_color;
	//else {
	//	output[launchIndex.xy] = old_texture[launchIndex.xy];
	//}
	//output[launchIndex.xy] = this_color;
	int2 sampleid = launchIndex.xy;

	//float r3 = rand_next(seed);
	//if (r3 < 0.025)
	//	sampleid.x--;
	//else if (r3 > 0.975)
	//	sampleid.x++;

	//float r4 = rand_next(seed);
	//if (r4 < 0.025)
	//	sampleid.y--;
	//else if (r4 > 0.975)
	//	sampleid.y++;
	//this_color = pow(this_color, 1.2);
	output[launchIndex.xy] = (old_texture[sampleid/* + float2((rand_next(seed)) / 2, (rand_next(seed)) / 2)*/] * framecount + this_color) / (framecount + 1);
	//output[launchIndex.xy] = float4(tanHalfFovY,0,0,1);
}
#pragma once

struct vec3
{
	float v[3];
};

struct vec4
{
	float v[4];
};

template<typename T>
auto my_max(T x, T y, T z)
{
	return max(x,max(y, z));
}

template<typename T>
auto my_min(T x, T y, T z)
{
	return min(x, min(y, z));
}

static auto bound_max(vec3 x, vec3 y , vec3 z)
{
	vec3 max;
	for (int i = 0; i < 3; ++i)
	{
		max.v[i] = my_max(x.v[i], y.v[i], z.v[i]);
	}
	return max;
}

static auto bound_min(vec3 x, vec3 y, vec3 z)
{
	vec3 min;
	for (int i = 0; i < 3; ++i)
	{
		min.v[i] = my_min(x.v[i], y.v[i], z.v[i]);
	}
	return min;
}


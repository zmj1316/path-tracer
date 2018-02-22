//--------------------------------------------------------------------------------------
// File: BasicHLSL11_VS.hlsl
//
// The vertex shader file for the BasicHLSL11 sample.  
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------
cbuffer cbPerFrame : register(b0)
{
	matrix		g_mWorldViewProjection	: packoffset(c0);
	matrix		g_mWorld				: packoffset(c4);
	float3		g_vLightDir				: packoffset(c8);
};

cbuffer cbPerObject : register(b1)
{
	float4		params				: packoffset(c0);
};

//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
	float4 vPosition	: POSITION;
	float3 vNormal		: NORMAL;
};

struct VS_OUTPUT
{
	float4 vPosition	: SV_POSITION;
	float3 vNormal		: NORMAL;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VSMain(VS_INPUT Input)
{
	VS_OUTPUT Output;

	Output.vPosition = mul(Input.vPosition, g_mWorldViewProjection);
	Output.vNormal = Input.vNormal;
	return Output;
}

float4 PSMain(VS_OUTPUT Input) : SV_TARGET
{
	//float light = Input.vNormal * g_vLightDir;
	float light = Input.vNormal * g_vLightDir;
	float3 diffuse = params.xyz;
	return float4(Input.vNormal, 1);
}

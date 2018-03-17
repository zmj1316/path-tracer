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
	matrix		g_mWorldViewProjection;
	matrix		g_mWorld;
};

cbuffer cbPerObject : register(b1)
{
	float3		color;
	int		matid;
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
	float4 vNormal		: NORMAL;
	float3 vWORLD		: POSITION;
};

struct PS_OUTPUT {
	float4 color;
	float4 normal;
	float4 position;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VSMain(VS_INPUT Input)
{
	VS_OUTPUT Output;

	Output.vPosition = mul(Input.vPosition, g_mWorldViewProjection);
	Output.vNormal = mul(Input.vNormal, g_mWorld);
	Output.vWORLD = mul(Input.vPosition, g_mWorld);
	return Output;
}

PS_OUTPUT PSMain(VS_OUTPUT Input) : SV_TARGET
{
	PS_OUTPUT output;
	output.color = float4(color,1);
	output.normal.xyz = normalize(Input.vNormal.xyz);
	output.normal.w = asfloat(matid + 1);
	output.position = float4(Input.vWORLD,1);
	return output;
}

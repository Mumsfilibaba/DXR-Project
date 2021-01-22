#include "PBRCommon.hlsli"

RWTexture2D<float> Texture : register(u0, space0);

cbuffer Params : register(b0, space0)
{
	int2 ScreenSize;
};

#define THREAD_COUNT 16

groupshared float gTextureCache[THREAD_COUNT][THREAD_COUNT];

static const int2 MAX_SIZE = int2(THREAD_COUNT - 1, THREAD_COUNT - 1);

[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void Main(ComputeShaderInput Input)
{
	const int2 TexCoords = int2(Input.DispatchThreadID.xy);
	if (TexCoords.x > ScreenSize.x || TexCoords.y > ScreenSize.y)
	{
		return;
	}
	
	// Cache texture fetches
	const int2 GroupThreadID = int2(Input.GroupThreadID.xy);
	gTextureCache[GroupThreadID.x][GroupThreadID.y] = Texture[TexCoords];
	
	GroupMemoryBarrierWithGroupSync();
	
	// Perform blur
	float Result = 0.0f;
	for (int x = -2; x < 2; x++)
	{
		for (int y = -2; y < 2; y++)
		{
			int2 Offset				= int2(x, y);
			int2 CurrentTexCoord	= max(int2(0, 0), min(MAX_SIZE, GroupThreadID + Offset));
			Result += gTextureCache[CurrentTexCoord.x][CurrentTexCoord.y];
		}
	}
	
	const float NumSamples = 16.0f;
	Texture[TexCoords] = Result / NumSamples;
}
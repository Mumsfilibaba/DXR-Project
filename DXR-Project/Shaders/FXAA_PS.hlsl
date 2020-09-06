#include "PBRCommon.hlsli"

#define FXAA_EDGE_THRESHOLD			(1.0f / 16.0f)
#define FXAA_EDGE_THRESHOLD_MIN		(1.0f / 32.0f)

#define FXAA_SUBPIX_TRIM			(1.0f / 8.0f)
//#define FXAA_SUBPIX_TRIM			0.0f
#define FXAA_SUBPIX_CAP				(7.0f / 8.0f)
//#define FXAA_SUBPIX_CAP			1.0f
#define FXAA_SUBPIX_TRIM_SCALE		(1.0f / (1.0f - FXAA_SUBPIX_TRIM))

#define FXAA_SEARCH_THRESHOLD		(1.0f / 8.0f)
#define FXAA_SEARCH_STEPS			32

cbuffer CB0 : register(b0, space0)
{
	float2 TextureSize;
}

Texture2D FinalImage : register(t0, space0);
SamplerState Sampler : register(s0, space0);

static const float FXAA_REDUCE_MUL = 1.0f / 8.0f;

/*
* Helpers
*/
float3 SampleFXAA(in Texture2D Texture, in SamplerState InSampler, float2 TexCoord)
{
	return ApplyGammaCorrectionAndTonemapping(Texture.SampleLevel(InSampler, TexCoord, 0).rgb);
}

float FXAALuma(float3 Color)
{
	return (Color.y * (0.587f / 0.299f)) + Color.x;
}

float3 Lerp(float3 A, float3 B, float AmountOfA)
{
	return (ToFloat3(-AmountOfA) * B) + ((A * ToFloat3(AmountOfA)) + B);
}

/*
* PixelShader
*/
float4 Main(float2 TexCoord : TEXCOORD0) : SV_TARGET
{
	// Correct the texcoords
	TexCoord.y = 1.0f - TexCoord.y;
	
	// Perform edge detection
	const float2 InvTextureSize = float2(1.0f, 1.0f) / TextureSize;
	const float3 Middle	= SampleFXAA(FinalImage, Sampler, TexCoord);
	const float3 North	= SampleFXAA(FinalImage, Sampler, TexCoord + float2(0.0f, -InvTextureSize.y));
	const float3 South	= SampleFXAA(FinalImage, Sampler, TexCoord + float2(0.0f, InvTextureSize.y));
	const float3 West	= SampleFXAA(FinalImage, Sampler, TexCoord + float2(-InvTextureSize.x, 0.0f));
	const float3 East	= SampleFXAA(FinalImage, Sampler, TexCoord + float2( InvTextureSize.x, 0.0f));
	float LumaM	= FXAALuma(Middle);
	float LumaN	= FXAALuma(North);
	float LumaS	= FXAALuma(South);
	float LumaW	= FXAALuma(West);
	float LumaE	= FXAALuma(East);
	
	float RangeMin = min(LumaM, min(min(LumaN, LumaS), min(LumaW, LumaE)));
	float RangeMax = max(LumaM, max(max(LumaN, LumaS), max(LumaW, LumaE)));
	float Range = RangeMax - RangeMin;
	if (Range < max(FXAA_EDGE_THRESHOLD_MIN, RangeMax * FXAA_EDGE_THRESHOLD))
	{
		return float4(Middle, 1.0f);
	}
	
	float LumaL = (LumaN + LumaS + LumaW + LumaE) * 0.25f;
	float RangeL = abs(LumaL - LumaM);
	float BlendL = max(0.0f, (RangeL / Range) - FXAA_SUBPIX_TRIM) * FXAA_SUBPIX_TRIM_SCALE;
	BlendL = min(BlendL, FXAA_SUBPIX_CAP);
	
	float3 ColorL = (Middle + North + South + West + East);
	const float3 NorthWest = SampleFXAA(FinalImage, Sampler, TexCoord + float2(-InvTextureSize.x, -InvTextureSize.y));
	const float3 SouthWest = SampleFXAA(FinalImage, Sampler, TexCoord + float2(-InvTextureSize.x,  InvTextureSize.y));
	const float3 NorthEast = SampleFXAA(FinalImage, Sampler, TexCoord + float2( InvTextureSize.x, -InvTextureSize.y));
	const float3 SouthEast = SampleFXAA(FinalImage, Sampler, TexCoord + float2( InvTextureSize.x,  InvTextureSize.y));
 #if 1
	ColorL += (NorthWest + SouthWest + NorthEast + SouthEast);
	ColorL = ColorL * ToFloat3(1.0f / 9.0f);
#endif
	
	float LumaNW = FXAALuma(NorthWest);
	float LumaNE = FXAALuma(NorthEast);
	float LumaSW = FXAALuma(SouthWest);
	float LumaSE = FXAALuma(SouthEast);
	float EdgeVert =
		abs((0.25 * LumaNW) + (-0.5 * LumaN) + (0.25 * LumaNE)) +
		abs((0.50 * LumaW)	+ (-1.0 * LumaM) + (0.50 * LumaE))	+
		abs((0.25 * LumaSW) + (-0.5 * LumaS) + (0.25 * LumaSE));
	float EdgeHorz =
		abs((0.25 * LumaNW) + (-0.5 * LumaW) + (0.25 * LumaSW)) +
		abs((0.50 * LumaN)	+ (-1.0 * LumaM) + (0.50 * LumaS))	+
		abs((0.25 * LumaNE) + (-0.5 * LumaE) + (0.25 * LumaSE));
	
	bool HorzSpan = EdgeHorz >= EdgeVert;
	if (!HorzSpan)
	{
		LumaN = LumaW;
	}
	
	if (!HorzSpan)
	{
		LumaS = LumaE;
	}
	
	float LengthSign = HorzSpan ? -InvTextureSize.y : -InvTextureSize.x;
	float GradientN = abs(LumaN - LumaM);
	float GradientS = abs(LumaS - LumaM);
	LumaN = (LumaN + LumaM) * 0.5f;
	LumaS = (LumaS + LumaM) * 0.5f;
	
	bool PairN = GradientN >= GradientS;
	if (!PairN)
	{
		LumaN = LumaS;
	}
	if (!PairN)
	{
		GradientN = GradientS;
	}
	if (!PairN)
	{
		LengthSign *= -1.0;
	}
	
	float2 PosN;
	PosN.x = TexCoord.x + (HorzSpan ? 0.0f : (LengthSign * 0.5f));
	PosN.y = TexCoord.y + (HorzSpan ? (LengthSign * 0.5f) : 0.0f);
	
	GradientN = GradientN * FXAA_SEARCH_THRESHOLD;
	
	float2 PosP = PosN;
	float2 OffNP = HorzSpan ? float2(InvTextureSize.x, 0.0f) : float2(0.0f, InvTextureSize.y);
	float LumaEndN = LumaN;
	float LumaEndP = LumaN;
	bool DoneN = false;
	bool DoneP = false;
	
	PosN += OffNP * float2(-2.0f, -2.0f);
	PosP += OffNP * float2( 2.0f,  2.0f);
	OffNP *= float2(3.0f, 3.0f);
	
	for (int i = 0; i < FXAA_SEARCH_STEPS; i++)
	{
		if (!DoneN)
		{
			LumaEndN = FXAALuma(SampleFXAA(FinalImage, Sampler, PosN.xy).rgb);
		}
		if(!DoneP)
		{
			LumaEndP = FXAALuma(SampleFXAA(FinalImage, Sampler, PosP.xy).rgb);
		}
		
		DoneN = DoneN || (abs(LumaEndN - LumaN) >= GradientN);
		DoneP = DoneP || (abs(LumaEndP - LumaN) >= GradientN);
		if (DoneN && DoneP)
		{
			break;
		}
		if (!DoneN)
		{
			PosN -= OffNP;
		}
		if (!DoneP)
		{
			PosP += OffNP;
		}
	}
	
	float DstN = HorzSpan ? (TexCoord.x - PosN.x) : (TexCoord.y - PosN.y);
	float DstP = HorzSpan ? (PosP.x - TexCoord.x) : (PosP.y - TexCoord.y);
	
	bool DirectionN = DstN < DstP;
	LumaEndN = DirectionN ? LumaEndN : LumaEndP;
	if (((LumaM - LumaN) < 0.0) == ((LumaEndN - LumaN) < 0.0))
	{
		LengthSign = 0.0;
	}
	
	float SpanLength = (DstP + DstN);
	DstN = DirectionN ? DstN : DstP;
	
	float SubPixelOffset = (0.5f + (DstN * (-1.0f / SpanLength))) * LengthSign;
	float2 TexCoordOffset = float2((HorzSpan ? 0.0f : SubPixelOffset), (HorzSpan ? SubPixelOffset : 0.0f));
	float3 ColorF = SampleFXAA(FinalImage, Sampler, TexCoord + TexCoordOffset).rgb;
	return float4(Lerp(ColorL, ColorF, BlendL), 1.0f);
}
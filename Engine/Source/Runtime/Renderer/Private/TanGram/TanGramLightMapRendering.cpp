//TanGram
#include "TanGramLightMapRendering.h"
#include "ScenePrivate.h"
#include "PrecomputedVolumetricLightmap.h"

IMPLEMENT_TYPE_LAYOUT(FTanGramUniformLightMapPolicyShaderParametersType);
//IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FIndirectLightingCacheUniformParameters, "IndirectLightingCache");

void TanGramSetupLCIUniformBuffers(const FPrimitiveSceneProxy* PrimitiveSceneProxy, const FLightCacheInterface* LCI, FRHIUniformBuffer*& PrecomputedLightingBuffer, FRHIUniformBuffer*& LightmapResourceClusterBuffer, FRHIUniformBuffer*& IndirectLightingCacheBuffer)
{
	if (LCI)
	{
		PrecomputedLightingBuffer = LCI->GetPrecomputedLightingBuffer();
	}

	if (LCI && LCI->GetResourceCluster())
	{
		check(LCI->GetResourceCluster()->UniformBuffer);
		LightmapResourceClusterBuffer = LCI->GetResourceCluster()->UniformBuffer;
	}

	if (!PrecomputedLightingBuffer)
	{
		PrecomputedLightingBuffer = GEmptyPrecomputedLightingUniformBuffer.GetUniformBufferRHI();
	}

	if (!LightmapResourceClusterBuffer)
	{
		LightmapResourceClusterBuffer = GDefaultLightmapResourceClusterUniformBuffer.GetUniformBufferRHI();
	}

	if (PrimitiveSceneProxy && PrimitiveSceneProxy->GetPrimitiveSceneInfo())
	{
		IndirectLightingCacheBuffer = PrimitiveSceneProxy->GetPrimitiveSceneInfo()->IndirectLightingCacheUniformBuffer;
	}

	if (!IndirectLightingCacheBuffer)
	{
		IndirectLightingCacheBuffer = GEmptyIndirectLightingCacheUniformBuffer.GetUniformBufferRHI();
	}
}

void FTanGramUniformLightMapPolicy::GetVertexShaderBindings(const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	const ElementDataType& ShaderElementData, const VertexParametersType* VertexShaderParameters,
	FMeshDrawSingleShaderBindings& ShaderBindings)
{
	FRHIUniformBuffer* PrecomputedLightingBuffer = nullptr;
	FRHIUniformBuffer* LightmapResourceClusterBuffer = nullptr;
	FRHIUniformBuffer* IndirectLightingCacheBuffer = nullptr;

	TanGramSetupLCIUniformBuffers(PrimitiveSceneProxy, ShaderElementData, PrecomputedLightingBuffer, LightmapResourceClusterBuffer, IndirectLightingCacheBuffer);

	ShaderBindings.Add(VertexShaderParameters->PrecomputedLightingBufferParameter, PrecomputedLightingBuffer);
	ShaderBindings.Add(VertexShaderParameters->IndirectLightingCacheParameter, IndirectLightingCacheBuffer);
	ShaderBindings.Add(VertexShaderParameters->LightmapResourceCluster, LightmapResourceClusterBuffer);
}

void FTanGramUniformLightMapPolicy::GetPixelShaderBindings(
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	const ElementDataType& ShaderElementData,
	const PixelParametersType* PixelShaderParameters,
	FMeshDrawSingleShaderBindings& ShaderBindings)
{
	FRHIUniformBuffer* PrecomputedLightingBuffer = nullptr;
	FRHIUniformBuffer* LightmapResourceClusterBuffer = nullptr;
	FRHIUniformBuffer* IndirectLightingCacheBuffer = nullptr;

	TanGramSetupLCIUniformBuffers(PrimitiveSceneProxy, ShaderElementData, PrecomputedLightingBuffer, LightmapResourceClusterBuffer, IndirectLightingCacheBuffer);

	ShaderBindings.Add(PixelShaderParameters->PrecomputedLightingBufferParameter, PrecomputedLightingBuffer);
	ShaderBindings.Add(PixelShaderParameters->IndirectLightingCacheParameter, IndirectLightingCacheBuffer);
	ShaderBindings.Add(PixelShaderParameters->LightmapResourceCluster, LightmapResourceClusterBuffer);
}
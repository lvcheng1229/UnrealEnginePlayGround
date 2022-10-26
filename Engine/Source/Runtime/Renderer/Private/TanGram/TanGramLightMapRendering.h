#pragma once
#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "RenderResource.h"
#include "UniformBuffer.h"
#include "ShaderParameters.h"
#include "ShadowRendering.h"
#include "LightMap.h"

struct FTanGramNoLightMapPolicy
{
	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters) { return true; }
	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {}
	static bool RequiresSkylight() { return false; }
};

enum ETanGramLightMapPolicyType
{
	TG_LMP_NO_LIGHTMAP,
};

//Bind ILC , LightMap and so on
class RENDERER_API FTanGramUniformLightMapPolicyShaderParametersType
{
	DECLARE_TYPE_LAYOUT(FTanGramUniformLightMapPolicyShaderParametersType, NonVirtual);
public:
	void Bind(const FShaderParameterMap& ParameterMap)
	{
		PrecomputedLightingBufferParameter.Bind(ParameterMap, TEXT("PrecomputedLightingBuffer"));
		IndirectLightingCacheParameter.Bind(ParameterMap, TEXT("IndirectLightingCache"));
		LightmapResourceCluster.Bind(ParameterMap, TEXT("LightmapResourceCluster"));
	}

	void Serialize(FArchive& Ar)
	{
		Ar << PrecomputedLightingBufferParameter;
		Ar << IndirectLightingCacheParameter;
		Ar << LightmapResourceCluster;
	}

	LAYOUT_FIELD(FShaderUniformBufferParameter, PrecomputedLightingBufferParameter);
	LAYOUT_FIELD(FShaderUniformBufferParameter, IndirectLightingCacheParameter);
	LAYOUT_FIELD(FShaderUniformBufferParameter, LightmapResourceCluster);
};

class RENDERER_API FTanGramUniformLightMapPolicy
{
public:
	using ElementDataType = const FLightCacheInterface*;
	using VertexParametersType = FTanGramUniformLightMapPolicyShaderParametersType;
	using PixelParametersType = FTanGramUniformLightMapPolicyShaderParametersType;

	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		return false; // This one does not compile shaders since we can't tell which policy to use.
	}
	
	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{}

	FTanGramUniformLightMapPolicy(ETanGramLightMapPolicyType InIndirectPolicy) : IndirectPolicy(InIndirectPolicy) {}
	
	static void GetVertexShaderBindings(
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	const ElementDataType& ShaderElementData,
	const VertexParametersType* VertexShaderParameters,
	FMeshDrawSingleShaderBindings& ShaderBindings);

	static void GetPixelShaderBindings(
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const ElementDataType& ShaderElementData,
		const PixelParametersType* PixelShaderParameters,
		FMeshDrawSingleShaderBindings& ShaderBindings);
	
	friend bool operator==(const FTanGramUniformLightMapPolicy A,const FTanGramUniformLightMapPolicy B)
	{
		return A.IndirectPolicy == B.IndirectPolicy;
	}

	ETanGramLightMapPolicyType GetIndirectPolicy() const { return IndirectPolicy; }

private:

	ETanGramLightMapPolicyType IndirectPolicy;
};



template <ETanGramLightMapPolicyType Policy>
class TTanGramUniformLightMapPolicy :public FTanGramUniformLightMapPolicy
{
public:
	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		switch (Policy)
		{
		case ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP:
			return FTanGramNoLightMapPolicy::ShouldCompilePermutation(Parameters);

		default:
			check(false);
			return false;
		}
	}

	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		switch (Policy)
		{
		case ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP:
			FTanGramNoLightMapPolicy::ModifyCompilationEnvironment(Parameters, OutEnvironment);
			return;
		default:
			check(false);
		}
	}

	static bool RequiresSkylight()
	{
		return false;
	}
};

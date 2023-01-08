#pragma once

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "RHI.h"
#include "ShaderParameters.h"
#include "SceneView.h"
#include "Shader.h"
#include "MaterialShared.h"
#include "GlobalShader.h"
#include "MaterialShaderType.h"
#include "SceneRenderTargetParameters.h"
#include "ShaderParameterUtils.h"
#include "ShaderParameters.h"
#include "VertexFactory.h"
#include "MeshMaterialShaderType.h"
#include "MaterialShader.h"
#include "MeshDrawShaderBindings.h"

struct FTanGramShaderPermutationParameters : public FShaderPermutationParameters
{
	FMaterialShaderParameters MaterialParameters;

	FTanGramShaderPermutationParameters(EShaderPlatform InPlatform, const FMaterialShaderParameters& InMaterialParameters, int32 InPermutationId, EShaderPermutationFlags InFlags)
		: FShaderPermutationParameters(InPlatform, InPermutationId, InFlags)
		, MaterialParameters(InMaterialParameters)
	{}
};



class RENDERER_API FTanGramShader : public FShader
{
	DECLARE_TYPE_LAYOUT(FTanGramShader, NonVirtual);
public:
	using FPermutationParameters = FTanGramShaderPermutationParameters;
	
	FTanGramShader() = default;
	FTanGramShader(const FMaterialShaderType::CompiledShaderInitializerType& Initializer);

	void GetElementShaderBindings(
	const FShaderMapPointerTable& PointerTable,
	const FScene* Scene, 
	const FSceneView* ViewIfDynamicMeshCommand, 
	const FVertexFactory* VertexFactory,
	const EVertexInputStreamType InputStreamType,
	const FStaticFeatureLevel FeatureLevel,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	const FMeshBatch& MeshBatch,
	const FMeshBatchElement& BatchElement, 
	FMeshDrawSingleShaderBindings& ShaderBindings,
	FVertexInputStreamArray& VertexStreams) const;

	template<typename ShaderType, typename PointerTableType>
	static inline void GetElementShaderBindings(
	const TShaderRefBase<ShaderType, PointerTableType>& Shader,
	const FScene* Scene,
	const FSceneView* ViewIfDynamicMeshCommand,
	const FVertexFactory* VertexFactory,
	const EVertexInputStreamType InputStreamType,
	ERHIFeatureLevel::Type FeatureLevel,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	const FMeshBatch& MeshBatch,
	const FMeshBatchElement& BatchElement,
	FMeshDrawSingleShaderBindings& ShaderBindings,
	FVertexInputStreamArray& VertexStreams)
	{
		Shader->GetElementShaderBindings(
			Shader.GetPointerTable(), Scene, ViewIfDynamicMeshCommand, VertexFactory,
			InputStreamType, FeatureLevel, PrimitiveSceneProxy, MeshBatch, BatchElement, ShaderBindings, VertexStreams);
	}
	
	void GetShaderBindings(
	const FScene* Scene,
	ERHIFeatureLevel::Type FeatureLevel,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	const FMaterialRenderProxy& MaterialRenderProxy,
	const FMaterial& Material,
	const FMeshPassProcessorRenderState& DrawRenderState,
	FMeshDrawSingleShaderBindings& ShaderBindings) const;

private:
	LAYOUT_FIELD(FShaderUniformBufferParameter, MaterialUniformBuffer);
};


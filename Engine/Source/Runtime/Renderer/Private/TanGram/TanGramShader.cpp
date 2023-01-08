#include "TanGramShader.h"
#include "PostProcess/SceneRenderTargets.h"
#include "RendererModule.h"
#include "ScenePrivate.h"
#include "ParameterCollection.h"
#include "VT/VirtualTextureTest.h"
#include "VT/VirtualTextureSpace.h"
#include "VT/VirtualTextureSystem.h"

//!!!!!!!!!!!!!
//!!!!!!!!!!!!!
//!!!!!!!!!!!!!
//!!!!!!!!!!!!!
IMPLEMENT_TYPE_LAYOUT(FTanGramShader);

FTanGramShader::FTanGramShader(const FMaterialShaderType::CompiledShaderInitializerType& Initializer):FShader(Initializer)
{
	
}

void FTanGramShader::GetElementShaderBindings(const FShaderMapPointerTable& PointerTable, const FScene* Scene,
	const FSceneView* ViewIfDynamicMeshCommand, const FVertexFactory* VertexFactory,
	const EVertexInputStreamType InputStreamType, const FStaticFeatureLevel FeatureLevel,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy, const FMeshBatch& MeshBatch, const FMeshBatchElement& BatchElement,
	FMeshDrawSingleShaderBindings& ShaderBindings, FVertexInputStreamArray& VertexStreams) const
{
	//const FVertexFactoryType* VertexFactoryType = GetVertexFactoryType(PointerTable);
	//if (VertexFactoryType)
	//{
	//	const FVertexFactoryShaderParameters* VFParameters = VertexFactoryParameters.Get();
	//	if (VFParameters)
	//	{
	//		VertexFactoryType->GetShaderParameterElementShaderBindings(GetFrequency(), VFParameters, Scene, ViewIfDynamicMeshCommand, this, InputStreamType, FeatureLevel, VertexFactory, BatchElement, ShaderBindings, VertexStreams);
	//	}
	//}
		
	if (UseGPUScene(GMaxRHIShaderPlatform, FeatureLevel) && VertexFactory->GetPrimitiveIdStreamIndex(FeatureLevel, InputStreamType) >= 0 
		&& !(FeatureLevel == ERHIFeatureLevel::ES3_1 && GetFrequency() == SF_Vertex)) // Allow Primitive UB for VS on mobile
	{
		const FShaderType* ShaderType = GetType(PointerTable);
		ensureMsgf(!GetUniformBufferParameter<FPrimitiveUniformShaderParameters>().IsBound(), TEXT("Shader %s attempted to bind the Primitive uniform buffer even though Vertex Factory computes a PrimitiveId per-instance.  This will break auto-instancing.  Shaders should use GetPrimitiveData(PrimitiveId).Member instead of Primitive.Member."), ShaderType->GetName());
		ensureMsgf(!BatchElement.PrimitiveUniformBuffer || (FeatureLevel == ERHIFeatureLevel::ES3_1), TEXT("FMeshBatchElement was assigned a PrimitiveUniformBuffer even though Vertex Factory %s fetches primitive shader data through a Scene buffer.  The assigned PrimitiveUniformBuffer cannot be respected.  Use PrimitiveUniformBufferResource instead for dynamic primitive data."), ShaderType->GetName());
	}
	else
	{
		if (BatchElement.PrimitiveUniformBuffer)
		{
			ShaderBindings.Add(GetUniformBufferParameter<FPrimitiveUniformShaderParameters>(), BatchElement.PrimitiveUniformBuffer);
		}
		else
		{
			const FShaderType* ShaderType = GetType(PointerTable);
			checkf(BatchElement.PrimitiveUniformBufferResource, TEXT("%s expected a primitive uniform buffer but none was set on BatchElement.PrimitiveUniformBuffer or BatchElement.PrimitiveUniformBufferResource"), ShaderType->GetName());
			ShaderBindings.Add(GetUniformBufferParameter<FPrimitiveUniformShaderParameters>(), BatchElement.PrimitiveUniformBufferResource->GetUniformBufferRHI());
		}
	}
}

void FTanGramShader::GetShaderBindings(const FScene* Scene, ERHIFeatureLevel::Type FeatureLevel,
                                       const FPrimitiveSceneProxy* PrimitiveSceneProxy, const FMaterialRenderProxy& MaterialRenderProxy,
                                       const FMaterial& Material, const FMeshPassProcessorRenderState& DrawRenderState,
                                       FMeshDrawSingleShaderBindings& ShaderBindings) const
{
	// part 1 : FMaterialShader::GetShaderBindings
	check(Material.GetRenderingThreadShaderMap() && Material.GetRenderingThreadShaderMap()->IsValidForRendering() && Material.GetFeatureLevel() == FeatureLevel);

	const FUniformExpressionCache& UniformExpressionCache = MaterialRenderProxy.UniformExpressionCache[FeatureLevel];

	checkf(UniformExpressionCache.CachedUniformExpressionShaderMap == Material.GetRenderingThreadShaderMap(),TEXT("tangram fail"));
	checkf(UniformExpressionCache.UniformBuffer, TEXT("tangram fail"));

#if !(UE_BUILD_TEST || UE_BUILD_SHIPPING || !WITH_EDITOR)
//	VerifyExpressionAndShaderMaps(&MaterialRenderProxy, Material, &UniformExpressionCache);
#endif

	ShaderBindings.Add(MaterialUniformBuffer, UniformExpressionCache.UniformBuffer);

	{
		const TArray<FGuid>& ParameterCollections = UniformExpressionCache.ParameterCollections;
		ensure(ParameterCollections.Num() == 0);
	}
	
	// part 2 : FMeshMaterialShader::GetShaderBindings
	
	if (DrawRenderState.GetViewUniformBuffer())
	{
		ShaderBindings.Add(GetUniformBufferParameter<FViewUniformShaderParameters>(), DrawRenderState.GetViewUniformBuffer());
	}
}

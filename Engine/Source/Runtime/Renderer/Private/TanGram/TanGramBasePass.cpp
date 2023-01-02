#include "TanGramBasePassRendering.h"
//#include "ScenePrivate.h"

#include "TranslucentRendering.h"
#include "DynamicPrimitiveDrawing.h"
#include "ScenePrivate.h"
#include "ShaderPlatformQualitySettings.h"
#include "MaterialShaderQualitySettings.h"
#include "PrimitiveSceneInfo.h"
#include "MeshPassProcessor.inl"
#include "TanGramMeshPassProcessor.inl"
#include "Engine/TextureCube.h"

const FLightSceneInfo* TanGram::GetDirectionalLightInfo(const FScene* Scene, const FPrimitiveSceneProxy* PrimitiveSceneProxy)
{
	const FLightSceneInfo* DirectionalLight = nullptr;
	if (PrimitiveSceneProxy && Scene)
	{
		const int32 LightChannel = GetFirstLightingChannelFromMask(PrimitiveSceneProxy->GetLightingChannelMask());
		DirectionalLight = LightChannel >= 0 ? Scene->MobileDirectionalLights[LightChannel] : nullptr;
	}
	return DirectionalLight;
}

bool TanGram::GetShaders(int32 NumMovablePointLights, const FMaterial& Material, FVertexFactoryType* VertexFactoryType,
	TShaderRef<TTanGramBasePassVS>& VertexShader,
	TShaderRef<TTanGramBasePassPS>& PixelShader)
{
	FMaterialShaderTypes ShaderTypes;
	ensure(IsMobileHDR());
	ShaderTypes.AddShaderType<TTanGramBasePassVS>();
	ShaderTypes.AddShaderType<TTanGramBasePassPS>();

	FMaterialShaders Shaders;
	if (!Material.TryGetShaders(ShaderTypes, VertexFactoryType, Shaders))
	{
		return false;
	}

	Shaders.TryGetVertexShader(VertexShader);
	Shaders.TryGetPixelShader(PixelShader);
	return true;
}

FTanGramBasePassProcessor::FTanGramBasePassProcessor(
	const FScene* Scene,
	ERHIFeatureLevel::Type InFeatureLevel,
	const FSceneView* InViewIfDynamicMeshCommand,
	const FMeshPassProcessorRenderState& InDrawRenderState,
	FMeshPassDrawListContext* InDrawListContext)
	: FMeshPassProcessor(Scene, InFeatureLevel, InViewIfDynamicMeshCommand, InDrawListContext)
	, PassDrawRenderState(InDrawRenderState) 
{

}

void FTanGramBasePassProcessor::AddMeshBatch(const FMeshBatch& RESTRICT MeshBatch, uint64 BatchElementMask, const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy, int32 StaticMeshId)
{
	if (!MeshBatch.bUseForMaterial || (PrimitiveSceneProxy && !PrimitiveSceneProxy->ShouldRenderInMainPass()))
	{
		return;
	}

	const FMaterialRenderProxy* MaterialRenderProxy = MeshBatch.MaterialRenderProxy;
	while (MaterialRenderProxy)
	{
		const FMaterial* Material = MaterialRenderProxy->GetMaterialNoFallback(FeatureLevel);
		if (Material && Material->GetRenderingThreadShaderMap())
		{
			if (TryAddMeshBatch(MeshBatch, BatchElementMask, PrimitiveSceneProxy, StaticMeshId, *MaterialRenderProxy, *Material))
			{
				break;
			}
		}

		MaterialRenderProxy = MaterialRenderProxy->GetFallback(FeatureLevel);
	}
}

bool FTanGramBasePassProcessor::TryAddMeshBatch(const FMeshBatch& RESTRICT MeshBatch, uint64 BatchElementMask, const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy, int32 StaticMeshId, const FMaterialRenderProxy& MaterialRenderProxy, const FMaterial& Material)
{
	const EBlendMode BlendMode = Material.GetBlendMode();
	const FMaterialShadingModelField ShadingModels = Material.GetShadingModels();
	const FLightSceneInfo* MobileDirectionalLight = TanGram::GetDirectionalLightInfo(Scene, PrimitiveSceneProxy);

	return Process(MeshBatch, BatchElementMask, StaticMeshId, PrimitiveSceneProxy, MaterialRenderProxy, Material, BlendMode, ShadingModels, MeshBatch.LCI);
}

bool FTanGramBasePassProcessor::Process(const FMeshBatch& RESTRICT MeshBatch, uint64 BatchElementMask, int32 StaticMeshId,
	const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy, const FMaterialRenderProxy& RESTRICT MaterialRenderProxy,
	const FMaterial& RESTRICT MaterialResource, EBlendMode BlendMode, FMaterialShadingModelField ShadingModels, const FUniformLightMapPolicy::ElementDataType& RESTRICT LightMapElementData)
{
	TMeshProcessorShaders<TTanGramBasePassVS, TTanGramBasePassPS> BasePassShaders;
	
	if (Scene && Scene->SkyLight)
	{
		UE_LOG(LogTemp, Error, TEXT("FTanGramBasePassProcessor::Process"));
	}

	int32 	NumMovablePointLights = 0;
	{
		const bool bIsUnlit = MaterialResource.GetShadingModels().IsUnlit();
		NumMovablePointLights = (PrimitiveSceneProxy && !bIsUnlit) ? PrimitiveSceneProxy->GetPrimitiveSceneInfo()->NumMobileMovablePointLights : 0;
		if (NumMovablePointLights > 0)
		{
			NumMovablePointLights = INT32_MAX;
		}
	}

	if (!TanGram::GetShaders(
		NumMovablePointLights,
		MaterialResource,
		MeshBatch.VertexFactory->GetType(),
		BasePassShaders.VertexShader,
		BasePassShaders.PixelShader))
	{
		return false;
	}

	FMeshPassProcessorRenderState DrawRenderState(PassDrawRenderState);
	
	FMeshDrawCommandSortKey SortKey;
	{
		bool bBackground = PrimitiveSceneProxy ? PrimitiveSceneProxy->TreatAsBackgroundForOcclusion() : false;
		SortKey.PackedData = (BlendMode == EBlendMode::BLEND_Masked ? 1 : 0);
		SortKey.PackedData |= (bBackground ? 2 : 0); // background flag in second bit
	}

	const FMeshDrawingPolicyOverrideSettings OverrideSettings = ComputeMeshOverrideSettings(MeshBatch);
	ERasterizerFillMode MeshFillMode = ComputeMeshFillMode(MeshBatch, MaterialResource, OverrideSettings);
	ERasterizerCullMode MeshCullMode = ComputeMeshCullMode(MeshBatch, MaterialResource, OverrideSettings);

	TTanGramBasePassShaderElementData<FTanGramUniformLightMapPolicy> ShaderElementData(LightMapElementData, false);
	ShaderElementData.InitializeMeshMaterialData(ViewIfDynamicMeshCommand, PrimitiveSceneProxy, MeshBatch, StaticMeshId, false);

	BuildTanGramMeshDrawCommands(
		MeshBatch,
		BatchElementMask,
		PrimitiveSceneProxy,
		MaterialRenderProxy,
		MaterialResource,
		DrawRenderState,
		BasePassShaders,
		MeshFillMode,
		MeshCullMode,
		SortKey,
		EMeshPassFeatures::Default,
		ShaderElementData);
	return true;
}

FMeshPassProcessor* CreateTanGramBasePassProcessor(const FScene* Scene, const FSceneView* InViewIfDynamicMeshCommand, FMeshPassDrawListContext* InDrawListContext)
{
	FMeshPassProcessorRenderState PassDrawRenderState;
	PassDrawRenderState.SetBlendState(TStaticBlendStateWriteMask<CW_RGBA>::GetRHI());
	PassDrawRenderState.SetDepthStencilAccess(Scene->DefaultBasePassDepthStencilAccess);
	PassDrawRenderState.SetDepthStencilState(TStaticDepthStencilState<true, CF_DepthNearOrEqual>::GetRHI());
	
	return new(FMemStack::Get()) FTanGramBasePassProcessor(Scene, Scene->GetFeatureLevel(), InViewIfDynamicMeshCommand, PassDrawRenderState, InDrawListContext);
}

FRegisterPassProcessorCreateFunction RegisterTanGramBasePass(&CreateTanGramBasePassProcessor, EShadingPath::Mobile, EMeshPass::BasePass, EMeshPassFlags::CachedMeshCommands | EMeshPassFlags::MainView);


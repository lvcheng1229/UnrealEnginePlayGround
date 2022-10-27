#include "TanGramBasePassRendering.h"
//#include "ScenePrivate.h"

#include "TranslucentRendering.h"
#include "DynamicPrimitiveDrawing.h"
#include "ScenePrivate.h"
#include "ShaderPlatformQualitySettings.h"
#include "MaterialShaderQualitySettings.h"
#include "PrimitiveSceneInfo.h"
#include "MeshPassProcessor.inl"
#include "Engine/TextureCube.h"


//Temp
static bool UseSkyReflectionCapture(const FScene* RenderScene)
{
	return RenderScene
		&& RenderScene->SkyLight
		&& RenderScene->SkyLight->ProcessedTexture
		&& RenderScene->SkyLight->ProcessedTexture->TextureRHI;
}

template<>
void TTanGramBasePassPSPolicyParamType<FTanGramUniformLightMapPolicy>::GetShaderBindings(
	const FScene* Scene,
	ERHIFeatureLevel::Type FeatureLevel,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	const FMaterialRenderProxy& MaterialRenderProxy,
	const FMaterial& Material,
	const FMeshPassProcessorRenderState& DrawRenderState,
	const TTanGramBasePassShaderElementData<FTanGramUniformLightMapPolicy>& ShaderElementData,
	FMeshDrawSingleShaderBindings& ShaderBindings) const
{
	FMeshMaterialShader::GetShaderBindings(Scene, FeatureLevel, PrimitiveSceneProxy, MaterialRenderProxy, Material, DrawRenderState, ShaderElementData, ShaderBindings);

	FTanGramUniformLightMapPolicy::GetPixelShaderBindings(
	PrimitiveSceneProxy,
	ShaderElementData.LightMapPolicyElementData,
	this,
	ShaderBindings);
	
	if (TanGramDirectionLightBufferParam.IsBound() && Scene)
	{
		const int32 UniformBufferIndex = PrimitiveSceneProxy ? GetFirstLightingChannelFromMask(PrimitiveSceneProxy->GetLightingChannelMask()) + 1 : 0;
		ShaderBindings.Add(TanGramDirectionLightBufferParam, Scene->UniformBuffers.MobileDirectionalLightUniformBuffers[UniformBufferIndex]);
	}


	if (Scene)
	{
		// test for HQ reflection parameter existence
		//if (TanGramHQReflectionCubemaps[0].IsBound() || TanGramHQReflectionCubemaps[1].IsBound() || TanGramHQReflectionCubemaps[2].IsBound())
		//{
		//	static const int32 MaxNumReflections = FPrimitiveSceneInfo::MaxCachedReflectionCaptureProxies;
		//	static_assert(MaxNumReflections == 3, "Update reflection array initializations to match MaxCachedReflectionCaptureProxies");
		//	// set reflection parameters
		//	FTexture* ReflectionCubemapTextures[MaxNumReflections] = { GBlackTextureCube, GBlackTextureCube, GBlackTextureCube };
		//	FVector4f CapturePositions[MaxNumReflections] = { FVector4f(0, 0, 0, 0), FVector4f(0, 0, 0, 0), FVector4f(0, 0, 0, 0) };
		//	FVector4f CaptureTilePositions[MaxNumReflections] = { FVector4f(0, 0, 0, 0), FVector4f(0, 0, 0, 0), FVector4f(0, 0, 0, 0) };
		//	FVector4f ReflectionParams(0.0f, 0.0f, 0.0f, 0.0f);
		//	FVector4f ReflectanceMaxValueRGBMParams(0.0f, 0.0f, 0.0f, 0.0f);
		//	FMatrix44f CaptureBoxTransformArray[MaxNumReflections] = { FMatrix44f(EForceInit::ForceInitToZero), FMatrix44f(EForceInit::ForceInitToZero), FMatrix44f(EForceInit::ForceInitToZero) };
		//	FVector4f CaptureBoxScalesArray[MaxNumReflections] = { FVector4f(EForceInit::ForceInitToZero), FVector4f(EForceInit::ForceInitToZero), FVector4f(EForceInit::ForceInitToZero) };
		//	FPrimitiveSceneInfo* PrimitiveSceneInfo = PrimitiveSceneProxy ? PrimitiveSceneProxy->GetPrimitiveSceneInfo() : nullptr;
		//	if (PrimitiveSceneInfo)
		//	{
		//		for (int32 i = 0; i < MaxNumReflections; i++)
		//		{
		//			const FReflectionCaptureProxy* ReflectionProxy = PrimitiveSceneInfo->CachedReflectionCaptureProxies[i];
		//			if (ReflectionProxy)
		//			{
		//				CapturePositions[i] = ReflectionProxy->RelativePosition;
		//				CapturePositions[i].W = ReflectionProxy->InfluenceRadius;
		//				CaptureTilePositions[i] = FVector4f(ReflectionProxy->TilePosition, 0);
		//				if (ReflectionProxy->EncodedHDRCubemap)
		//				{
		//					ReflectionCubemapTextures[i] = ReflectionProxy->EncodedHDRCubemap->GetResource();
		//				}
		//				//To keep ImageBasedReflectionLighting coherence with PC, use AverageBrightness instead of InvAverageBrightness to calculate the IBL contribution
		//				ReflectionParams[i] = ReflectionProxy->EncodedHDRAverageBrightness;
		//
		//				ReflectanceMaxValueRGBMParams[i] = ReflectionProxy->MaxValueRGBM;
		//				if (ReflectionProxy->Shape == EReflectionCaptureShape::Box)
		//				{
		//					CaptureBoxTransformArray[i] = ReflectionProxy->BoxTransform;
		//					CaptureBoxScalesArray[i] = FVector4f(ReflectionProxy->BoxScales, ReflectionProxy->BoxTransitionDistance);
		//				}
		//			}
		//			else if (Scene->SkyLight != nullptr && Scene->SkyLight->ProcessedTexture != nullptr)
		//			{
		//				// NegativeInfluence to signal the shader we are defaulting to SkyLight if there are no ReflectionComponents in the Level
		//				CapturePositions[i].W = -1.0f;
		//				ReflectionCubemapTextures[i] = Scene->SkyLight->ProcessedTexture;
		//				ReflectionParams[3] = FMath::FloorLog2(Scene->SkyLight->ProcessedTexture->GetSizeX());
		//				break;
		//			}
		//		}
		//	}
		//
		//	for (int32 i = 0; i < MaxNumReflections; i++)
		//	{
		//		ShaderBindings.AddTexture(TanGramHQReflectionCubemaps[i], TanGramHQReflectionSamplers[i], ReflectionCubemapTextures[i]->SamplerStateRHI, ReflectionCubemapTextures[i]->TextureRHI);
		//	}
		//	ShaderBindings.Add(TanGramHQReflectionInvAverageBrigtnessParams, ReflectionParams);
		//	ShaderBindings.Add(TanGramHQReflectanceMaxValueRGBMParams, ReflectanceMaxValueRGBMParams);
		//	ShaderBindings.Add(TanGramHQReflectionPositionsAndRadii, CapturePositions);
		//	ShaderBindings.Add(TanGramHQReflectionTilePositions, CaptureTilePositions);
		//	ShaderBindings.Add(TanGramHQReflectionCaptureBoxTransformArray, CaptureBoxTransformArray);
		//	ShaderBindings.Add(TanGramHQReflectionCaptureBoxScalesArray, CaptureBoxScalesArray);
		//}
		//else if (TanGramReflectionParameter.IsBound())
		//if (TanGramReflectionParameter.IsBound())
		{
			FRHIUniformBuffer* ReflectionUB = GDefaultMobileReflectionCaptureUniformBuffer.GetUniformBufferRHI();
			FPrimitiveSceneInfo* PrimitiveSceneInfo = PrimitiveSceneProxy ? PrimitiveSceneProxy->GetPrimitiveSceneInfo() : nullptr;
			if (PrimitiveSceneInfo && PrimitiveSceneInfo->CachedReflectionCaptureProxy)
			{
				ReflectionUB = PrimitiveSceneInfo->CachedReflectionCaptureProxy->MobileUniformBuffer;
			}
			// If no reflection captures are available then attempt to use sky light's texture.
			else if (UseSkyReflectionCapture(Scene))
			{
				ReflectionUB = Scene->UniformBuffers.MobileSkyReflectionUniformBuffer;
			}
			ShaderBindings.Add(TanGramReflectionParameter, ReflectionUB);
		}
		

		//{
		//	FRHIUniformBuffer* ReflectionUB = GDefaultMobileReflectionCaptureUniformBuffer.GetUniformBufferRHI();
		//	ShaderBindings.Add(TanGramReflectionParameter, ReflectionUB);
		//}
		

		if (TanGramNumDynamicPointLightsParameter.IsBound())
		{
			static FHashedName MobileMovablePointLightHashedName[MAX_BASEPASS_DYNAMIC_POINT_LIGHTS] = { FHashedName(TEXT("MobileMovablePointLight0")), FHashedName(TEXT("MobileMovablePointLight1")), FHashedName(TEXT("MobileMovablePointLight2")), FHashedName(TEXT("MobileMovablePointLight3")) };

			// Set dynamic point lights
			FMobileBasePassMovableLightInfo LightInfo(PrimitiveSceneProxy);
			ShaderBindings.Add(TanGramNumDynamicPointLightsParameter, LightInfo.NumMovablePointLights);
			for (int32 i = 0; i < MAX_BASEPASS_DYNAMIC_POINT_LIGHTS; ++i)
			{
				if (i < LightInfo.NumMovablePointLights && LightInfo.MovablePointLightUniformBuffer[i])
				{
					ShaderBindings.Add(GetUniformBufferParameter(MobileMovablePointLightHashedName[i]), LightInfo.MovablePointLightUniformBuffer[i]);
				}
				else
				{
					ShaderBindings.Add(GetUniformBufferParameter(MobileMovablePointLightHashedName[i]), GDummyMovablePointLightUniformBuffer.GetUniformBufferRHI());
				}
			}
		}
	}
	else
	{
		ensure(!TanGramReflectionParameter.IsBound());
	}

	if (TanGramCSMDebugHintParams.IsBound())
	{
		static const auto CVarsCSMDebugHint = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.Mobile.Shadow.CSMDebugHint"));
		float CSMDebugValue = CVarsCSMDebugHint->GetValueOnRenderThread();
		ShaderBindings.Add(TanGramCSMDebugHintParams, CSMDebugValue);
	}

	if (TanGramUseCSMParameter.IsBound())
	{
		ShaderBindings.Add(TanGramUseCSMParameter, ShaderElementData.bCanReceiveCSM ? 1 : 0);
	}
}


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

template <int32 NumMovablePointLights>
bool GetUniformTanGramBasePassShaders(
	const FMaterial& Material,
	FVertexFactoryType* VertexFactoryType,
	TShaderRef<TTanGramBasePassVSPolicyParamType<FTanGramUniformLightMapPolicy>>& VertexShader,
	TShaderRef<TTanGramBasePassPSPolicyParamType<FTanGramUniformLightMapPolicy>>& PixelShader
)
{
	//using FVertexShaderType = TMobileBasePassVSPolicyParamType<FUniformLightMapPolicy>;
	//using FPixelShaderType = TMobileBasePassPSPolicyParamType<FUniformLightMapPolicy>;

	FMaterialShaderTypes ShaderTypes;
	ensure(IsMobileHDR());
	ShaderTypes.AddShaderType<TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>>();
	ShaderTypes.AddShaderType<TTanGramBasePassPS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>,NumMovablePointLights>>();

	FMaterialShaders Shaders;
	if (!Material.TryGetShaders(ShaderTypes, VertexFactoryType, Shaders))
	{
		return false;
	}

	Shaders.TryGetVertexShader(VertexShader);
	Shaders.TryGetPixelShader(PixelShader);
	return true;
}

bool TanGram::GetShaders(int32 NumMovablePointLights, const FMaterial& MaterialResource, FVertexFactoryType* VertexFactoryType,
	TShaderRef<TTanGramBasePassVSPolicyParamType<FTanGramUniformLightMapPolicy>>& VertexShader,
	TShaderRef<TTanGramBasePassPSPolicyParamType<FTanGramUniformLightMapPolicy>>& PixelShader)
{
	switch (NumMovablePointLights)
	{
	case INT32_MAX:
		return GetUniformTanGramBasePassShaders<INT32_MAX>(MaterialResource, VertexFactoryType, VertexShader, PixelShader);
	case 0:
	default:
		return GetUniformTanGramBasePassShaders<0>(MaterialResource, VertexFactoryType, VertexShader, PixelShader);
	}

}

FTanGramBasePassProcessor::FTanGramBasePassProcessor(
	const FScene* Scene,
	ERHIFeatureLevel::Type InFeatureLevel,
	const FSceneView* InViewIfDynamicMeshCommand,
	const FMeshPassProcessorRenderState& InDrawRenderState,
	FMeshPassDrawListContext* InDrawListContext)
	: FMeshPassProcessor(Scene, InFeatureLevel, InViewIfDynamicMeshCommand, InDrawListContext)
	, PassDrawRenderState(InDrawRenderState) {}

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
	TMeshProcessorShaders<TTanGramBasePassVSPolicyParamType<FTanGramUniformLightMapPolicy>, TTanGramBasePassPSPolicyParamType<FTanGramUniformLightMapPolicy>> BasePassShaders;
	
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
	//DrawRenderState.SetDepthStencilState
	//DrawRenderState.SetStencilRef(0);
	
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

	BuildMeshDrawCommands(
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


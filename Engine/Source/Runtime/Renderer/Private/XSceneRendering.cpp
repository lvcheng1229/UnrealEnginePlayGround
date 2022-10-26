//TangGram
#include "XSceneRendering.h"
#include "ScreenPass.h"
#include "ClearQuad.h"

#include "SystemTextures.h"

#include "ScenePrivate.h"
#include "MobileBasePassRendering.h"
#include "PostProcess/PostProcessing.h"

#include "TanGram/TanGramBasePassRendering.h"

DECLARE_CYCLE_STAT(TEXT("Opaque"), STAT_CLMM_Opaque, STATGROUP_CommandListMarkers);

RDG_REGISTER_BLACKBOARD_STRUCT(FSceneTextures);

//PRAGMA_DISABLE_OPTIMIZATION

FGlobalDynamicIndexBuffer FXMobileSceneRenderer::DynamicIndexBuffer;
FGlobalDynamicVertexBuffer FXMobileSceneRenderer::DynamicVertexBuffer;
TGlobalResource<FGlobalDynamicReadBuffer> FXMobileSceneRenderer::DynamicReadBuffer;

FXMobileSceneRenderer::FXMobileSceneRenderer(const FSceneViewFamily* InViewFamily, FHitProxyConsumer* HitProxyConsumer)
	: FSceneRenderer(InViewFamily, HitProxyConsumer)
{
	bRequiresSceneDepthAux = false;//True
	bKeepDepthContent = false;
	bIsFullDepthPrepassEnabled = false;//Only support mask material to render in pre-z
}

/** Helper class used to track and compute a suitable scene texture extent for the renderer based on history / global configuration. */
class FXSceneTextureExtentState
{
public:
	static FXSceneTextureExtentState& Get()
	{
		static FXSceneTextureExtentState Instance;
		return Instance;
	}

	FIntPoint Compute(const FSceneViewFamily& ViewFamily)
	{
		enum ESizingMethods { RequestedSize, ScreenRes, Grow, VisibleSizingMethodsCount };
		ESizingMethods SceneTargetsSizingMethod = Grow;

		bool bIsSceneCapture = false;
		bool bIsReflectionCapture = false;

		for (const FSceneView* View : ViewFamily.Views)
		{
			bIsSceneCapture |= View->bIsSceneCapture;
			bIsReflectionCapture |= View->bIsReflectionCapture;
		}

		FIntPoint DesiredExtent = FIntPoint::ZeroValue;
		FIntPoint DesiredFamilyExtent = FSceneRenderer::GetDesiredInternalBufferSize(ViewFamily);

		if (!FPlatformProperties::SupportsWindowedMode())
		{
			SceneTargetsSizingMethod = RequestedSize;
		}
		else if (GIsEditor)
		{
			SceneTargetsSizingMethod = Grow;
		}
	

		switch (SceneTargetsSizingMethod)
		{
		case RequestedSize:
			DesiredExtent = DesiredFamilyExtent;
			break;

		case Grow:
			DesiredExtent = FIntPoint(
				FMath::Max((int32)LastExtent.X, DesiredFamilyExtent.X),
				FMath::Max((int32)LastExtent.Y, DesiredFamilyExtent.Y));
			break;

		default:
			checkNoEntry();
		}

		const uint32 FrameNumber = ViewFamily.FrameNumber;
		if (ThisFrameNumber != FrameNumber)
		{
			ThisFrameNumber = FrameNumber;
			if (++DesiredExtentIndex == ExtentHistoryCount)
			{
				DesiredExtentIndex -= ExtentHistoryCount;
			}
			// This allows the extent to shrink each frame (in game)
			LargestDesiredExtents[DesiredExtentIndex] = FIntPoint::ZeroValue;
			HistoryFlags[DesiredExtentIndex] = ERenderTargetHistory::None;
		}

		// this allows The extent to not grow below the SceneCapture requests (happen before scene rendering, in the same frame with a Grow request)
		FIntPoint& LargestDesiredExtentThisFrame = LargestDesiredExtents[DesiredExtentIndex];
		LargestDesiredExtentThisFrame = LargestDesiredExtentThisFrame.ComponentMax(DesiredExtent);
		bool bIsHighResScreenshot = GIsHighResScreenshot;
		UpdateHistoryFlags(HistoryFlags[DesiredExtentIndex], bIsSceneCapture, bIsReflectionCapture, bIsHighResScreenshot);

		// We want to shrink the buffer but as we can have multiple scene captures per frame we have to delay that a frame to get all size requests
		// Don't save buffer size in history while making high-res screenshot.
		// We have to use the requested size when allocating an hmd depth target to ensure it matches the hmd allocated render target size.
		bool bAllowDelayResize = !GIsHighResScreenshot;

		// Don't consider the history buffer when the aspect ratio changes, the existing buffers won't make much sense at all.
		// This prevents problems when orientation changes on mobile in particular.
		// bIsReflectionCapture is explicitly checked on all platforms to prevent aspect ratio change detection from forcing the immediate buffer resize.
		// This ensures that 1) buffers are not resized spuriously during reflection rendering 2) all cubemap faces use the same render target size.
		if (bAllowDelayResize && !bIsReflectionCapture && !AnyCaptureRenderedRecently<ExtentHistoryCount>(ERenderTargetHistory::MaskAll))
		{
			const bool bAspectRatioChanged =
				!LastExtent.Y ||
				!FMath::IsNearlyEqual(
					(float)LastExtent.X / LastExtent.Y,
					(float)DesiredExtent.X / DesiredExtent.Y);

			if (bAspectRatioChanged)
			{
				bAllowDelayResize = false;

				// At this point we're assuming a simple output resize and forcing a hard swap so clear the history.
				// If we don't the next frame will fail this check as the allocated aspect ratio will match the new
				// frame's forced size so we end up looking through the history again, finding the previous old size
				// and reallocating. Only after a few frames can the results actually settle when the history clears 
				for (int32 i = 0; i < ExtentHistoryCount; ++i)
				{
					LargestDesiredExtents[i] = FIntPoint::ZeroValue;
					HistoryFlags[i] = ERenderTargetHistory::None;
				}
			}
		}
		const bool bAnyHighresScreenshotRecently = AnyCaptureRenderedRecently<ExtentHistoryCount>(ERenderTargetHistory::HighresScreenshot);
		if (bAnyHighresScreenshotRecently != GIsHighResScreenshot)
		{
			bAllowDelayResize = false;
		}

		if (bAllowDelayResize)
		{
			for (int32 i = 0; i < ExtentHistoryCount; ++i)
			{
				DesiredExtent = DesiredExtent.ComponentMax(LargestDesiredExtents[i]);
			}
		}

		check(DesiredExtent.X > 0 && DesiredExtent.Y > 0);
		QuantizeSceneBufferSize(DesiredExtent, DesiredExtent);
		LastExtent = DesiredExtent;
		return DesiredExtent;
	}

	void ResetHistory()
	{
		LastStereoExtent = FIntPoint(0, 0);
		LastExtent = FIntPoint(0, 0);
	}

private:
	enum class ERenderTargetHistory
	{
		None = 0,
		SceneCapture = 1 << 0,
		ReflectionCapture = 1 << 1,
		HighresScreenshot = 1 << 2,
		MaskAll = 1 << 3,
	};
	FRIEND_ENUM_CLASS_FLAGS(ERenderTargetHistory);

	static void UpdateHistoryFlags(ERenderTargetHistory& Flags, bool bIsSceneCapture, bool bIsReflectionCapture, bool bIsHighResScreenShot)
	{
		Flags |= bIsSceneCapture ? ERenderTargetHistory::SceneCapture : ERenderTargetHistory::None;
		Flags |= bIsReflectionCapture ? ERenderTargetHistory::ReflectionCapture : ERenderTargetHistory::None;
		Flags |= bIsHighResScreenShot ? ERenderTargetHistory::HighresScreenshot : ERenderTargetHistory::None;
	}

	template <uint32 EntryCount>
	bool AnyCaptureRenderedRecently(ERenderTargetHistory Mask) const
	{
		ERenderTargetHistory Result = ERenderTargetHistory::None;
		for (uint32 EntryIndex = 0; EntryIndex < EntryCount; ++EntryIndex)
		{
			Result |= HistoryFlags[EntryIndex] & Mask;
		}
		return Result != ERenderTargetHistory::None;
	}

	FXSceneTextureExtentState()
	{
		FMemory::Memset(LargestDesiredExtents, 0);
		FMemory::Memset(HistoryFlags, 0, sizeof(HistoryFlags));
	}

	FIntPoint LastStereoExtent = FIntPoint(0, 0);
	FIntPoint LastExtent = FIntPoint(0, 0);

	/** as we might get multiple extent requests each frame for SceneCaptures and we want to avoid reallocations we can only go as low as the largest request */
	static const uint32 ExtentHistoryCount = 3;
	uint32 DesiredExtentIndex = 0;
	FIntPoint LargestDesiredExtents[ExtentHistoryCount];
	ERenderTargetHistory HistoryFlags[ExtentHistoryCount];

	/** to detect when LargestDesiredSizeThisFrame is outdated */
	uint32 ThisFrameNumber = 0;
};

ENUM_CLASS_FLAGS(FXSceneTextureExtentState::ERenderTargetHistory);

//Temp !!!!!!!!!!!!!!!
//void FXMobileSceneRenderer::UpdateSkyReflectionUniformBuffer()
//{
//	FSkyLightSceneProxy* SkyLight = nullptr;
//	if (Scene->SkyLight
//		&& Scene->SkyLight->ProcessedTexture
//		&& Scene->SkyLight->ProcessedTexture->TextureRHI
//		// Don't use skylight reflection if it is a static sky light for keeping coherence with PC.
//		&& !Scene->SkyLight->bHasStaticLighting)
//	{
//		SkyLight = Scene->SkyLight;
//	}
//
//	FMobileReflectionCaptureShaderParameters Parameters;
//	SetupMobileSkyReflectionUniformParameters(SkyLight, Parameters);
//	Scene->UniformBuffers.MobileSkyReflectionUniformBuffer.UpdateUniformBufferImmediate(Parameters);
//}


void FXMobileSceneRenderer::Render(FRDGBuilder& GraphBuilder)
{
	Scene->UpdateAllPrimitiveSceneInfos(GraphBuilder);
	PrepareViewRectsForRendering(GraphBuilder.RHICmdList);

	if (!ViewFamily.EngineShowFlags.Rendering)
	{
		return;
	}


	FSceneTexturesConfig Config;
	{
		Config.FeatureLevel = ViewFamily.GetFeatureLevel();
		Config.ShadingPath = EShadingPath::Mobile;//TangGram
		Config.ShaderPlatform = GetFeatureLevelShaderPlatform(Config.FeatureLevel);
		Config.Extent = FXSceneTextureExtentState::Get().Compute(ViewFamily);
		Config.NumSamples = 1;// TangGram
		Config.EditorPrimitiveNumSamples = 1;//TangGram
		Config.ColorClearValue = FClearValueBinding::Black;
		Config.DepthClearValue = FClearValueBinding::DepthFar;
		Config.CustomDepthDownsampleFactor = GetCustomDepthDownsampleFactor(Config.FeatureLevel);
		Config.bRequireMultiView = ViewFamily.bRequireMultiView;
		Config.bIsUsingGBuffers = false;//TangGram
		Config.ColorFormat = PF_FloatRGBA;//TangGram
	}
	
	GSystemTextures.InitializeTextures(GraphBuilder.RHICmdList, FeatureLevel);
	FRDGSystemTextures::Create(GraphBuilder);
	FInstanceCullingManager& InstanceCullingManager = *GraphBuilder.AllocObject<FInstanceCullingManager>(false, GraphBuilder);

	//InitViews
	{
		FSceneTexturesConfig::Set(Config);
		for (int32 ViewIndex = Views.Num() - 1; ViewIndex >= 0; --ViewIndex)
		{
			FViewInfo& View = Views[ViewIndex];
			View.InitRHIResources();
		}
		Config.bKeepDepthContent = bKeepDepthContent;

		const FExclusiveDepthStencil::Type BasePassDepthStencilAccess = FExclusiveDepthStencil::DepthWrite_StencilWrite;
		FViewVisibleCommandsPerView ViewCommandsPerView;
		ViewCommandsPerView.SetNum(Views.Num());

		FRHICommandListImmediate& RHICmdList = GraphBuilder.RHICmdList;
		ComputeViewVisibility(RHICmdList, BasePassDepthStencilAccess, ViewCommandsPerView, DynamicIndexBuffer, DynamicVertexBuffer, DynamicReadBuffer, InstanceCullingManager);
		SetupMobileBasePassAfterShadowInit(BasePassDepthStencilAccess, ViewCommandsPerView, InstanceCullingManager);
		//UpdateSkyReflectionUniformBuffer();
	}

	DynamicIndexBuffer.Commit();
	DynamicVertexBuffer.Commit();
	DynamicReadBuffer.Commit();
	GraphBuilder.RHICmdList.ImmediateFlush(EImmediateFlushType::DispatchToRHIThread);

	FSceneTextures& SceneTextures = GraphBuilder.Blackboard.Create<FSceneTextures>(Config);
	{
		{
			ensure(Config.NumSamples == 1);
			ETextureCreateFlags Flags = TexCreate_DepthStencilTargetable | TexCreate_ShaderResource | TexCreate_InputAttachmentRead | GFastVRamConfig.SceneDepth;
			if (!Config.bKeepDepthContent)
			{
				Flags |= TexCreate_Memoryless;
			}

			FRDGTextureDesc Desc(FRDGTextureDesc::Create2D(SceneTextures.Config.Extent, PF_DepthStencil, Config.DepthClearValue, Flags));
			Desc.NumSamples = Config.NumSamples;
			SceneTextures.Depth = GraphBuilder.CreateTexture(Desc, TEXT("SceneDepthZ"));
			SceneTextures.Stencil = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::CreateWithPixelFormat(SceneTextures.Depth.Target, PF_X24_G8));
		}

		{
			ETextureCreateFlags Flags = TexCreate_RenderTargetable | TexCreate_ShaderResource | GFastVRamConfig.SceneColor | TexCreate_None;//TangGram Render in HDR linear space

			const TCHAR* SceneColorName = TEXT("SceneColor");
			FRDGTextureDesc Desc(FRDGTextureDesc::Create2D(Config.Extent, Config.ColorFormat, Config.ColorClearValue, Flags));
			Desc.NumSamples = Config.NumSamples;
			SceneTextures.Color = GraphBuilder.CreateTexture(Desc, SceneColorName);
		}

		SceneTextures.CustomDepth = FCustomDepthTextures::Create(GraphBuilder, Config.Extent, Config.FeatureLevel, Config.CustomDepthDownsampleFactor);
	}

	FRDGTextureRef ViewFamilyTexture = TryCreateViewFamilyTexture(GraphBuilder, ViewFamily);
	RenderForward(GraphBuilder, ViewFamilyTexture, SceneTextures);

	{
		RDG_EVENT_SCOPE(GraphBuilder, "PostProcessing");
		SCOPE_CYCLE_COUNTER(STAT_FinishRenderViewTargetTime);

		FMobilePostProcessingInputs PostProcessingInputs;
		PostProcessingInputs.ViewFamilyTexture = ViewFamilyTexture;
		PostProcessingInputs.SceneTextures = CreateMobileSceneTextureUniformBuffer(GraphBuilder, EMobileSceneTextureSetupMode::All);

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			RDG_EVENT_SCOPE_CONDITIONAL(GraphBuilder, Views.Num() > 1, "View%d", ViewIndex);
			AddMobilePostProcessingPasses(GraphBuilder, Scene, Views[ViewIndex], PostProcessingInputs, InstanceCullingManager);
		}
	}

	RenderFinish(GraphBuilder, ViewFamilyTexture);
}

void FXMobileSceneRenderer::UpdateDirectionalLightUniformBuffers(FRDGBuilder& GraphBuilder, const FViewInfo& View)
{
	if (CachedView == &View)
	{
		return;
	}
	CachedView = &View;

	AddPass(GraphBuilder, RDG_EVENT_NAME("UpdateDirectionalLightUniformBuffers"), [this, &View](FRHICommandListImmediate&)
		{
			const bool bDynamicShadows = ViewFamily.EngineShowFlags.DynamicShadows;
			// Fill in the other entries based on the lights
			for (int32 ChannelIdx = 0; ChannelIdx < UE_ARRAY_COUNT(Scene->MobileDirectionalLights); ChannelIdx++)
			{
				FMobileDirectionalLightShaderParameters Params;
				SetupMobileDirectionalLightUniformParameters(*Scene, View, VisibleLightInfos, ChannelIdx, bDynamicShadows, Params);
				Scene->UniformBuffers.MobileDirectionalLightUniformBuffers[ChannelIdx + 1].UpdateUniformBufferImmediate(Params);
			}
		});
}

BEGIN_SHADER_PARAMETER_STRUCT(FXMobileRenderPassParameters, RENDERER_API)
SHADER_PARAMETER_STRUCT_INCLUDE(FViewShaderParameters, View)
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FMobileBasePassUniformParameters, MobileBasePass)
RDG_BUFFER_ACCESS_ARRAY(DrawIndirectArgsBuffers)
RDG_BUFFER_ACCESS_ARRAY(InstanceIdOffsetBuffers)
RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

void FXMobileSceneRenderer::RenderForward(FRDGBuilder& GraphBuilder, FRDGTextureRef ViewFamilyTexture, FSceneTextures& SceneTextures)
{
	const FViewInfo& MainView = Views[0];

	FRDGTextureRef SceneColor = nullptr;
	FRDGTextureRef SceneColorResolve = nullptr;
	FRDGTextureRef SceneDepth = nullptr;

	ensure(SceneTextures.Config.NumSamples == 1);
	bool bMobileMSAA = false;

	SceneColor = SceneTextures.Color.Target;
	SceneColorResolve = bMobileMSAA ? SceneTextures.Color.Resolve : nullptr;
	SceneDepth = SceneTextures.Depth.Target;

	FRenderTargetBindingSlots BasePassRenderTargets;
	BasePassRenderTargets[0] = FRenderTargetBinding(SceneColor, SceneColorResolve, ERenderTargetLoadAction::EClear);
	
	BasePassRenderTargets.DepthStencil = FDepthStencilBinding(SceneDepth, ERenderTargetLoadAction::EClear, ERenderTargetLoadAction::EClear, FExclusiveDepthStencil::DepthWrite_StencilWrite);
	BasePassRenderTargets.ShadingRateTexture = nullptr;
	BasePassRenderTargets.SubpassHint = ESubpassHint::DepthReadSubpass;

	BasePassRenderTargets.MultiViewCount = 0;

	const FRDGSystemTextures& SystemTextures = FRDGSystemTextures::Get(GraphBuilder);

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		FViewInfo& View = Views[ViewIndex];

		SCOPED_CONDITIONAL_DRAW_EVENTF(GraphBuilder.RHICmdList, EventView, Views.Num() > 1, TEXT("View%d"), ViewIndex);

		if (!View.ShouldRenderView())
		{
			continue;
		}

		if (ViewIndex > 0)
		{
			BasePassRenderTargets[0].SetLoadAction(ERenderTargetLoadAction::ELoad);
			BasePassRenderTargets.DepthStencil.SetDepthLoadAction(ERenderTargetLoadAction::ELoad);
			BasePassRenderTargets.DepthStencil.SetStencilLoadAction(ERenderTargetLoadAction::ELoad);
			BasePassRenderTargets.DepthStencil.SetDepthStencilAccess(bIsFullDepthPrepassEnabled ? FExclusiveDepthStencil::DepthRead_StencilWrite : FExclusiveDepthStencil::DepthWrite_StencilWrite);
		}

		View.BeginRenderView();

		UpdateDirectionalLightUniformBuffers(GraphBuilder, View);

		FMobileBasePassTextures MobileBasePassTextures{};
		MobileBasePassTextures.ScreenSpaceAO = SystemTextures.White;

		EMobileSceneTextureSetupMode SetupMode = EMobileSceneTextureSetupMode::CustomDepth;
		FXMobileRenderPassParameters* PassParameters = GraphBuilder.AllocParameters<FXMobileRenderPassParameters>();
		PassParameters->View = View.GetShaderParameters();
		PassParameters->MobileBasePass = CreateMobileBasePassUniformBuffer(GraphBuilder, View, EMobileBasePass::Opaque, SetupMode, MobileBasePassTextures);
		PassParameters->RenderTargets = BasePassRenderTargets;

		RenderForwardMultiPass(GraphBuilder, PassParameters, BasePassRenderTargets, ViewIndex, View, SceneTextures);
	}
}

void FXMobileSceneRenderer::RenderMobileBasePass(FRHICommandListImmediate& RHICmdList, const FViewInfo& View)
{
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(RenderBasePass);
	SCOPED_DRAW_EVENT(RHICmdList, MobileBasePass);
	SCOPE_CYCLE_COUNTER(STAT_BasePassDrawTime);
	SCOPED_GPU_STAT(RHICmdList, Basepass);

	RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1);
	View.ParallelMeshDrawCommandPasses[EMeshPass::BasePass].DispatchDraw(nullptr, RHICmdList, nullptr);
}
void FXMobileSceneRenderer::RenderForwardMultiPass(FRDGBuilder& GraphBuilder, FXMobileRenderPassParameters* PassParameters, FRenderTargetBindingSlots& BasePassRenderTargets, int32 ViewIndex, FViewInfo& View, FSceneTextures& SceneTextures)
{
	GraphBuilder.AddPass(
		RDG_EVENT_NAME("SceneColorRendering"),
		PassParameters,
		ERDGPassFlags::Raster,
		[this, PassParameters, ViewIndex, &View, &SceneTextures](FRHICommandListImmediate& RHICmdList)
		{
			if (GIsEditor && !View.bIsSceneCapture && ViewIndex == 0)
			{
				DrawClearQuad(RHICmdList, View.BackgroundColor);
			}

			RHICmdList.SetCurrentStat(GET_STATID(STAT_CLMM_Opaque));
			RenderMobileBasePass(RHICmdList, View);
		});
}

void FXMobileSceneRenderer::SetupMobileBasePassAfterShadowInit(FExclusiveDepthStencil::Type BasePassDepthStencilAccess, FViewVisibleCommandsPerView& ViewCommandsPerView, FInstanceCullingManager& InstanceCullingManager)
{
	// Sort front to back on all platforms, even HSR benefits from it
	//const bool bWantsFrontToBackSorting = (GHardwareHiddenSurfaceRemoval == false);

	// compute keys for front to back sorting and dispatch pass setup.
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
	{
		FViewInfo& View = Views[ViewIndex];
		FViewCommands& ViewCommands = ViewCommandsPerView[ViewIndex];

		PassProcessorCreateFunction CreateFunction = FPassProcessorManager::GetCreateFunction(EShadingPath::Mobile, EMeshPass::BasePass);
		FMeshPassProcessor* MeshPassProcessor = CreateFunction(Scene, &View, nullptr);

		PassProcessorCreateFunction BasePassCSMCreateFunction = FPassProcessorManager::GetCreateFunction(EShadingPath::Mobile, EMeshPass::MobileBasePassCSM);
		FMeshPassProcessor* BasePassCSMMeshPassProcessor = BasePassCSMCreateFunction(Scene, &View, nullptr);

		TArray<int32, TInlineAllocator<2> > ViewIds;
		ViewIds.Add(View.GPUSceneViewId);
		// Only apply instancing for ISR to main view passes
		EInstanceCullingMode InstanceCullingMode = View.IsInstancedStereoPass() ? EInstanceCullingMode::Stereo : EInstanceCullingMode::Normal;
		if (InstanceCullingMode == EInstanceCullingMode::Stereo)
		{
			check(View.GetInstancedView() != nullptr);
			ViewIds.Add(View.GetInstancedView()->GPUSceneViewId);
		}

		// Run sorting on BasePass, as it's ignored inside FSceneRenderer::SetupMeshPass, so it can be done after shadow init on mobile.
		FParallelMeshDrawCommandPass& Pass = View.ParallelMeshDrawCommandPasses[EMeshPass::BasePass];
		if (ShouldDumpMeshDrawCommandInstancingStats())
		{
			Pass.SetDumpInstancingStats(GetMeshPassName(EMeshPass::BasePass));
		}

		Pass.DispatchPassSetup(
			Scene,
			View,
			FInstanceCullingContext(FeatureLevel, &InstanceCullingManager, ViewIds, nullptr, InstanceCullingMode),
			EMeshPass::BasePass,
			BasePassDepthStencilAccess,
			MeshPassProcessor,
			View.DynamicMeshElements,
			&View.DynamicMeshElementsPassRelevance,
			View.NumVisibleDynamicMeshElements[EMeshPass::BasePass],
			ViewCommands.DynamicMeshCommandBuildRequests[EMeshPass::BasePass],
			ViewCommands.NumDynamicMeshCommandBuildRequestElements[EMeshPass::BasePass],
			ViewCommands.MeshCommands[EMeshPass::BasePass],
			BasePassCSMMeshPassProcessor,
			&ViewCommands.MeshCommands[EMeshPass::MobileBasePassCSM]);
	}
}
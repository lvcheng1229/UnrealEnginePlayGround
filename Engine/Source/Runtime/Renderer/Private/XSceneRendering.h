//tanggram
#pragma once

#include "SceneRendering.h"

class FXMobileSceneRenderer : public FSceneRenderer
{
public:
	FXMobileSceneRenderer(const FSceneViewFamily* InViewFamily, FHitProxyConsumer* HitProxyConsumer);
	virtual void Render(FRDGBuilder& GraphBuilder) override;
protected:
	void RenderForward(FRDGBuilder& GraphBuilder, FRDGTextureRef ViewFamilyTexture, FSceneTextures& SceneTextures);
	void UpdateDirectionalLightUniformBuffers(FRDGBuilder& GraphBuilder, const FViewInfo& View);
	void RenderForwardMultiPass(FRDGBuilder& GraphBuilder, class FXMobileRenderPassParameters* PassParameters, FRenderTargetBindingSlots& BasePassRenderTargets, int32 ViewIndex, FViewInfo& View, FSceneTextures& SceneTextures);
	void RenderMobileBasePass(FRHICommandListImmediate& RHICmdList, const FViewInfo& View);
	void SetupMobileBasePassAfterShadowInit(FExclusiveDepthStencil::Type BasePassDepthStencilAccess, FViewVisibleCommandsPerView& ViewCommandsPerView, FInstanceCullingManager& InstanceCullingManager);
private:
	bool bRequiresSceneDepthAux;
	bool bKeepDepthContent;
	bool bIsFullDepthPrepassEnabled;

	static FGlobalDynamicIndexBuffer DynamicIndexBuffer;
	static FGlobalDynamicVertexBuffer DynamicVertexBuffer;
	static TGlobalResource<FGlobalDynamicReadBuffer> DynamicReadBuffer;

	const FViewInfo* CachedView = nullptr;
};
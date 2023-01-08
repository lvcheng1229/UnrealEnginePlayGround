#pragma once
#include "TanGramLightMapRendering.h"
//#include "BasePassRendering.h"


//!
#include "MobileBasePassRendering.h"

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "RHI.h"
#include "ShaderParameters.h"
#include "Shader.h"
#include "HitProxies.h"
#include "RHIStaticStates.h"
#include "SceneManagement.h"
#include "PrimitiveSceneInfo.h"
#include "PostProcess/SceneRenderTargets.h"
#include "LightMapRendering.h"
#include "MeshMaterialShaderType.h"
#include "MeshMaterialShader.h"
#include "FogRendering.h"
#include "PlanarReflectionRendering.h"
#include "BasePassRendering.h"
#include "SkyAtmosphereRendering.h"
#include "RenderUtils.h"
#include "DebugViewModeRendering.h"
#include "TanGramShader.h"

template<typename LightMapPolicyType>
class TTanGramBasePassShaderElementData : public FMeshMaterialShaderElementData
{
public:
	TTanGramBasePassShaderElementData(const typename LightMapPolicyType::ElementDataType& InLightMapPolicyElementData, bool bInCanReceiveCSM)
		: LightMapPolicyElementData(InLightMapPolicyElementData)
		, bCanReceiveCSM(bInCanReceiveCSM)
	{}

	typename LightMapPolicyType::ElementDataType LightMapPolicyElementData;

	const bool bCanReceiveCSM;
};

//class TTanGramBasePassVS : public FMeshMaterialShader
class TTanGramBasePassVS : public FTanGramShader
{
public:
	using ShaderMetaType = FMeshMaterialShaderType;
	using ShaderMapType = FMeshMaterialShaderMap;
	static  ShaderMetaType StaticType;

	static FShader* ConstructSerializedInstance()
	{
		return new TTanGramBasePassVS();
	}
	
	static FShader* ConstructCompiledInstance(const typename FShader::CompiledShaderInitializerType& Initializer)
	{
		return new TTanGramBasePassVS(static_cast<const typename ShaderMetaType::CompiledShaderInitializerType&>(Initializer));
	}
	
	static void ModifyCompilationEnvironmentImpl(const FShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		const typename TTanGramBasePassVS::FPermutationDomain PermutationVector(Parameters.PermutationId);
		PermutationVector.ModifyCompilationEnvironment(OutEnvironment);
		TTanGramBasePassVS::ModifyCompilationEnvironment(static_cast<const typename TTanGramBasePassVS::FPermutationParameters&>(Parameters), OutEnvironment);
	}
	static bool ShouldCompilePermutationImpl(const FShaderPermutationParameters& Parameters)
	{
		return TTanGramBasePassVS::ShouldCompilePermutation(static_cast<const typename TTanGramBasePassVS::FPermutationParameters&>(Parameters));
	};
private:
	using InternalBaseType = typename TGetBaseTypeHelper< TTanGramBasePassVS>::Type;
	template<typename InternalType>
	static void InternalInitializeBases(FTypeLayoutDesc& TypeDesc)
	{
		TInitializeBaseHelper<InternalType, InternalBaseType>::Initialize(TypeDesc);
	};

	static void InternalDestroy(void* Object, const FTypeLayoutDesc&, const FPointerTableBase* PtrTable, bool bIsFrozen);
public:
	static FTypeLayoutDesc& StaticGetTypeLayout();
	const FTypeLayoutDesc& GetTypeLayout() const ;
	static constexpr int CounterBase = 189623592;
	using DerivedType =  TTanGramBasePassVS;
	static constexpr ETypeLayoutInterface::Type InterfaceType = ETypeLayoutInterface::NonVirtual;

	template<int Counter>
	struct InternalLinkType
	{
		static __forceinline void Initialize(FTypeLayoutDesc& TypeDesc) {}
	};
	
public:
	static bool ShouldCompilePermutation(const FTanGramShaderPermutationParameters& Parameters)
	{		
		return true;
	}

	static void ModifyCompilationEnvironment(const FTanGramShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
	}
	
	TTanGramBasePassVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FTanGramShader(Initializer)
	{}

	TTanGramBasePassVS() {}
};

class TTanGramBasePassPS :  public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(TTanGramBasePassPS,MeshMaterial);
public:

	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{		
		return true;
	}
	
	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{		
	}
	
	/** Initialization constructor. */
	TTanGramBasePassPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FMeshMaterialShader(Initializer)
	{}

	/** Default constructor. */
	TTanGramBasePassPS() {}
};


namespace TanGram
{
	const FLightSceneInfo* GetDirectionalLightInfo(const FScene* Scene, const FPrimitiveSceneProxy* PrimitiveSceneProxy);
	bool GetShaders(int32 NumMovablePointLights, const FMaterial& MaterialResource, FVertexFactoryType* VertexFactoryType,
		TShaderRef<TTanGramBasePassVS>& VertexShader,
		TShaderRef<TTanGramBasePassPS>& PixelShader);
};





class FTanGramBasePassProcessor :public FMeshPassProcessor
{
public:
	FTanGramBasePassProcessor(
		const FScene* InScene,
		ERHIFeatureLevel::Type InFeatureLevel,
		const FSceneView* InViewIfDynamicMeshCommand,
		const FMeshPassProcessorRenderState& InDrawRenderState,
		FMeshPassDrawListContext* InDrawListContext);

	virtual void AddMeshBatch(const FMeshBatch& RESTRICT MeshBatch, uint64 BatchElementMask, const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy, int32 StaticMeshId = -1) override final;
	FMeshPassProcessorRenderState PassDrawRenderState;
private:
	bool TryAddMeshBatch(const FMeshBatch& RESTRICT MeshBatch, uint64 BatchElementMask, const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy, int32 StaticMeshId, const FMaterialRenderProxy& MaterialRenderProxy, const FMaterial& Material);

	bool Process(
		const FMeshBatch& RESTRICT MeshBatch,
		uint64 BatchElementMask,
		int32 StaticMeshId,
		const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
		const FMaterialRenderProxy& RESTRICT MaterialRenderProxy,
		const FMaterial& RESTRICT MaterialResource,
		EBlendMode BlendMode,
		FMaterialShadingModelField ShadingModels,
		const FUniformLightMapPolicy::ElementDataType& RESTRICT LightMapElementData);
};
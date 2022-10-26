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









template<typename LightMapPolicyType>
class TTanGramBasePassVSPolicyParamType : public FMeshMaterialShader, public LightMapPolicyType::VertexParametersType
{
	DECLARE_INLINE_TYPE_LAYOUT_EXPLICIT_BASES(TTanGramBasePassVSPolicyParamType, NonVirtual, FMeshMaterialShader, typename LightMapPolicyType::VertexParametersType);
protected:

	TTanGramBasePassVSPolicyParamType() {}
	TTanGramBasePassVSPolicyParamType(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer) :
		FMeshMaterialShader(Initializer)
	{
		LightMapPolicyType::VertexParametersType::Bind(Initializer.ParameterMap);
		PassUniformBuffer.Bind(Initializer.ParameterMap, FMobileBasePassUniformParameters::StaticStructMetadata.GetShaderVariableName());
	}

public:
	//static bool ShouldCompilePermutation(EShaderPlatform Platform, const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)

	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMeshMaterialShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

	void GetShaderBindings(
		const FScene* Scene,
		ERHIFeatureLevel::Type FeatureLevel,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMaterialRenderProxy& MaterialRenderProxy,
		const FMaterial& Material,
		const FMeshPassProcessorRenderState& DrawRenderState,
		const TTanGramBasePassShaderElementData<LightMapPolicyType>& ShaderElementData,
		FMeshDrawSingleShaderBindings& ShaderBindings) const
	{
		FMeshMaterialShader::GetShaderBindings(Scene, FeatureLevel, PrimitiveSceneProxy, MaterialRenderProxy, Material, DrawRenderState, ShaderElementData, ShaderBindings);
		LightMapPolicyType::GetVertexShaderBindings(PrimitiveSceneProxy,ShaderElementData.LightMapPolicyElementData,this,ShaderBindings);
	}
};


template<typename LightMapPolicyType>
class TTanGramBasePassVSBaseType : public TTanGramBasePassVSPolicyParamType<LightMapPolicyType>
{
	typedef TTanGramBasePassVSPolicyParamType<LightMapPolicyType> Super;
protected:

	TTanGramBasePassVSBaseType() {}
	TTanGramBasePassVSBaseType(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer) : Super(Initializer) {}

public:

	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		return IsMobilePlatform(Parameters.Platform) && LightMapPolicyType::ShouldCompilePermutation(Parameters);
	}

	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		LightMapPolicyType::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		Super::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};

template< typename LightMapPolicyType>
class TTanGramBasePassVS : public TTanGramBasePassVSBaseType<LightMapPolicyType>
{
	/*
	 * MACRO BEGIN
	 */
	public: using ShaderMetaType = FMeshMaterialShaderType;
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
		TTanGramBasePassVS::ModifyCompilationEnvironment(
			static_cast<const typename TTanGramBasePassVS::FPermutationParameters&>(Parameters), OutEnvironment);
	}
	static bool ShouldCompilePermutationImpl(const FShaderPermutationParameters& Parameters)
	{
		return TTanGramBasePassVS::ShouldCompilePermutation(static_cast<const typename TTanGramBasePassVS::FPermutationParameters&>(Parameters));
	};
	private: using InternalBaseType = typename TGetBaseTypeHelper< TTanGramBasePassVS>::Type;
	template<typename InternalType>
	static void InternalInitializeBases(FTypeLayoutDesc& TypeDesc)
	{
		TInitializeBaseHelper<InternalType, InternalBaseType>::Initialize(TypeDesc);
	};
private:
	static void InternalDestroy(void* Object, const FTypeLayoutDesc&, const FPointerTableBase* PtrTable, bool bIsFrozen);
public:
	static FTypeLayoutDesc& StaticGetTypeLayout();
	const FTypeLayoutDesc& GetTypeLayout() const ;
	static constexpr int CounterBase = 189623592;
	using DerivedType =  TTanGramBasePassVS;
	static constexpr ETypeLayoutInterface::Type InterfaceType = ETypeLayoutInterface::NonVirtual;
	template<int Counter> struct InternalLinkType
	{
		static __forceinline void Initialize(FTypeLayoutDesc& TypeDesc) {}
	};
	
	/*
	 * MACRO END
	 */
	
public:
	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{		
		return TTanGramBasePassVSBaseType<LightMapPolicyType>::ShouldCompilePermutation(Parameters);
	}

	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		TTanGramBasePassVSBaseType<LightMapPolicyType>::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine( TEXT("OUTPUT_GAMMA_SPACE"), 0u);
		OutEnvironment.SetDefine( TEXT("OUTPUT_MOBILE_HDR"), 1u);
	}
	
	TTanGramBasePassVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		TTanGramBasePassVSBaseType<LightMapPolicyType>(Initializer)
	{}

	TTanGramBasePassVS() {}
};































#define MAX_TANGRAM_BASEPASS_DYNAMIC_POINT_LIGHTS 4



template<typename LightMapPolicyType>
class TTanGramBasePassPSPolicyParamType : public FMeshMaterialShader, public LightMapPolicyType::PixelParametersType
{
	DECLARE_INLINE_TYPE_LAYOUT_EXPLICIT_BASES(TTanGramBasePassPSPolicyParamType, NonVirtual, FMeshMaterialShader, typename LightMapPolicyType::PixelParametersType);
public:
	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		return IsMobilePlatform(Parameters.Platform);
	}
	
	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMeshMaterialShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		
		// This define simply lets the compilation environment know that we are using a Base Pass PixelShader.
		OutEnvironment.SetDefine(TEXT("IS_BASE_PASS"), 1);
		OutEnvironment.SetDefine(TEXT("IS_MOBILE_BASE_PASS"), 1);
		
		
		
		//??? For TanGramReflectionParameter
		ModifyCompilationEnvironmentForQualityLevel(Parameters.Platform, Parameters.MaterialParameters.QualityLevel, OutEnvironment);
	}
	
	/** Initialization constructor. */
	TTanGramBasePassPSPolicyParamType(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer)
		: FMeshMaterialShader(Initializer)
	{
		LightMapPolicyType::PixelParametersType::Bind(Initializer.ParameterMap);
		PassUniformBuffer.Bind(Initializer.ParameterMap, FMobileBasePassUniformParameters::StaticStructMetadata.GetShaderVariableName());
		TanGramDirectionLightBufferParam.Bind(Initializer.ParameterMap, FMobileDirectionalLightShaderParameters::StaticStructMetadata.GetShaderVariableName());


		
		TanGramReflectionParameter.Bind(Initializer.ParameterMap, FMobileReflectionCaptureShaderParameters::StaticStructMetadata.GetShaderVariableName());
		//TanGramHQReflectionCubemaps[0].Bind(Initializer.ParameterMap, TEXT("ReflectionCubemap0"));
		//TanGramHQReflectionSamplers[0].Bind(Initializer.ParameterMap, TEXT("ReflectionCubemapSampler0"));
		//TanGramHQReflectionCubemaps[1].Bind(Initializer.ParameterMap, TEXT("ReflectionCubemap1"));
		//TanGramHQReflectionSamplers[1].Bind(Initializer.ParameterMap, TEXT("ReflectionCubemapSampler1"));
		//TanGramHQReflectionCubemaps[2].Bind(Initializer.ParameterMap, TEXT("ReflectionCubemap2"));
		//TanGramHQReflectionSamplers[2].Bind(Initializer.ParameterMap, TEXT("ReflectionCubemapSampler2"));
		TanGramHQReflectionInvAverageBrigtnessParams.Bind(Initializer.ParameterMap, TEXT("ReflectionAverageBrigtness"));
		TanGramHQReflectanceMaxValueRGBMParams.Bind(Initializer.ParameterMap, TEXT("ReflectanceMaxValueRGBM"));
		TanGramHQReflectionPositionsAndRadii.Bind(Initializer.ParameterMap, TEXT("ReflectionPositionsAndRadii"));
		TanGramHQReflectionTilePositions.Bind(Initializer.ParameterMap, TEXT("ReflectionTilePositions"));
		TanGramHQReflectionCaptureBoxTransformArray.Bind(Initializer.ParameterMap, TEXT("CaptureBoxTransformArray"));
		TanGramHQReflectionCaptureBoxScalesArray.Bind(Initializer.ParameterMap, TEXT("CaptureBoxScalesArray"));
		TanGramNumDynamicPointLightsParameter.Bind(Initializer.ParameterMap, TEXT("NumDynamicPointLights"));
		TanGramCSMDebugHintParams.Bind(Initializer.ParameterMap, TEXT("CSMDebugHint"));
		TanGramUseCSMParameter.Bind(Initializer.ParameterMap, TEXT("UseCSM"));
	}
	
	TTanGramBasePassPSPolicyParamType() {}

private:
	LAYOUT_FIELD(FShaderUniformBufferParameter, TanGramDirectionLightBufferParam);



	LAYOUT_FIELD(FShaderUniformBufferParameter, TanGramReflectionParameter);
	LAYOUT_ARRAY(FShaderResourceParameter, TanGramHQReflectionCubemaps, 3);
	LAYOUT_ARRAY(FShaderResourceParameter, TanGramHQReflectionSamplers, 3);
	LAYOUT_FIELD(FShaderParameter, TanGramHQReflectionInvAverageBrigtnessParams);
	LAYOUT_FIELD(FShaderParameter, TanGramHQReflectanceMaxValueRGBMParams);
	LAYOUT_FIELD(FShaderParameter, TanGramHQReflectionPositionsAndRadii);
	LAYOUT_FIELD(FShaderParameter, TanGramHQReflectionTilePositions);
	LAYOUT_FIELD(FShaderParameter, TanGramHQReflectionCaptureBoxTransformArray);
	LAYOUT_FIELD(FShaderParameter, TanGramHQReflectionCaptureBoxScalesArray);
	LAYOUT_FIELD(FShaderParameter, TanGramNumDynamicPointLightsParameter);
	LAYOUT_FIELD(FShaderParameter, TanGramCSMDebugHintParams);
	LAYOUT_FIELD(FShaderParameter, TanGramUseCSMParameter);


public:
	void GetShaderBindings(
		const FScene* Scene,
		ERHIFeatureLevel::Type FeatureLevel,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMaterialRenderProxy& MaterialRenderProxy,
		const FMaterial& Material,
		const FMeshPassProcessorRenderState& DrawRenderState,
		const TTanGramBasePassShaderElementData<LightMapPolicyType>& ShaderElementData,
		FMeshDrawSingleShaderBindings& ShaderBindings) const;

private:
	static bool ModifyCompilationEnvironmentForQualityLevel(EShaderPlatform Platform, EMaterialQualityLevel::Type QualityLevel, FShaderCompilerEnvironment& OutEnvironment);
};


template<typename LightMapPolicyType>
class TTanGramBasePassPSBaseType : public TTanGramBasePassPSPolicyParamType<LightMapPolicyType>
{
	typedef TTanGramBasePassPSPolicyParamType<LightMapPolicyType> Super;
	DECLARE_INLINE_TYPE_LAYOUT(TTanGramBasePassPSBaseType, NonVirtual);
public:
	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		return LightMapPolicyType::ShouldCompilePermutation(Parameters)
			&& Super::ShouldCompilePermutation(Parameters);
	}

	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		LightMapPolicyType::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		Super::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

	/** Initialization constructor. */
	TTanGramBasePassPSBaseType(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer) : Super(Initializer) {}
	TTanGramBasePassPSBaseType() {}
};

template< typename LightMapPolicyType,int32 NumMovablePointLights>
class TTanGramBasePassPS : public TTanGramBasePassPSBaseType<LightMapPolicyType>
{
	DECLARE_SHADER_TYPE(TTanGramBasePassPS,MeshMaterial);
public:

	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{		
		return TTanGramBasePassPSBaseType<LightMapPolicyType>::ShouldCompilePermutation(Parameters);
	}
	
	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{		
		TTanGramBasePassPSBaseType<LightMapPolicyType>::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("ENABLE_SKY_LIGHT"), 0u);
		OutEnvironment.SetDefine(TEXT("OUTPUT_GAMMA_SPACE"), 0u);
		OutEnvironment.SetDefine(TEXT("OUTPUT_MOBILE_HDR"), 1u);
		if (NumMovablePointLights == INT32_MAX)
		{
			OutEnvironment.SetDefine(TEXT("MAX_DYNAMIC_POINT_LIGHTS"), (uint32)MAX_TANGRAM_BASEPASS_DYNAMIC_POINT_LIGHTS);
			OutEnvironment.SetDefine(TEXT("VARIABLE_NUM_DYNAMIC_POINT_LIGHTS"), (uint32)1);
		}
		else
		{
			OutEnvironment.SetDefine(TEXT("MAX_DYNAMIC_POINT_LIGHTS"), (uint32)NumMovablePointLights);
			OutEnvironment.SetDefine(TEXT("VARIABLE_NUM_DYNAMIC_POINT_LIGHTS"), (uint32)0);
			OutEnvironment.SetDefine(TEXT("NUM_DYNAMIC_POINT_LIGHTS"), (uint32)NumMovablePointLights);
		}

		OutEnvironment.SetDefine(TEXT("ENABLE_AMBIENT_OCCLUSION"), 0u);
		OutEnvironment.SetDefine(TEXT("ENABLE_DISTANCE_FIELD"), 0u);
	}
	
	/** Initialization constructor. */
	TTanGramBasePassPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		TTanGramBasePassPSBaseType<LightMapPolicyType>(Initializer)
	{}

	/** Default constructor. */
	TTanGramBasePassPS() {}
};


namespace TanGram
{
	const FLightSceneInfo* GetDirectionalLightInfo(const FScene* Scene, const FPrimitiveSceneProxy* PrimitiveSceneProxy);
	bool GetShaders(int32 NumMovablePointLights, const FMaterial& MaterialResource, FVertexFactoryType* VertexFactoryType,
		TShaderRef<TTanGramBasePassVSPolicyParamType<FTanGramUniformLightMapPolicy> >& VertexShader,
		TShaderRef<TTanGramBasePassPSPolicyParamType<FTanGramUniformLightMapPolicy> >& PixelShader);
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
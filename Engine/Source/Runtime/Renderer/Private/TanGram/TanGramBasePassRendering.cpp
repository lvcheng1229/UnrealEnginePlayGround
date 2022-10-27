#include "TanGramBasePassRendering.h"

#include "MobileBasePassRendering.h"
#include "DynamicPrimitiveDrawing.h"
#include "ScenePrivate.h"
#include "SceneTextureParameters.h"
#include "ShaderPlatformQualitySettings.h"
#include "MaterialShaderQualitySettings.h"
#include "PrimitiveSceneInfo.h"
#include "MeshPassProcessor.h"
#include "MeshPassProcessor.inl"
#include "EditorPrimitivesRendering.h"

#include "FramePro/FrameProProfiler.h"
#include "PostProcess/PostProcessPixelProjectedReflectionMobile.h"
#include "Engine/SubsurfaceProfile.h"

template<>
void  TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>::InternalDestroy
(void* Object, const FTypeLayoutDesc&, const FPointerTableBase* PtrTable, bool bIsFrozen)
{
	Freeze::DestroyObject(static_cast<TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>*>(Object), PtrTable, bIsFrozen);
}

template<>
FTypeLayoutDesc& TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>::StaticGetTypeLayout()
{
	static_assert(TValidateInterfaceHelper< TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>, InterfaceType>::Value, "Invalid interface for " "TTanGramBasePassVSFNoLightMapPolicyHDRLinear64");
	alignas(FTypeLayoutDesc)static uint8 TypeBuffer[sizeof(FTypeLayoutDesc)] = { 0 };
	FTypeLayoutDesc& TypeDesc = *(FTypeLayoutDesc*)TypeBuffer;
	if (!TypeDesc.IsInitialized)
	{
		TypeDesc.IsInitialized = true;
		TypeDesc.Name = L"TTanGramBasePassVSFNoLightMapPolicyHDRLinear64";
		TypeDesc.WriteFrozenMemoryImageFunc = TGetFreezeImageHelper< TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>>::Do();
		TypeDesc.UnfrozenCopyFunc = &Freeze::DefaultUnfrozenCopy;
		TypeDesc.AppendHashFunc = &Freeze::DefaultAppendHash;
		TypeDesc.GetTargetAlignmentFunc = &Freeze::DefaultGetTargetAlignment;
		TypeDesc.ToStringFunc = &Freeze::DefaultToString;
		TypeDesc.DestroyFunc = &InternalDestroy;
		TypeDesc.Size = sizeof(TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>);
		TypeDesc.Alignment = alignof(TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>);
		TypeDesc.Interface = InterfaceType;
		TypeDesc.SizeFromFields = ~0u;
		TypeDesc.GetDefaultObjectFunc = &TGetDefaultObjectHelper< TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>, InterfaceType>::Do;
		InternalLinkType<1>::Initialize(TypeDesc);
		InternalInitializeBases< TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>>(TypeDesc);
		FTypeLayoutDesc::Initialize(TypeDesc);
	}
	return TypeDesc;
};

template<>
const FTypeLayoutDesc& TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>::GetTypeLayout() const
{
	return StaticGetTypeLayout();
}



template<>
TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>::ShaderMetaType
TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>::StaticType
(
	TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>::StaticGetTypeLayout(),
	L"TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>",
	L"/Engine/Private/MobileBasePassVertexShader.usf",
	L"Main",
	SF_Vertex,
	TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>::FPermutationDomain::PermutationCount,
	TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>::ConstructSerializedInstance,
	TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>::ConstructCompiledInstance,
	TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>::ModifyCompilationEnvironmentImpl,
	TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>::ShouldCompilePermutationImpl,
	TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>::ValidateCompiledResult,
	sizeof(TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>),
	TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>::GetRootParametersMetadata()
);

template<typename LightMapPolicyType>
bool TTanGramBasePassPSPolicyParamType<LightMapPolicyType>::ModifyCompilationEnvironmentForQualityLevel(EShaderPlatform Platform, EMaterialQualityLevel::Type QualityLevel, FShaderCompilerEnvironment& OutEnvironment)
{
	// Get quality settings for shader platform
	const UShaderPlatformQualitySettings* MaterialShadingQuality = UMaterialShaderQualitySettings::Get()->GetShaderPlatformQualitySettings(Platform);
	const FMaterialQualityOverrides& QualityOverrides = MaterialShadingQuality->GetQualityOverrides(QualityLevel);

	// the point of this check is to keep the logic between enabling overrides here and in UMaterial::GetQualityLevelUsage() in sync
	checkf(QualityOverrides.CanOverride(Platform), TEXT("ShaderPlatform %d was not marked as being able to use quality overrides! Include it in CanOverride() and recook."), static_cast<int32>(Platform));
	OutEnvironment.SetDefine(TEXT("MOBILE_QL_FORCE_FULLY_ROUGH"), QualityOverrides.bEnableOverride && QualityOverrides.bForceFullyRough != 0 ? 1u : 0u);
	OutEnvironment.SetDefine(TEXT("MOBILE_QL_FORCE_NONMETAL"), QualityOverrides.bEnableOverride && QualityOverrides.bForceNonMetal != 0 ? 1u : 0u);
	OutEnvironment.SetDefine(TEXT("QL_FORCEDISABLE_LM_DIRECTIONALITY"), QualityOverrides.bEnableOverride && QualityOverrides.bForceDisableLMDirectionality != 0 ? 1u : 0u);
	OutEnvironment.SetDefine(TEXT("MOBILE_QL_FORCE_LQ_REFLECTIONS"), QualityOverrides.bEnableOverride && QualityOverrides.bForceLQReflections != 0 ? 1u : 0u);
	OutEnvironment.SetDefine(TEXT("MOBILE_QL_FORCE_DISABLE_PREINTEGRATEDGF"), QualityOverrides.bEnableOverride && QualityOverrides.bForceDisablePreintegratedGF != 0 ? 1u : 0u);
	OutEnvironment.SetDefine(TEXT("MOBILE_SHADOW_QUALITY"), (uint32)QualityOverrides.MobileShadowQuality);
	OutEnvironment.SetDefine(TEXT("MOBILE_QL_DISABLE_MATERIAL_NORMAL"), QualityOverrides.bEnableOverride && QualityOverrides.bDisableMaterialNormalCalculation);
	return true;
}

typedef TTanGramBasePassPS< TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP> , 0> TTanGramBasePassPS_TG_LMP_NO_LIGHT_MAP_HDRLinear64;
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>, TTanGramBasePassPS_TG_LMP_NO_LIGHT_MAP_HDRLinear64, TEXT("/Engine/Private/TanGram/TanGramBasePassPixelShader.usf"), TEXT("Main"), SF_Pixel);

typedef TTanGramBasePassPS< TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP> , INT32_MAX> TTanGramBasePassPS_TG_LMP_NO_LIGHT_MAP_HDRLinear64_MAXPOINT;
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>, TTanGramBasePassPS_TG_LMP_NO_LIGHT_MAP_HDRLinear64_MAXPOINT, TEXT("/Engine/Private/TanGram/TanGramBasePassPixelShader.usf"), TEXT("Main"), SF_Pixel);

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

void  TTanGramBasePassVS::InternalDestroy(void* Object, const FTypeLayoutDesc&, const FPointerTableBase* PtrTable, bool bIsFrozen)
{
	Freeze::DestroyObject(static_cast<TTanGramBasePassVS*>(Object), PtrTable, bIsFrozen);
}

FTypeLayoutDesc& TTanGramBasePassVS::StaticGetTypeLayout()
{
	static_assert(TValidateInterfaceHelper<TTanGramBasePassVS,InterfaceType>::Value, "Invalid interface for " "TTanGramBasePassVSFNoLightMapPolicyHDRLinear64");
	alignas(FTypeLayoutDesc)static uint8 TypeBuffer[sizeof(FTypeLayoutDesc)] = { 0 };
	FTypeLayoutDesc& TypeDesc = *(FTypeLayoutDesc*)TypeBuffer;
	if (!TypeDesc.IsInitialized)
	{
		TypeDesc.IsInitialized = true;
		TypeDesc.Name = L"TTanGramBasePassVSFNoLightMapPolicyHDRLinear64";
		TypeDesc.WriteFrozenMemoryImageFunc = TGetFreezeImageHelper< TTanGramBasePassVS>::Do();
		TypeDesc.UnfrozenCopyFunc = &Freeze::DefaultUnfrozenCopy;
		TypeDesc.AppendHashFunc = &Freeze::DefaultAppendHash;
		TypeDesc.GetTargetAlignmentFunc = &Freeze::DefaultGetTargetAlignment;
		TypeDesc.ToStringFunc = &Freeze::DefaultToString;
		TypeDesc.DestroyFunc = &InternalDestroy;
		TypeDesc.Size = sizeof(TTanGramBasePassVS);
		TypeDesc.Alignment = alignof(TTanGramBasePassVS);
		TypeDesc.Interface = InterfaceType;
		TypeDesc.SizeFromFields = ~0u;
		TypeDesc.GetDefaultObjectFunc = &TGetDefaultObjectHelper< TTanGramBasePassVS,InterfaceType>::Do;
		InternalLinkType<1>::Initialize(TypeDesc);
		InternalInitializeBases<TTanGramBasePassVS>(TypeDesc);
		FTypeLayoutDesc::Initialize(TypeDesc);
	}
	return TypeDesc;
};


const FTypeLayoutDesc& TTanGramBasePassVS::GetTypeLayout() const
{
	return StaticGetTypeLayout();
}

TTanGramBasePassVS::ShaderMetaType TTanGramBasePassVS::StaticType
(
	TTanGramBasePassVS::StaticGetTypeLayout(),
	L"TTanGramBasePassVS<TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP>>",
	L"/Engine/Private/TanGram/TanGramBasePassVertexShader.usf",
	L"Main",
	SF_Vertex,
	TTanGramBasePassVS::FPermutationDomain::PermutationCount,
	TTanGramBasePassVS::ConstructSerializedInstance,
	TTanGramBasePassVS::ConstructCompiledInstance,
	TTanGramBasePassVS::ModifyCompilationEnvironmentImpl,
	TTanGramBasePassVS::ShouldCompilePermutationImpl,
	TTanGramBasePassVS::ValidateCompiledResult,
	sizeof(TTanGramBasePassVS),
	TTanGramBasePassVS::GetRootParametersMetadata()
);

typedef TTanGramBasePassPS< TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP> , 0> TTanGramBasePassPS_TG_LMP_NO_LIGHT_MAP_HDRLinear64;
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>, TTanGramBasePassPS_TG_LMP_NO_LIGHT_MAP_HDRLinear64, TEXT("/Engine/Private/TanGram/TanGramBasePassPixelShader.usf"), TEXT("Main"), SF_Pixel);

typedef TTanGramBasePassPS< TTanGramUniformLightMapPolicy<ETanGramLightMapPolicyType::TG_LMP_NO_LIGHTMAP> , INT32_MAX> TTanGramBasePassPS_TG_LMP_NO_LIGHT_MAP_HDRLinear64_MAXPOINT;
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>, TTanGramBasePassPS_TG_LMP_NO_LIGHT_MAP_HDRLinear64_MAXPOINT, TEXT("/Engine/Private/TanGram/TanGramBasePassPixelShader.usf"), TEXT("Main"), SF_Pixel);

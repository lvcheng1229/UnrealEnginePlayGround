#pragma once
#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "RenderResource.h"
#include "UniformBuffer.h"
#include "ShaderParameters.h"
#include "ShadowRendering.h"
#include "LightMap.h"

enum ETanGramLightMapPolicyType
{
	TG_LMP_NO_LIGHTMAP,
};

class RENDERER_API FTanGramUniformLightMapPolicy
{
public:
	using ElementDataType = const FLightCacheInterface*;
	FTanGramUniformLightMapPolicy(ETanGramLightMapPolicyType InIndirectPolicy) : IndirectPolicy(InIndirectPolicy) {}

	friend bool operator==(const FTanGramUniformLightMapPolicy A,const FTanGramUniformLightMapPolicy B)
	{
		return A.IndirectPolicy == B.IndirectPolicy;
	}

	ETanGramLightMapPolicyType GetIndirectPolicy() const { return IndirectPolicy; }
private:

	ETanGramLightMapPolicyType IndirectPolicy;
};


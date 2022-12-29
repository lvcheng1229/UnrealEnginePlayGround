//TanGram
#pragma once

#include "CoreMinimal.h"
#include "TanGramVertexAttribute.h"

class ENGINE_API FTanGramLocalVertexAttribute : public FTanGramVertexAttribute
{
public:
	FTanGramLocalVertexAttribute(ERHIFeatureLevel::Type InFeatureLevel, const char* InDebugName)
		:FTanGramVertexAttribute(InFeatureLevel)
		, DebugName(InDebugName)
	{

	}

	virtual void InitRHI() override;

	virtual void ReleaseRHI() override
	{
		FTanGramVertexAttribute::ReleaseRHI();
	}

	void SetData(const FStaticMeshDataType& InData);

protected:

	const FStaticMeshDataType& GetData() const { return TanGramVertexData; }

	FStaticMeshDataType TanGramVertexData;

	struct FDebugName
	{
		FDebugName(const char* InDebugName)
#if !UE_BUILD_SHIPPING
			: DebugName(InDebugName)
#endif
		{}
	private:
#if !UE_BUILD_SHIPPING
		const char* DebugName;
#endif
	} DebugName;
};
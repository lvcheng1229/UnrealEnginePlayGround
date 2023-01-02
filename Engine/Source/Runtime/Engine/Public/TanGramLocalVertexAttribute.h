//TanGram
#pragma once

#include "CoreMinimal.h"

#include "TanGramVertexAttribute.h"

class ENGINE_API FTanGramLocalVertexAttribute : public FTanGramVertexAttribute
{
public: 
	static FTanGramVertexAttributeType StaticType; 
	virtual FTanGramVertexAttributeType* GetType() const override;;
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

#if !UE_BUILD_SHIPPING
	struct FDebugName
	{
		FDebugName(const char* InDebugName)
		: DebugName(InDebugName){}
	private:
		const char* DebugName;
	} DebugName;
#else
	struct FDebugName
	{
		FDebugName(const char* InDebugName){}
	} DebugName;
#endif
};
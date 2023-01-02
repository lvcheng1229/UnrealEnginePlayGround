//TanGram
//TanGram Vertex Factory

#pragma once
#include "CoreMinimal.h"
#include "VertexFactory.h"

class ENGINE_API FTanGramVertexFactory : public FVertexFactory
{
public:
	
	FTanGramVertexFactory(ERHIFeatureLevel::Type InFeatureLevel , const char* InDebugName)
		: FVertexFactory(InFeatureLevel)
		, DebugName(InDebugName)

	{}

	virtual void InitRHI() override;

	virtual void ReleaseRHI() override
	{
		//UniformBuffer.SafeRelease();
		FVertexFactory::ReleaseRHI();
	}

	/**
	* An implementation of the interface used by TSynchronizedResource to update the resource with new data from the game thread.
	*/
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
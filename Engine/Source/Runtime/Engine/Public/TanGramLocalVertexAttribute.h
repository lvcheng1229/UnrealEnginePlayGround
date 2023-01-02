//TanGram
#pragma once

#include "CoreMinimal.h"
#include "TanGramVertexAttribute.h"

class ENGINE_API FTanGramLocalVertexAttribute : public FTanGramVertexAttribute
{
public:
	FTanGramLocalVertexAttribute(ERHIFeatureLevel::Type InFeatureLevel)
		:FTanGramVertexAttribute(InFeatureLevel)
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
};
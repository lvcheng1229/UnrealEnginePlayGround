//TanGram

#include "TanGramLocalVertexAttribute.h"

/*
 * Inti RHI Vertex Attributes : Position Only / Position And Normal / Default
 * Position Only : Attribute Index 0
 * Position And Normal : Position - Attribute 0 , Normal Attribute 2
 * Default : Position Attribute 0 / TangentX Attribute 1 /Tangent Z Attribute 2 / Texture Coord 0 - 4 : Attribute 4 - 8
 */

FTanGramVertexAttributeType* FTanGramLocalVertexAttribute::GetType() const
{
	return nullptr;
}

void FTanGramLocalVertexAttribute::InitRHI()
{
	ensure((TanGramVertexData.PositionComponent.VertexBuffer != nullptr));
	ensure(TanGramVertexData.PositionComponent.VertexBuffer != TanGramVertexData.TangentBasisComponents[0].VertexBuffer);

	//Init Position Only Declaration
	FVertexDeclarationElementList PositionStreamElements;
	PositionStreamElements.Add(CreateVertexElementAndVertexStream(TanGramVertexData.PositionComponent,0,EVertexInputStreamType::PositionOnly));
	InitDeclaration(PositionStreamElements,EVertexInputStreamType::PositionOnly);

	//Init Position And Normal Decalaration
	FVertexDeclarationElementList PositionAndNormalStreamElements;
	PositionAndNormalStreamElements.Add(CreateVertexElementAndVertexStream(TanGramVertexData.PositionComponent,0,EVertexInputStreamType::PositionAndNormalOnly));
	if(TanGramVertexData.TangentBasisComponents[1].VertexBuffer!=nullptr)
	{
		PositionAndNormalStreamElements.Add(CreateVertexElementAndVertexStream(TanGramVertexData.TangentBasisComponents[1],2,EVertexInputStreamType::PositionAndNormalOnly));
	}
	InitDeclaration(PositionAndNormalStreamElements,EVertexInputStreamType::PositionAndNormalOnly);

	//Init Default Decalration
	FVertexDeclarationElementList DefaultStreamElements;
	DefaultStreamElements.Add(CreateVertexElementAndVertexStream(TanGramVertexData.PositionComponent,0,EVertexInputStreamType::Default));

	DefaultStreamElements.Add(CreateVertexElementAndVertexStream(TanGramVertexData.TangentBasisComponents[0],1,EVertexInputStreamType::Default));
	DefaultStreamElements.Add(CreateVertexElementAndVertexStream(TanGramVertexData.TangentBasisComponents[1],2,EVertexInputStreamType::Default));

	if (TanGramVertexData.TextureCoordinates.Num())
	{
		const int32 BaseTexCoordAttribute = 4;
		for (int32 CoordinateIndex = 0; CoordinateIndex < TanGramVertexData.TextureCoordinates.Num(); ++CoordinateIndex)
		{
			DefaultStreamElements.Add(CreateVertexElementAndVertexStream(TanGramVertexData.TextureCoordinates[CoordinateIndex],BaseTexCoordAttribute + CoordinateIndex,EVertexInputStreamType::Default));
		}

		for (int32 CoordinateIndex = TanGramVertexData.TextureCoordinates.Num(); CoordinateIndex < MAX_STATIC_TEXCOORDS / 2; ++CoordinateIndex)
		{
			DefaultStreamElements.Add(CreateVertexElementAndVertexStream(TanGramVertexData.TextureCoordinates[TanGramVertexData.TextureCoordinates.Num() - 1],BaseTexCoordAttribute + CoordinateIndex,EVertexInputStreamType::Default));
		}
	}
	InitDeclaration(DefaultStreamElements,EVertexInputStreamType::Default);
}

void FTanGramLocalVertexAttribute::SetData(const FStaticMeshDataType& InData)
{
	check(IsInRenderingThread());
	check((InData.ColorComponent.Type == VET_None) || (InData.ColorComponent.Type == VET_Color));
	TanGramVertexData = InData;
	UpdateRHI();
}

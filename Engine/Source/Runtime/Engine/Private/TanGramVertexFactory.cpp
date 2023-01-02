//TanGram
//TanGram VertexFacoty.cpp

#include "TanGramVertexFactory.h"


//#include "LocalVertexFactory.h"
#include "SceneView.h"
#include "MeshBatch.h"
#include "SpeedTreeWind.h"
#include "ShaderParameterUtils.h"
#include "Rendering/ColorVertexBuffer.h"
#include "MeshMaterialShader.h"
#include "ProfilingDebugging/LoadTimeTracker.h"

void FTanGramVertexFactory::InitRHI()
{
	//Init Three Vertex Declaration
	// 1. PositionOnly 2.PositionAndNormal 3. Position Tangent * 2  UV
	
	if (TanGramVertexData.PositionComponent.VertexBuffer != TanGramVertexData.TangentBasisComponents[0].VertexBuffer)
	{
		auto AddDeclaration = [this](EVertexInputStreamType InputStreamType, bool bAddNormal)
		{
			FVertexDeclarationElementList StreamElements;
			StreamElements.Add(AccessStreamComponent(TanGramVertexData.PositionComponent, 0, InputStreamType));

			bAddNormal = bAddNormal && TanGramVertexData.TangentBasisComponents[1].VertexBuffer != nullptr;
			if (bAddNormal)
			{
				StreamElements.Add(AccessStreamComponent(TanGramVertexData.TangentBasisComponents[1], 2, InputStreamType));
			}

			//don't support gpu scene for now
			//AddPrimitiveIdStreamElement(InputStreamType, StreamElements, 1, 8);

			InitDeclaration(StreamElements, InputStreamType);
		};

		AddDeclaration(EVertexInputStreamType::PositionOnly, false);
		AddDeclaration(EVertexInputStreamType::PositionAndNormalOnly, true);
	}

	FVertexDeclarationElementList Elements;

	//Attribute0 : Position
	if (TanGramVertexData.PositionComponent.VertexBuffer != nullptr)
	{
		Elements.Add(AccessStreamComponent(TanGramVertexData.PositionComponent, 0));
	}

	//don't support gpu scene for now
	//AddPrimitiveIdStreamElement(EVertexInputStreamType::Default, Elements, 13, 8);

	//Attribute 1/2 : TangentX/Y 
	uint8 TangentBasisAttributes[2] = { 1, 2 };
	for (int32 AxisIndex = 0; AxisIndex < 2; AxisIndex++)
	{
		if (TanGramVertexData.TangentBasisComponents[AxisIndex].VertexBuffer != nullptr)
		{
			Elements.Add(AccessStreamComponent(TanGramVertexData.TangentBasisComponents[AxisIndex], TangentBasisAttributes[AxisIndex]));
		}
	}

	//Attribute [3,6] : TextureCoords
	if (TanGramVertexData.TextureCoordinates.Num())
	{
		const int32 BaseTexCoordAttribute = 4;
		for (int32 CoordinateIndex = 0; CoordinateIndex < TanGramVertexData.TextureCoordinates.Num(); ++CoordinateIndex)
		{
			Elements.Add(AccessStreamComponent(
				TanGramVertexData.TextureCoordinates[CoordinateIndex],
				BaseTexCoordAttribute + CoordinateIndex
				));
		}

		for (int32 CoordinateIndex = TanGramVertexData.TextureCoordinates.Num(); CoordinateIndex < MAX_STATIC_TEXCOORDS / 2; ++CoordinateIndex)
		{
			Elements.Add(AccessStreamComponent(
				TanGramVertexData.TextureCoordinates[TanGramVertexData.TextureCoordinates.Num() - 1],
				BaseTexCoordAttribute + CoordinateIndex
				));
		}
	}
	
	check(Streams.Num() > 0);
	InitDeclaration(Elements);
	check(IsValidRef(GetDeclaration()));
}

void FTanGramVertexFactory::SetData(const FStaticMeshDataType& InData)
{
	TanGramVertexData = InData;
}

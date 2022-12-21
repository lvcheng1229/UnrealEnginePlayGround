//TanGram

#include "TanGramVertexAttribute.h"
#include "Serialization/MemoryWriter.h"
#include "UObject/DebugSerializationFlags.h"
#include "PipelineStateCache.h"
#include "ShaderCompilerCore.h"
#include "RenderUtils.h"

void FTanGramVertexAttribute::InitDeclaration(const FVertexDeclarationElementList& Elements,
	EVertexInputStreamType StreamType)
{
	if (StreamType == EVertexInputStreamType::PositionOnly)
	{
		PositionDeclaration = PipelineStateCache::GetOrCreateVertexDeclaration(Elements);
	}
	else if (StreamType == EVertexInputStreamType::PositionAndNormalOnly)
	{
		PositionAndNormalDeclaration = PipelineStateCache::GetOrCreateVertexDeclaration(Elements);
	}
	else 
	{
		Declaration = PipelineStateCache::GetOrCreateVertexDeclaration(Elements);
	}
}

FVertexElement FTanGramVertexAttribute::CreateVertexElementAndVertexStream(const FVertexStreamComponent& Component,uint8 AttributeIndex,EVertexInputStreamType StreamType)
{
	if(StreamType == EVertexInputStreamType::PositionOnly)
	{
		return FVertexElement(uint8(PositionStream.AddUnique(Component)),Component.Offset,Component.Type,AttributeIndex,Component.Stride,EnumHasAnyFlags(EVertexStreamUsage::Instancing, Component.VertexStreamUsage));
	}
	else if(StreamType == EVertexInputStreamType::PositionAndNormalOnly)
	{
		return FVertexElement(uint8(PositionAndNormalStream.AddUnique(Component)),Component.Offset,Component.Type,AttributeIndex,Component.Stride,EnumHasAnyFlags(EVertexStreamUsage::Instancing, Component.VertexStreamUsage));
	}
	else
	{
		return FVertexElement(uint8(Streams.AddUnique(Component)),Component.Offset,Component.Type,AttributeIndex,Component.Stride,EnumHasAnyFlags(EVertexStreamUsage::Instancing, Component.VertexStreamUsage));
	}
}

//TanGram
#pragma once
#include "CoreMinimal.h"

enum class EVertexStreamUsage : uint8
{
	Default			= 0 << 0,
	Instancing		= 1 << 0,
	Overridden		= 1 << 1,
	ManualFetch		= 1 << 2
};
ENUM_CLASS_FLAGS(EVertexStreamUsage);

enum class EVertexInputStreamType : uint8
{
	Default = 0,
	PositionOnly,
	PositionAndNormalOnly,
	Count
};

enum class EVertexFactoryFlags : uint32
{
	None                                  = 0u,
	UsedWithMaterials                     = 1u << 1,
	SupportsStaticLighting                = 1u << 2,
	SupportsDynamicLighting               = 1u << 3,
	SupportsPrecisePrevWorldPos           = 1u << 4,
	SupportsPositionOnly                  = 1u << 5,
	SupportsCachingMeshDrawCommands       = 1u << 6,
	SupportsPrimitiveIdStream             = 1u << 7,
	SupportsNaniteRendering               = 1u << 8,
	SupportsRayTracing                    = 1u << 9,
	SupportsRayTracingDynamicGeometry     = 1u << 10,
	SupportsRayTracingProceduralPrimitive = 1u << 11,
};
ENUM_CLASS_FLAGS(EVertexFactoryFlags);

/**
 * A typed data source for a vertex factory which streams data from a vertex buffer.
 */
struct FVertexStreamComponent
{
	/** The vertex buffer to stream data from.  If null, no data can be read from this stream. */
	const FVertexBuffer* VertexBuffer = nullptr;

	/** The offset to the start of the vertex buffer fetch. */
	uint32 StreamOffset = 0;

	/** The offset of the data, relative to the beginning of each element in the vertex buffer. */
	uint8 Offset = 0;

	/** The stride of the data. */
	uint8 Stride = 0;

	/** The type of the data read from this stream. */
	TEnumAsByte<EVertexElementType> Type = VET_None;

	EVertexStreamUsage VertexStreamUsage = EVertexStreamUsage::Default;

	/**
	 * Initializes the data stream to null.
	 */
	FVertexStreamComponent()
	{}

	/**
	 * Minimal initialization constructor.
	 */
	FVertexStreamComponent(const FVertexBuffer* InVertexBuffer, uint32 InOffset, uint32 InStride, EVertexElementType InType, EVertexStreamUsage Usage = EVertexStreamUsage::Default) :
		VertexBuffer(InVertexBuffer),
		StreamOffset(0),
		Offset((uint8)InOffset),
		Stride((uint8)InStride),
		Type(InType),
		VertexStreamUsage(Usage)
	{
		check(InStride <= 0xFF);
		check(InOffset <= 0xFF);
	}

	FVertexStreamComponent(const FVertexBuffer* InVertexBuffer, uint32 InStreamOffset, uint32 InOffset, uint32 InStride, EVertexElementType InType, EVertexStreamUsage Usage = EVertexStreamUsage::Default) :
		VertexBuffer(InVertexBuffer),
		StreamOffset(InStreamOffset),
		Offset((uint8)InOffset),
		Stride((uint8)InStride),
		Type(InType),
		VertexStreamUsage(Usage)
	{
		check(InStride <= 0xFF);
		check(InOffset <= 0xFF);
	}

	//TanGram BEGIN
	friend bool operator==(const FVertexStreamComponent& A,const FVertexStreamComponent& B)
	{
		return A.VertexBuffer == B.VertexBuffer && A.Stride == B.Stride && A.Offset == B.Offset && A.VertexStreamUsage == B.VertexStreamUsage;
	}
	//TanGram END
};

/**
 * A macro which initializes a FVertexStreamComponent to read a member from a struct.
 */
#define STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,VertexType,Member,MemberType) \
	FVertexStreamComponent(VertexBuffer,STRUCT_OFFSET(VertexType,Member),sizeof(VertexType),MemberType)
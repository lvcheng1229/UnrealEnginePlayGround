//TanGram
//TanGramVertexAttribute

#pragma once
#include "CoreMinimal.h"
#include "Containers/List.h"
#include "Misc/SecureHash.h"
#include "RHI.h"
#include "RenderResource.h"
#include "ShaderCore.h"
#include "Shader.h"
#include "Misc/EnumClassFlags.h"
#include "VertexCommon.h"

struct FVertexFactoryShaderPermutationParameters;

class FTanGramVertexAttributeType
{
public:
	RENDERCORE_API FTanGramVertexAttributeType(
	const TCHAR* InName,
	const TCHAR* InShaderFilename);

	const TCHAR* GetName() const { return Name; }
	FName GetFName() const { return TypeName; }
	const FHashedName& GetHashedName() const { return HashedName; }
	const TCHAR* GetShaderFilename() const { return ShaderFilename; }

	/** Calculates a Hash based on this vertex attribute type's source code and includes */
	RENDERCORE_API const FSHAHash& GetSourceHash(EShaderPlatform ShaderPlatform) const;
	
	static RENDERCORE_API const TArray<FTanGramVertexAttributeType*>& GetSortedMaterialTypes();
private:
	const TCHAR* Name;
	const TCHAR* ShaderFilename;
	FName TypeName;
	FHashedName HashedName;
};

class RENDERCORE_API FTanGramVertexAttribute : public FRenderResource
{
public:
	FTanGramVertexAttribute(ERHIFeatureLevel::Type InFeatureLevel)
		: FRenderResource(InFeatureLevel)
	{
		
	}

	virtual FTanGramVertexAttributeType* GetType() const { return nullptr; }
	
	const FVertexDeclarationRHIRef& GetDeclaration(EVertexInputStreamType InputStreamType) const 
	{
		switch (InputStreamType)
		{
		case EVertexInputStreamType::Default:				return Declaration;
		case EVertexInputStreamType::PositionOnly:			return PositionDeclaration;
		case EVertexInputStreamType::PositionAndNormalOnly:	return PositionAndNormalDeclaration;
		default:ensure(false);return Declaration;
		}
	}

	TArray<FVertexStreamComponent,TInlineAllocator<8> > Streams;
protected:
	TArray<FVertexStreamComponent,TInlineAllocator<2> > PositionStream;
	TArray<FVertexStreamComponent, TInlineAllocator<3> > PositionAndNormalStream;
	
	void InitDeclaration(const FVertexDeclarationElementList& Elements, EVertexInputStreamType StreamType = EVertexInputStreamType::Default);
	FVertexElement CreateVertexElementAndVertexStream(const FVertexStreamComponent& Component, uint8 AttributeIndex , EVertexInputStreamType StreamType = EVertexInputStreamType::Default);
private:
	FVertexDeclarationRHIRef Declaration;

	FVertexDeclarationRHIRef PositionDeclaration;
	FVertexDeclarationRHIRef PositionAndNormalDeclaration;
};
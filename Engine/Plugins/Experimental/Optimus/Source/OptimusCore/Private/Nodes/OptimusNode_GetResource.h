// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Nodes/OptimusNode_ResourceAccessorBase.h"

#include "OptimusNode_GetResource.generated.h"


class UOptimusResourceDescription;


UCLASS(Hidden)
class UOptimusNode_GetResource
	: public UOptimusNode_ResourceAccessorBase
{
	GENERATED_BODY()

protected:
	void ConstructNode() override;
};

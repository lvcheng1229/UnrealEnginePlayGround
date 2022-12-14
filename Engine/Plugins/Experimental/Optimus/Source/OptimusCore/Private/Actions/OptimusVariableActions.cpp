// Copyright Epic Games, Inc. All Rights Reserved.

#include "OptimusVariableActions.h"

#include "OptimusDeformer.h"
#include "OptimusHelpers.h"
#include "OptimusVariableDescription.h"


FOptimusVariableAction_AddVariable::FOptimusVariableAction_AddVariable(
	UOptimusDeformer* InDeformer, 
	FOptimusDataTypeRef InDataType, 
	FName InName
	)
{
	if (ensure(InDeformer))
	{
		// FIXME: Validate name?
		VariableName = Optimus::GetUniqueNameForScopeAndClass(InDeformer, UOptimusVariableDescription::StaticClass(), InName);
		DataType = InDataType;

		SetTitlef(TEXT("Add variable '%s'"), *VariableName.ToString());
	}
}


UOptimusVariableDescription* FOptimusVariableAction_AddVariable::GetVariable(
	IOptimusPathResolver* InRoot
	) const
{
	return Cast<UOptimusDeformer>(InRoot)->ResolveVariable(VariableName);
}


bool FOptimusVariableAction_AddVariable::Do(IOptimusPathResolver* InRoot)
{
	UOptimusDeformer* Deformer = Cast<UOptimusDeformer>(InRoot);

	UOptimusVariableDescription* Variable = Deformer->CreateVariableDirect(VariableName);
	if (!Variable)
	{
		return false;
	}

	// The name should not have changed.
	check(Variable->GetFName() == VariableName);


	Variable->VariableName = Variable->GetFName();
	Variable->DataType = DataType;

	if (!Deformer->AddVariableDirect(Variable))
	{
		Variable->Rename(nullptr, GetTransientPackage());
		return false;
	}
	
	VariableName = Variable->GetFName();
	return true;
}


bool FOptimusVariableAction_AddVariable::Undo(IOptimusPathResolver* InRoot)
{
	UOptimusVariableDescription* Variable = GetVariable(InRoot);
	if (!Variable)
	{
		return false;
	}

	UOptimusDeformer* Deformer = Cast<UOptimusDeformer>(InRoot);
	return Deformer->RemoveVariableDirect(Variable);
}


FOptimusVariableAction_RemoveVariable::FOptimusVariableAction_RemoveVariable(
	UOptimusVariableDescription* InVariable
	)
{
	if (ensure(InVariable))
	{
		VariableName = InVariable->GetFName();
		DataType = InVariable->DataType;

		SetTitlef(TEXT("Remove variable '%s'"), *InVariable->GetName());
	}
}


bool FOptimusVariableAction_RemoveVariable::Do(IOptimusPathResolver* InRoot)
{
	UOptimusDeformer* Deformer = Cast<UOptimusDeformer>(InRoot);

	UOptimusVariableDescription* Variable = Deformer->ResolveVariable(VariableName);
	if (!Variable)
	{
		return false;
	}

	{
		Optimus::FBinaryObjectWriter VarArchive(Variable, VariableData);
	}

	return Deformer->RemoveVariableDirect(Variable);
}


bool FOptimusVariableAction_RemoveVariable::Undo(IOptimusPathResolver* InRoot)
{
	UOptimusDeformer* Deformer = Cast<UOptimusDeformer>(InRoot);

	UOptimusVariableDescription* Variable = Deformer->CreateVariableDirect(VariableName);
	if (!Variable)
	{
		return false;
	}

	// The names should match since the name should have remained unique.
	check(Variable->GetFName() == VariableName);

	// Fill in the stored data
	{
		Optimus::FBinaryObjectReader VarArchive(Variable, VariableData);
	}

	if (!Deformer->AddVariableDirect(Variable))
	{
		Variable->Rename(nullptr, GetTransientPackage());
		return false;
	}
	
	return true;
}


FOptimusVariableAction_RenameVariable::FOptimusVariableAction_RenameVariable(
	UOptimusVariableDescription* InVariable, 
	FName InNewName
	)
{
	if (ensure(InVariable))
	{
		UOptimusDeformer* Deformer = Cast<UOptimusDeformer>(InVariable->GetOuter());

		OldName = InVariable->GetFName();
		NewName = Optimus::GetUniqueNameForScopeAndClass(Deformer, UOptimusVariableDescription::StaticClass(), InNewName);

		SetTitlef(TEXT("Rename variable to '%s'"), *NewName.ToString());
	}
}


bool FOptimusVariableAction_RenameVariable::Do(
	IOptimusPathResolver* InRoot
	)
{
	UOptimusDeformer* Deformer = Cast<UOptimusDeformer>(InRoot);
	UOptimusVariableDescription* Variable = Deformer->ResolveVariable(OldName);

	return Variable && Deformer->RenameVariableDirect(Variable, NewName);
}


bool FOptimusVariableAction_RenameVariable::Undo(
	IOptimusPathResolver* InRoot
	)
{
	UOptimusDeformer* Deformer = Cast<UOptimusDeformer>(InRoot);
	UOptimusVariableDescription* Variable = Deformer->ResolveVariable(NewName);

	return Variable && Deformer->RenameVariableDirect(Variable, OldName);
}

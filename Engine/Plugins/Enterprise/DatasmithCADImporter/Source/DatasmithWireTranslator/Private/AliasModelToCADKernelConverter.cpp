// Copyright Epic Games, Inc. All Rights Reserved.

#include "AliasModelToCADKernelConverter.h"

#include "CADData.h"
#include "CADKernelTools.h"
#include "Hal/PlatformMemory.h"
#include "MeshDescriptionHelper.h"

#include "CADKernel/Core/Session.h"
#include "CADKernel/Geo/Curves/NURBSCurve.h"
#include "CADKernel/Geo/GeoEnum.h"
#include "CADKernel/Geo/Surfaces/NURBSSurface.h"
#include "CADKernel/Geo/Surfaces/Surface.h"

#include "CADKernel/Math/Point.h"
#include "CADKernel/Mesh/Meshers/ParametricMesher.h"
#include "CADKernel/Mesh/Structure/ModelMesh.h"

#include "CADKernel/Topo/Body.h"
#include "CADKernel/Topo/Model.h"
#include "CADKernel/Topo/Shell.h"
#include "CADKernel/Topo/TopologicalEdge.h"
#include "CADKernel/Topo/TopologicalFace.h"
#include "CADKernel/Topo/TopologicalLoop.h"

#ifdef USE_OPENMODEL

// Alias API wrappes object in AlObjects. This is an abstract base class which holds a reference to an anonymous data structure.
// The only way to compare two AlObjects is to compare their data structure. That is the reason why private fields are made public. 
#define private public
#include "AlCurve.h"
#include "AlDagNode.h"
#include "AlShell.h"
#include "AlShellNode.h"
#include "AlSurface.h"
#include "AlSurfaceNode.h"
#include "AlTrimBoundary.h"
#include "AlTrimCurve.h"
#include "AlTrimRegion.h"
#include "AlTM.h"

namespace UE_DATASMITHWIRETRANSLATOR_NAMESPACE
{

namespace AliasToCADKernelUtils
{

	template<typename Surface_T>
	TSharedPtr<CADKernel::FSurface> AddNURBSSurface(double GeometricTolerance, Surface_T& AliasSurface, EAliasObjectReference InObjectReference, const AlMatrix4x4& InAlMatrix)
	{
		CADKernel::FNurbsSurfaceHomogeneousData NURBSData;
		NURBSData.bSwapUV= true;
		NURBSData.bIsRational = true;

		NURBSData.PoleUCount = AliasSurface.uNumberOfCVsInclMultiples();
		NURBSData.PoleVCount = AliasSurface.vNumberOfCVsInclMultiples();

		NURBSData.UDegree = AliasSurface.uDegree();  // U order of the surface
		NURBSData.VDegree = AliasSurface.vDegree();  // V order of the surface

		int32 KnotSizeU = AliasSurface.realuNumberOfKnots() + 2;
		int32 KnotSizeV = AliasSurface.realvNumberOfKnots() + 2;

		NURBSData.UNodalVector.SetNumUninitialized(KnotSizeU);
		NURBSData.VNodalVector.SetNumUninitialized(KnotSizeV);

		AliasSurface.realuKnotVector(NURBSData.UNodalVector.GetData() + 1);
		AliasSurface.realvKnotVector(NURBSData.VNodalVector.GetData() + 1);

		NURBSData.UNodalVector[0] = NURBSData.UNodalVector[1];
		NURBSData.UNodalVector[KnotSizeU - 1] = NURBSData.UNodalVector[KnotSizeU - 2];
		NURBSData.VNodalVector[0] = NURBSData.VNodalVector[1];
		NURBSData.VNodalVector[KnotSizeV - 1] = NURBSData.VNodalVector[KnotSizeV - 2];

		const int32 CoordinateCount = NURBSData.PoleUCount * NURBSData.PoleVCount * 4;
		NURBSData.HomogeneousPoles.SetNumUninitialized(CoordinateCount);

		if (InObjectReference == EAliasObjectReference::WorldReference)
		{
			AliasSurface.CVsWorldPositionInclMultiples(NURBSData.HomogeneousPoles.GetData());
		}
		else if (InObjectReference == EAliasObjectReference::ParentReference)
		{
			AlTM TranformMatrix(InAlMatrix);
			AliasSurface.CVsAffectedPositionInclMultiples(TranformMatrix, NURBSData.HomogeneousPoles.GetData());
		}
		else  // EAliasObjectReference::LocalReference
		{
			AliasSurface.CVsUnaffectedPositionInclMultiples(NURBSData.HomogeneousPoles.GetData());
		}

		// convert cm to mm
		double* Poles = NURBSData.HomogeneousPoles.GetData();
		for (int32 Index = 0; Index < NURBSData.HomogeneousPoles.Num(); Index += 4)
		{
			Poles[Index + 0] *= 10.;
			Poles[Index + 1] *= 10.;
			Poles[Index + 2] *= 10.;
		}

		return CADKernel::FEntity::MakeShared<CADKernel::FNURBSSurface>(GeometricTolerance, NURBSData);
	}
}

TSharedPtr<CADKernel::FTopologicalEdge> FAliasModelToCADKernelConverter::AddEdge(const AlTrimCurve& AliasTrimCurve, TSharedPtr<CADKernel::FSurface>& CarrierSurface)
{
	CADKernel::FNurbsCurveData NurbsCurveData;

	NurbsCurveData.Degree = AliasTrimCurve.degree();
	int32 ControlPointCount = AliasTrimCurve.numberOfCVs();

	NurbsCurveData.Dimension = 2;
	NurbsCurveData.bIsRational = true;

	int32 KnotCount = AliasTrimCurve.realNumberOfKnots() + 2;

	NurbsCurveData.Weights.SetNumUninitialized(ControlPointCount);
	NurbsCurveData.Poles.SetNumUninitialized(ControlPointCount);
	NurbsCurveData.NodalVector.SetNumUninitialized(KnotCount);

	using AlPoint = double[3];
	// Notice that each CV has three coordinates - the three coordinates describe 2D parameter space, with a homogeneous coordinate.
	// Each control point is u, v and w, where u and v are parameter space and w is the homogeneous coordinate.
	AliasTrimCurve.CVsUVPosition(NurbsCurveData.NodalVector.GetData() + 1, (AlPoint*) NurbsCurveData.Poles.GetData());

	AliasTrimCurve.realKnotVector(NurbsCurveData.NodalVector.GetData() + 1);
	NurbsCurveData.NodalVector[0] = NurbsCurveData.NodalVector[1];
	NurbsCurveData.NodalVector[KnotCount - 1] = NurbsCurveData.NodalVector[KnotCount - 2];

	for (int32 Index = 0; Index < ControlPointCount; ++Index)
	{
		NurbsCurveData.Weights[Index] = NurbsCurveData.Poles[Index].Z;
		NurbsCurveData.Poles[Index].Z = 0;
	}

	TSharedRef<CADKernel::FNURBSCurve> Nurbs = CADKernel::FEntity::MakeShared<CADKernel::FNURBSCurve>(NurbsCurveData);

	TSharedRef<CADKernel::FRestrictionCurve> RestrictionCurve = CADKernel::FEntity::MakeShared<CADKernel::FRestrictionCurve>(CarrierSurface.ToSharedRef(), Nurbs);
	TSharedPtr<CADKernel::FTopologicalEdge> Edge = CADKernel::FTopologicalEdge::Make(RestrictionCurve);
	if (!Edge.IsValid())
	{
		return TSharedPtr<CADKernel::FTopologicalEdge>();
	}

	// Only TrimCurve with twin need to be in the map used in LinkEdgesLoop 
	TUniquePtr<AlTrimCurve> TwinCurve(AliasTrimCurve.getTwinCurve());
	if (TwinCurve.IsValid())
	{
		AlEdge2CADKernelEdge.Add(AliasTrimCurve.fSpline, Edge);
	}

	return Edge;
}

TSharedPtr<CADKernel::FTopologicalLoop> FAliasModelToCADKernelConverter::AddLoop(const AlTrimBoundary& TrimBoundary, TSharedPtr<CADKernel::FSurface>& CarrierSurface, const bool bIsExternal)
{
	TArray<TSharedPtr<CADKernel::FTopologicalEdge>> Edges;
	TArray<CADKernel::EOrientation> Directions;

	for (TUniquePtr<AlTrimCurve> TrimCurve(TrimBoundary.firstCurve()); TrimCurve.IsValid(); TrimCurve = TUniquePtr<AlTrimCurve>(TrimCurve->nextCurve()))
	{
		TSharedPtr<CADKernel::FTopologicalEdge> Edge = AddEdge(*TrimCurve, CarrierSurface);
		if (Edge.IsValid())
		{
			Edges.Add(Edge);
			Directions.Emplace(CADKernel::EOrientation::Front);
		}
	}

	if (Edges.Num() == 0)
	{
		return TSharedPtr<CADKernel::FTopologicalLoop>();
	}

	TSharedPtr<CADKernel::FTopologicalLoop> Loop = CADKernel::FTopologicalLoop::Make(Edges, Directions, GeometricTolerance);
	if (!bIsExternal)
	{
		Loop->SetAsInnerBoundary();
	}
	return Loop;
}

void FAliasModelToCADKernelConverter::LinkEdgesLoop(const AlTrimBoundary& TrimBoundary, CADKernel::FTopologicalLoop& Loop)
{
	for (TUniquePtr<AlTrimCurve> TrimCurve(TrimBoundary.firstCurve()); TrimCurve.IsValid(); TrimCurve = TUniquePtr<AlTrimCurve>(TrimCurve->nextCurve()))
	{
		TSharedPtr<CADKernel::FTopologicalEdge>* Edge = AlEdge2CADKernelEdge.Find(TrimCurve->fSpline);
		if (!Edge || !Edge->IsValid() || (*Edge)->IsDeleted() || (*Edge)->IsDegenerated())
		{
			continue;
		}

		ensure(&Loop == (*Edge)->GetLoop());

		// Link edges
		TUniquePtr<AlTrimCurve> TwinCurve(TrimCurve->getTwinCurve());
		if (TwinCurve.IsValid())
		{
			if (TSharedPtr<CADKernel::FTopologicalEdge>* TwinEdge = AlEdge2CADKernelEdge.Find(TwinCurve->fSpline))
			{
				if (TwinEdge->IsValid() && !(*TwinEdge)->IsDeleted() && !(*TwinEdge)->IsDegenerated())
				{
					(*Edge)->Link(**TwinEdge, SquareTolerance);
				}
			}
		}
	}
}

TSharedPtr<CADKernel::FTopologicalFace> FAliasModelToCADKernelConverter::AddTrimRegion(const AlTrimRegion& TrimRegion, EAliasObjectReference InObjectReference, const AlMatrix4x4& InAlMatrix, bool bInOrientation)
{
	TSharedPtr<CADKernel::FSurface> Surface = AliasToCADKernelUtils::AddNURBSSurface(GeometricTolerance, TrimRegion, InObjectReference, InAlMatrix);
	if (!Surface.IsValid())
	{
		return TSharedPtr<CADKernel::FTopologicalFace>();
	}

	bool bIsExternal = true;
	TArray<TSharedPtr<CADKernel::FTopologicalLoop>> Loops;
	for (TUniquePtr<AlTrimBoundary> TrimBoundary(TrimRegion.firstBoundary()); TrimBoundary.IsValid(); TrimBoundary = TUniquePtr<AlTrimBoundary>(TrimBoundary->nextBoundary()))
	{
		TSharedPtr<CADKernel::FTopologicalLoop> Loop = AddLoop(*TrimBoundary, Surface, bIsExternal);
		if (Loop.IsValid())
		{
			LinkEdgesLoop(*TrimBoundary, *Loop);
			Loops.Add(Loop);
			bIsExternal = false;
		}
	}

	if (Loops.Num() == 0)
	{
		CADKernel::FMessage::Printf(CADKernel::EVerboseLevel::Log, TEXT("The Face %s is degenerate, this face is ignored\n"), TrimRegion.name());
		return TSharedPtr<CADKernel::FTopologicalFace>();
	}

	TSharedRef<CADKernel::FTopologicalFace> Face = CADKernel::FEntity::MakeShared<CADKernel::FTopologicalFace>(Surface);
	Face->SetPatchId(LastFaceId++);

	int32 DoubtfulLoopOrientationCount = 0;
	Face->AddLoops(Loops, DoubtfulLoopOrientationCount);
	return Face;
}

void FAliasModelToCADKernelConverter::AddFace(const AlSurface& Surface, EAliasObjectReference InObjectReference, const AlMatrix4x4& InAlMatrix, bool bInOrientation, TSharedRef<CADKernel::FShell>& Shell)
{
	TUniquePtr<AlTrimRegion> TrimRegion(Surface.firstTrimRegion());
	if (TrimRegion.IsValid())
	{
		for (; TrimRegion.IsValid(); TrimRegion = TUniquePtr<AlTrimRegion>(TrimRegion->nextRegion()))
		{
			TSharedPtr<CADKernel::FTopologicalFace> Face = AddTrimRegion(*TrimRegion, InObjectReference, InAlMatrix, bInOrientation);
			if (Face.IsValid())
			{
				Shell->Add(Face.ToSharedRef(), bInOrientation ? CADKernel::EOrientation::Front : CADKernel::EOrientation::Back);
			}
		}
		return;
	}

	TSharedPtr<CADKernel::FSurface> CADKernelSurface = AliasToCADKernelUtils::AddNURBSSurface(GeometricTolerance, Surface, InObjectReference, InAlMatrix);
	if (CADKernelSurface.IsValid())
	{
		TSharedRef<CADKernel::FTopologicalFace> Face = CADKernel::FEntity::MakeShared<CADKernel::FTopologicalFace>(CADKernelSurface);
		Face->ApplyNaturalLoops();
		Shell->Add(Face, bInOrientation ? CADKernel::EOrientation::Front : CADKernel::EOrientation::Back);
	}
}

void FAliasModelToCADKernelConverter::AddShell(const AlShell& InShell, EAliasObjectReference InObjectReference, const AlMatrix4x4& InAlMatrix, bool bInOrientation, TSharedRef<CADKernel::FShell>& CADKernelShell)
{
	for(TUniquePtr<AlTrimRegion> TrimRegion(InShell.firstTrimRegion()); TrimRegion.IsValid(); TrimRegion = TUniquePtr<AlTrimRegion>(TrimRegion->nextRegion()))
	{
		TSharedPtr<CADKernel::FTopologicalFace> Face = AddTrimRegion(*TrimRegion, InObjectReference, InAlMatrix, bInOrientation);
		if (Face.IsValid())
		{
			CADKernelShell->Add(Face.ToSharedRef(), bInOrientation ? CADKernel::EOrientation::Front : CADKernel::EOrientation::Back);
		}
	}
}

bool FAliasModelToCADKernelConverter::AddBRep(AlDagNode& DagNode, const FColor& Color, EAliasObjectReference InObjectReference)
{
	AlEdge2CADKernelEdge.Empty();

	TSharedRef<CADKernel::FBody> CADKernelBody = CADKernel::FEntity::MakeShared<CADKernel::FBody>();
	TSharedRef<CADKernel::FShell> CADKernelShell = CADKernel::FEntity::MakeShared<CADKernel::FShell>();
	uint32 ColorId = (uint32) CADLibrary::BuildColorName(Color);
	CADKernelShell->SetColorId(ColorId);
	CADKernelBody->AddShell(CADKernelShell);

	boolean bAlOrientation;
	DagNode.getSurfaceOrientation(bAlOrientation);
	bool bOrientation = (bool)bAlOrientation;

	AlMatrix4x4 AlMatrix;
	if (InObjectReference == EAliasObjectReference::ParentReference)
	{
		DagNode.localTransformationMatrix(AlMatrix);
	}

	AlObjectType objectType = DagNode.type();
	switch (objectType)
	{
		// Push all leaf nodes into 'leaves'
	case kShellNodeType:
	{
		if (AlShellNode* ShellNode = DagNode.asShellNodePtr())
		{
			AlShell* ShellPtr = ShellNode->shell();
			if(AlIsValid(ShellPtr))
			{
				TUniquePtr<AlShell> AliasShell(ShellPtr);
				AddShell(*ShellPtr, InObjectReference, AlMatrix, bOrientation, CADKernelShell);
			}
		}
		break;
	}

	case kSurfaceNodeType:
	{
		if (AlSurfaceNode* SurfaceNode = DagNode.asSurfaceNodePtr())
		{
			AlSurface* SurfacePtr = SurfaceNode->surface();
			if (AlIsValid(SurfacePtr))
			{
				TUniquePtr<AlSurface> AliasSurface(SurfacePtr);
				AddFace(*SurfacePtr, InObjectReference, AlMatrix, bOrientation, CADKernelShell);
			}
		}
		break;
	}
	default:
		break;
	}

	if (CADKernelShell->FaceCount() == 0)
	{
		return false;
	}

	// Create body from faces
	CADKernelSession.GetModel().Add(CADKernelBody);
	return true;
}

}

#endif
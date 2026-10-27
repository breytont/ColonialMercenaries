// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "World/CMRelayCompoundBlockout.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// The built-in engine cube is 100 x 100 x 100 Unreal units.
	constexpr float EngineCubeDimension = 100.0f;

	void AddBox(UInstancedStaticMeshComponent* Mesh,
		const FVector& LocalCenterCm, const FVector& SizeCm)
	{
		if (!Mesh)
		{
			return;
		}

		const FVector Scale = SizeCm / EngineCubeDimension;
		Mesh->AddInstance(FTransform(FQuat::Identity, LocalCenterCm, Scale));
	}
}

ACMRelayCompoundBlockout::ACMRelayCompoundBlockout()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false; // All pieces are static level geometry.

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Static);
	SetRootComponent(SceneRoot);

	PerimeterWalls = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PerimeterWalls"));
	PerimeterWalls->SetupAttachment(SceneRoot);
	PerimeterWalls->SetMobility(EComponentMobility::Static);
	PerimeterWalls->SetCollisionProfileName(TEXT("BlockAll"));

	CoverPieces = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CoverPieces"));
	CoverPieces->SetupAttachment(SceneRoot);
	CoverPieces->SetMobility(EComponentMobility::Static);
	CoverPieces->SetCollisionProfileName(TEXT("BlockAll"));

	Buildings = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("UtilityBuildings"));
	Buildings->SetupAttachment(SceneRoot);
	Buildings->SetMobility(EComponentMobility::Static);
	Buildings->SetCollisionProfileName(TEXT("BlockAll"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> EngineCube(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (EngineCube.Succeeded())
	{
		PerimeterWalls->SetStaticMesh(EngineCube.Object);
		CoverPieces->SetStaticMesh(EngineCube.Object);
		Buildings->SetStaticMesh(EngineCube.Object);
	}
}

void ACMRelayCompoundBlockout::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Instance lists are deterministic and rebuild only when edited / placed.
	PerimeterWalls->ClearInstances();
	CoverPieces->ClearInstances();
	Buildings->ClearInstances();

	const float X = FMath::Max(HalfExtentX, 1600.0f);
	const float Y = FMath::Max(HalfExtentY, 1400.0f);
	const float Gap = FMath::Clamp(EntranceHalfWidth, 300.0f, FMath::Min(X, Y) - 250.0f);
	const float H = FMath::Max(WallHeight, 70.0f);
	const float Thickness = 90.0f;

	// Four open entrances: west/east for assaulting factions and
	// north/south for flanking to the AA ridge and maintenance route.
	const float XSegmentLength = X - Gap;
	const float XSegmentCenter = (X + Gap) * 0.5f;
	const float YSegmentLength = Y - Gap;
	const float YSegmentCenter = (Y + Gap) * 0.5f;

	for (const float Side : {-1.0f, 1.0f})
	{
		for (const float Direction : {-1.0f, 1.0f})
		{
			// North and south perimeter wall segments.
			AddBox(PerimeterWalls,
				FVector(Direction * XSegmentCenter, Side * Y, H * 0.5f),
				FVector(XSegmentLength, Thickness, H));

			// East and west segments with clear central entries.
			AddBox(PerimeterWalls,
				FVector(Side * X, Direction * YSegmentCenter, H * 0.5f),
				FVector(Thickness, YSegmentLength, H));
		}
	}

	// Broken exterior cover creates staggered exposure when entering from
	// Red (west) or Blue (east), without walling off the cardinal entrances.
	const float LowH = FMath::Max(CoverHeight, 60.0f);
	for (const float Side : {-1.0f, 1.0f})
	{
		AddBox(CoverPieces,
			FVector(Side * 1490.0f, -350.0f, LowH * 0.5f),
			FVector(470.0f, 100.0f, LowH));
		AddBox(CoverPieces,
			FVector(Side * 1660.0f, 530.0f, LowH * 0.5f),
			FVector(350.0f, 120.0f, LowH));
	}

	// Interior cover on the 24x24m Relay_Foundation platform.
	const float InnerBase = FMath::Max(FoundationTopOffset, 0.0f);
	AddBox(CoverPieces, FVector(-850.0f, -760.0f, InnerBase + LowH * 0.5f),
		FVector(330.0f, 170.0f, LowH));
	AddBox(CoverPieces, FVector(730.0f, -720.0f, InnerBase + LowH * 0.5f),
		FVector(410.0f, 150.0f, LowH));
	AddBox(CoverPieces, FVector(-760.0f, 720.0f, InnerBase + LowH * 0.5f),
		FVector(350.0f, 140.0f, LowH));
	AddBox(CoverPieces, FVector(850.0f, 640.0f, InnerBase + LowH * 0.5f),
		FVector(270.0f, 170.0f, LowH));

	if (bPlaceUtilityHut)
	{
		// A solid industrial hut north of the capture point creates cover
		// and breaks central sight lines. Its back extends into the yard;
		// don't place the separate capture actor inside it.
		const float HutHeight = 290.0f;
		AddBox(Buildings,
			FVector(150.0f, 1070.0f, InnerBase + HutHeight * 0.5f),
			FVector(630.0f, 410.0f, HutHeight));

		// Short relay mast is a silhouette marker, not a climbable tower.
		AddBox(Buildings,
			FVector(150.0f, 1070.0f, InnerBase + HutHeight + 205.0f),
			FVector(45.0f, 45.0f, 410.0f));
	}
}

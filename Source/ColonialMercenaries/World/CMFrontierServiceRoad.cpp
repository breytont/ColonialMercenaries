// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "World/CMFrontierServiceRoad.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ACMFrontierServiceRoad::ACMFrontierServiceRoad()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SceneRoot->SetMobility(EComponentMobility::Static);
    SetRootComponent(SceneRoot);

    RoadSurface = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
        TEXT("RoadSurface"));
    RoadSurface->SetupAttachment(SceneRoot);
    RoadSurface->SetMobility(EComponentMobility::Static);
    RoadSurface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RoadSurface->SetCanEverAffectNavigation(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded())
    {
        RoadSurface->SetStaticMesh(Cube.Object);
    }
}

FVector ACMFrontierServiceRoad::TerrainAdjustedWorldPoint(float T) const
{
    constexpr float Pi = UE_PI;
    const FVector LocalXY(
        FMath::Sin(Pi * T) * MidpointLateralOffsetCm,
        T * LengthCm,
        0.0f);
    const FVector WorldXY = GetActorTransform().TransformPosition(LocalXY);

    UWorld* World = GetWorld();
    if (!World)
    {
        return WorldXY;
    }

    // Editor-time tracing against the landscape; the road itself never
    // blocks traces or NavMesh. Both ends sit inside existing wide gates,
    // away from the collision-enabled HQ and relay perimeter walls.
    FHitResult Hit;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CMServiceRoadGround), false);
    QueryParams.AddIgnoredActor(this);
    const FVector TraceStart(WorldXY.X, WorldXY.Y, WorldXY.Z + 20000.0f);
    const FVector TraceEnd(WorldXY.X, WorldXY.Y, WorldXY.Z - 20000.0f);
    if (World->LineTraceSingleByChannel(
            Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams)
        && Hit.bBlockingHit)
    {
        return FVector(WorldXY.X, WorldXY.Y, Hit.ImpactPoint.Z);
    }
    return WorldXY;
}

void ACMFrontierServiceRoad::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if (!RoadSurface)
    {
        return;
    }
    RoadSurface->ClearInstances();

    const int32 Count = FMath::Clamp(SegmentCount, 8, 80);
    const float EffectiveWidth = FMath::Clamp(WidthCm, 300.0f, 1000.0f);
    const float HalfThicknessCm = 4.0f;
    const float VerticalOffsetCm = HalfThicknessCm + FMath::Max(0.0f, RoadClearanceCm);
    const FTransform ActorTransform = GetActorTransform();

    // Sample once per vertex instead of tracing every cube independently.
    FVector WorldStart = TerrainAdjustedWorldPoint(0.0f);
    for (int32 I = 0; I < Count; ++I)
    {
        const float EndT = static_cast<float>(I + 1) / Count;
        const FVector WorldEnd = TerrainAdjustedWorldPoint(EndT);
        FVector LocalA = ActorTransform.InverseTransformPosition(WorldStart);
        FVector LocalB = ActorTransform.InverseTransformPosition(WorldEnd);

        const FVector Delta = LocalB - LocalA;
        const float SegmentLength = Delta.Size();
        if (SegmentLength > KINDA_SMALL_NUMBER)
        {
            const FQuat Orientation = FQuat::FindBetweenNormals(
                FVector::YAxisVector, Delta.GetSafeNormal());
            const FVector LocalCenter = (LocalA + LocalB) * 0.5f
                + FVector(0.0f, 0.0f, VerticalOffsetCm);
            // Built-in cube measures 100cm on each axis. Short thin cubes
            // tile to a single broad, walkable-looking visual corridor.
            RoadSurface->AddInstance(FTransform(
                Orientation,
                LocalCenter,
                FVector(EffectiveWidth / 100.0f,
                    (SegmentLength + 2.0f) / 100.0f,
                    (2.0f * HalfThicknessCm) / 100.0f)));
        }
        WorldStart = WorldEnd;
    }
}

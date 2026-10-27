// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "World/CMMercenaryHeadquartersBlockout.h"

#include "Components/ArrowComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    constexpr float CubeSizeCm = 100.0f;

    void AddBox(UInstancedStaticMeshComponent* Group, const FVector& Center,
        const FVector& Size)
    {
        if (IsValid(Group))
        {
            Group->AddInstance(FTransform(
                FQuat::Identity, Center, Size / CubeSizeCm));
        }
    }
}

ACMMercenaryHeadquartersBlockout::ACMMercenaryHeadquartersBlockout()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false; // The level itself provides static geometry to each client.

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SceneRoot->SetMobility(EComponentMobility::Static);
    SetRootComponent(SceneRoot);

    PrimaryGateDirection = CreateDefaultSubobject<UArrowComponent>(
        TEXT("PrimaryGateDirection"));
    PrimaryGateDirection->SetupAttachment(SceneRoot);
    PrimaryGateDirection->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    PrimaryGateDirection->SetArrowColor(FColor::Green);
    PrimaryGateDirection->SetArrowSize(1.8f);
    PrimaryGateDirection->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    GroundMarkers = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
        TEXT("GroundMarkers"));
    GroundMarkers->SetupAttachment(SceneRoot);
    GroundMarkers->SetMobility(EComponentMobility::Static);
    // Surface markings are visual only; the real landscape stays walkable.
    GroundMarkers->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GroundMarkers->SetCanEverAffectNavigation(false);

    PerimeterBarriers = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
        TEXT("PerimeterBarriers"));
    PerimeterBarriers->SetupAttachment(SceneRoot);
    PerimeterBarriers->SetMobility(EComponentMobility::Static);
    PerimeterBarriers->SetCollisionProfileName(TEXT("BlockAll"));

    Facilities = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
        TEXT("Facilities"));
    Facilities->SetupAttachment(SceneRoot);
    Facilities->SetMobility(EComponentMobility::Static);
    Facilities->SetCollisionProfileName(TEXT("BlockAll"));

    Storage = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
        TEXT("StorageProps"));
    Storage->SetupAttachment(SceneRoot);
    Storage->SetMobility(EComponentMobility::Static);
    Storage->SetCollisionProfileName(TEXT("BlockAll"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded())
    {
        GroundMarkers->SetStaticMesh(Cube.Object);
        PerimeterBarriers->SetStaticMesh(Cube.Object);
        Facilities->SetStaticMesh(Cube.Object);
        Storage->SetStaticMesh(Cube.Object);
    }

    HeadquartersLabel = CreateDefaultSubobject<UTextRenderComponent>(
        TEXT("HeadquartersLabel"));
    HeadquartersLabel->SetupAttachment(SceneRoot);
    HeadquartersLabel->SetText(FText::FromString(TEXT("MERCENARY HQ")));
    HeadquartersLabel->SetWorldSize(52.0f);
    HeadquartersLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
    HeadquartersLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    OperationsLabel = CreateDefaultSubobject<UTextRenderComponent>(
        TEXT("OperationsLabel"));
    OperationsLabel->SetupAttachment(SceneRoot);
    OperationsLabel->SetText(FText::FromString(TEXT("OPERATIONS / FUTURE CONTRACTS")));
    OperationsLabel->SetWorldSize(27.0f);
    OperationsLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
    OperationsLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACMMercenaryHeadquartersBlockout::OnConstruction(
    const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    GroundMarkers->ClearInstances();
    PerimeterBarriers->ClearInstances();
    Facilities->ClearInstances();
    Storage->ClearInstances();

    const float X = FMath::Max(HalfWidthCm, 1300.0f);
    const float Y = FMath::Max(HalfDepthCm, 1300.0f);
    const float NorthGap = FMath::Clamp(
        NorthGateHalfWidthCm, 350.0f, X - 400.0f);
    const float EastGap = FMath::Clamp(
        EastGateHalfWidthCm, 300.0f, Y - 400.0f);
    const float H = FMath::Max(BarrierHeightCm, 70.0f);
    const float Thickness = 75.0f;

    // Two separate entrances: northern mission road and eastern services.
    // The yard stays fully open for a real, manually placed PlayerStart.
    const float NorthRun = X - NorthGap;
    const float NorthCenter = (X + NorthGap) * 0.5f;
    for (const float Side : {-1.0f, 1.0f})
    {
        AddBox(PerimeterBarriers,
            FVector(Side * NorthCenter, Y, H * 0.5f),
            FVector(NorthRun, Thickness, H));
    }
    AddBox(PerimeterBarriers,
        FVector(0.0f, -Y, H * 0.5f),
        FVector(2.0f * X, Thickness, H));
    AddBox(PerimeterBarriers,
        FVector(-X, 0.0f, H * 0.5f),
        FVector(Thickness, 2.0f * Y, H));

    const float EastRun = Y - EastGap;
    const float EastCenter = (Y + EastGap) * 0.5f;
    for (const float Side : {-1.0f, 1.0f})
    {
        AddBox(PerimeterBarriers,
            FVector(X, Side * EastCenter, H * 0.5f),
            FVector(Thickness, EastRun, H));
    }

    // Thin visual wayfinding marks; never let these block navigation.
    AddBox(GroundMarkers, FVector(0.0f, Y - 380.0f, 2.5f),
        FVector(600.0f, 160.0f, 5.0f));
    AddBox(GroundMarkers, FVector(0.0f, Y - 850.0f, 2.5f),
        FVector(600.0f, 160.0f, 5.0f));
    AddBox(GroundMarkers, FVector(-80.0f, -280.0f, 2.5f),
        FVector(420.0f, 240.0f, 5.0f)); // Future PlayerStart area.

    if (bShowOperationsModule)
    {
        // Prefab operations building on the east side. Its western wall
        // faces the player yard, leaving room for a later separate,
        // interactive contract terminal. This building is NOT an interior.
        const float ModuleHeight = 360.0f;
        AddBox(Facilities,
            FVector(950.0f, 550.0f, ModuleHeight * 0.5f),
            FVector(850.0f, 750.0f, ModuleHeight));
        AddBox(Facilities,
            FVector(950.0f, 550.0f, ModuleHeight + 28.0f),
            FVector(980.0f, 860.0f, 56.0f));
    }

    if (bShowVehicleCanopy)
    {
        // Open-sided vehicle shelter on west side, about 11 x 8 metres.
        // Four supports and a roof leave clear vehicle access from yard.
        constexpr float CanopyX = -970.0f;
        constexpr float CanopyY = -250.0f;
        constexpr float CanopyHalfX = 530.0f;
        constexpr float CanopyHalfY = 410.0f;
        constexpr float CanopyHeight = 415.0f;
        for (const float SideX : {-1.0f, 1.0f})
        {
            for (const float SideY : {-1.0f, 1.0f})
            {
                AddBox(Facilities,
                    FVector(CanopyX + SideX * CanopyHalfX,
                        CanopyY + SideY * CanopyHalfY,
                        CanopyHeight * 0.5f),
                    FVector(55.0f, 55.0f, CanopyHeight));
            }
        }
        AddBox(Facilities,
            FVector(CanopyX, CanopyY, CanopyHeight + 17.0f),
            FVector(1180.0f, 940.0f, 34.0f));
        AddBox(GroundMarkers, FVector(CanopyX, CanopyY, 2.5f),
            FVector(1040.0f, 760.0f, 5.0f));
    }

    if (bShowStorage)
    {
        // Small low props, separated so player can walk between them.
        AddBox(Storage, FVector(500.0f, -880.0f, 65.0f),
            FVector(190.0f, 210.0f, 130.0f));
        AddBox(Storage, FVector(810.0f, -880.0f, 65.0f),
            FVector(210.0f, 200.0f, 130.0f));
        AddBox(Storage, FVector(1110.0f, -880.0f, 90.0f),
            FVector(200.0f, 200.0f, 180.0f));
    }

    HeadquartersLabel->SetRelativeLocation(FVector(0.0f, Y + 70.0f, 245.0f));
    HeadquartersLabel->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
    OperationsLabel->SetRelativeLocation(FVector(510.0f, 550.0f, 270.0f));
    OperationsLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
    OperationsLabel->SetVisibility(bShowOperationsModule);
}

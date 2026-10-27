// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "World/CMPrototypeIntelCache.h"

#include "ColonialMercenariesCharacter.h"
#include "World/CMPrototypeContractTerminal.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ACMPrototypeIntelCache::ACMPrototypeIntelCache()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(false);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    CacheBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CacheBody"));
    CacheBody->SetupAttachment(SceneRoot);
    CacheBody->SetRelativeLocation(FVector(0.0f, 0.0f, 45.0f));
    CacheBody->SetRelativeScale3D(FVector(0.65f, 0.5f, 0.55f));
    CacheBody->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CacheBody->SetCollisionObjectType(ECC_WorldDynamic);
    CacheBody->SetCollisionResponseToAllChannels(ECR_Ignore);
    CacheBody->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    CacheBody->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    CacheBody->SetCanEverAffectNavigation(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded())
    {
        CacheBody->SetStaticMesh(Cube.Object);
    }

    StatusLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("IntelCacheStatus"));
    StatusLabel->SetupAttachment(SceneRoot);
    StatusLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
    StatusLabel->SetWorldSize(23.0f);
    StatusLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
    StatusLabel->SetTextRenderColor(FColor::Yellow);
    StatusLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACMPrototypeIntelCache::BeginPlay()
{
    Super::BeginPlay();
    RefreshStatusLabel();
}

void ACMPrototypeIntelCache::TryInteract(AColonialMercenariesCharacter* Player)
{
    if (!HasAuthority() || bRecovered || !IsValid(ContractTerminal))
    {
        return;
    }

    if (ContractTerminal->TryRecoverIntel(Player, this))
    {
        bRecovered = true;
        RefreshStatusLabel();
        ForceNetUpdate();
    }
}

void ACMPrototypeIntelCache::RefreshStatusLabel()
{
    if (!IsValid(StatusLabel))
    {
        return;
    }

    StatusLabel->SetText(FText::FromString(bRecovered
        ? TEXT("RELAY INTEL RECOVERED")
        : TEXT("RELAY INTELLIGENCE CACHE\n[F] RECOVER (CONTRACT REQUIRED)")));
}

void ACMPrototypeIntelCache::OnRep_Recovered()
{
    RefreshStatusLabel();
}

void ACMPrototypeIntelCache::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACMPrototypeIntelCache, bRecovered);
}

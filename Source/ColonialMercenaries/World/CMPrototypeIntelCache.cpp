// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "World/CMPrototypeIntelCache.h"

#include "ColonialMercenariesCharacter.h"
#include "World/CMPrototypeContractTerminal.h"
#include "Combat/CMHealthComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ACMPrototypeIntelCache::ACMPrototypeIntelCache()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(true); // Ground-drop and carried attachment updates.
    
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
    RefreshPresentation();
}

void ACMPrototypeIntelCache::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnbindCarrierDelegates();
    Super::EndPlay(EndPlayReason);
}

void ACMPrototypeIntelCache::UnbindCarrierDelegates()
{
    if (IsValid(Carrier))
    {
        if (UCMHealthComponent* Health = Carrier->GetHealthComponent())
        {
            Health->OnDied.RemoveDynamic(
                this, &ACMPrototypeIntelCache::OnCarrierIncapacitated);
        }
        Carrier->OnDestroyed.RemoveDynamic(
            this, &ACMPrototypeIntelCache::OnCarrierActorDestroyed);
    }

    if (AController* PreviousController = CarrierController.Get())
    {
        PreviousController->OnDestroyed.RemoveDynamic(
            this, &ACMPrototypeIntelCache::OnCarrierControllerDestroyed);
    }
    CarrierController.Reset();
}

void ACMPrototypeIntelCache::TryInteract(AColonialMercenariesCharacter* Player)
{
    if (!HasAuthority() || bDelivered || IsValid(Carrier)
        || !IsValid(Player) || Player->IsIncapacitated()
        || !IsValid(ContractTerminal)
        || FVector::DistSquared(
            Player->GetActorLocation(), GetActorLocation()) > FMath::Square(300.0f))
    {
        return;
    }

    // The first pickup updates shared contract progress. If intel is
    // dropped after that, any teammate may retrieve the same world item.
    const ECMPrototypeContractStage Stage = ContractTerminal->GetContractStage();
    if (Stage == ECMPrototypeContractStage::Active)
    {
        if (!ContractTerminal->TryRecoverIntel(Player, this))
        {
            return;
        }
    }
    else if (Stage != ECMPrototypeContractStage::IntelRecovered)
    {
        return;
    }

    Carrier = Player;
    CarrierController = Player->GetController();
    if (UCMHealthComponent* Health = Player->GetHealthComponent())
    {
        Health->OnDied.AddUniqueDynamic(
            this, &ACMPrototypeIntelCache::OnCarrierIncapacitated);
    }
    Player->OnDestroyed.AddUniqueDynamic(
        this, &ACMPrototypeIntelCache::OnCarrierActorDestroyed);
    if (AController* Controller = CarrierController.Get())
    {
        Controller->OnDestroyed.AddUniqueDynamic(
            this, &ACMPrototypeIntelCache::OnCarrierControllerDestroyed);
    }

    // A visible, tangible briefcase-sized object travels with its carrier.
    // The mesh has no interaction collision while carried; otherwise F
    // near HQ could select the carried intel instead of the terminal.
    AttachToComponent(Player->GetRootComponent(),
        FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    SetActorRelativeLocation(FVector(-35.0f, 42.0f, 25.0f));

    UE_LOG(LogTemp, Display,
        TEXT("CM INTEL: %s is carrying relay intelligence."),
        *Player->GetName());
    RefreshPresentation();
    ForceNetUpdate();
}

void ACMPrototypeIntelCache::DropPhysicalIntel()
{
    if (!HasAuthority() || !Carrier)
    {
        return;
    }

    // Preserve the current in-world location before a pawn or controller
    // disappears. Disconnection and incapacitation both release the item.
    const FVector LastCarriedPosition = GetActorLocation();
    UnbindCarrierDelegates();
    Carrier = nullptr;
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    FVector DropPosition = LastCarriedPosition;
    if (UWorld* World = GetWorld())
    {
        FHitResult Hit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(CMIntelDrop), false);
        Params.AddIgnoredActor(this);
        FCollisionObjectQueryParams WorldGeometry;
        WorldGeometry.AddObjectTypesToQuery(ECC_WorldStatic);
        // Trace only world geometry; never put the dropped briefcase
        // on top of the dying carrier's Visibility-blocking capsule.
        if (World->LineTraceSingleByObjectType(
            Hit,
            LastCarriedPosition + FVector(0.0f, 0.0f, 150.0f),
            LastCarriedPosition - FVector(0.0f, 0.0f, 1800.0f),
            WorldGeometry, Params) && Hit.bBlockingHit)
        {
            DropPosition.Z = Hit.ImpactPoint.Z;
        }
    }
    SetActorLocation(DropPosition, false);
    SetActorRotation(FRotator::ZeroRotator);

    UE_LOG(LogTemp, Display,
        TEXT("CM INTEL: Carrier lost. Intel dropped at %s."),
        *DropPosition.ToString());
    RefreshPresentation();
    ForceNetUpdate();
}

void ACMPrototypeIntelCache::OnCarrierIncapacitated()
{
    DropPhysicalIntel();
}

void ACMPrototypeIntelCache::OnCarrierActorDestroyed(AActor* DestroyedActor)
{
    if (DestroyedActor == Carrier)
    {
        DropPhysicalIntel();
    }
}

void ACMPrototypeIntelCache::OnCarrierControllerDestroyed(AActor* DestroyedActor)
{
    // This callback is bound only to the current carrier's controller;
    // by the time an actor reports destruction IsValid may already be
    // false, so do not rely on weak-reference identity here.
    if (HasAuthority() && Carrier)
    {
        DropPhysicalIntel();
    }
}

bool ACMPrototypeIntelCache::FinalizeDelivery(AColonialMercenariesCharacter* Player)
{
    if (!HasAuthority() || bDelivered || !IsCarriedBy(Player))
    {
        return false;
    }

    UnbindCarrierDelegates();
    Carrier = nullptr;
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    bDelivered = true;

    UE_LOG(LogTemp, Display,
        TEXT("CM INTEL: Physical package delivered to HQ by %s."),
        *GetNameSafe(Player));
    RefreshPresentation();
    ForceNetUpdate();
    return true;
}

void ACMPrototypeIntelCache::RefreshPresentation()
{
    if (!IsValid(StatusLabel) || !IsValid(CacheBody))
    {
        return;
    }

    const bool bOnGround = !bDelivered && !IsValid(Carrier);
    CacheBody->SetVisibility(!bDelivered);
    CacheBody->SetCollisionEnabled(bOnGround
        ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
    StatusLabel->SetVisibility(bOnGround);
    if (bOnGround)
    {
        const bool bRecoverAgain = IsValid(ContractTerminal)
            && ContractTerminal->GetContractStage()
                == ECMPrototypeContractStage::IntelRecovered;
        StatusLabel->SetText(FText::FromString(bRecoverAgain
            ? TEXT("DROPPED RELAY INTEL\n[F] RETRIEVE")
            : TEXT("RELAY INTELLIGENCE CACHE\n[F] RECOVER (CONTRACT REQUIRED)")));
    }
}

void ACMPrototypeIntelCache::OnRep_Carrier()
{
    RefreshPresentation();
}

void ACMPrototypeIntelCache::OnRep_Delivered()
{
    RefreshPresentation();
}

void ACMPrototypeIntelCache::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACMPrototypeIntelCache, Carrier);
    DOREPLIFETIME(ACMPrototypeIntelCache, bDelivered);
}

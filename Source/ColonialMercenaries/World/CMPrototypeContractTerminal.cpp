// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "World/CMPrototypeContractTerminal.h"

#include "ColonialMercenariesCharacter.h"
#include "World/CMPrototypeIntelCache.h"
#include "Combat/CMHealthComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ACMPrototypeContractTerminal::ACMPrototypeContractTerminal()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(false);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    TerminalBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TerminalBody"));
    TerminalBody->SetupAttachment(SceneRoot);
    TerminalBody->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));
    TerminalBody->SetRelativeScale3D(FVector(0.55f, 0.45f, 1.4f));
    TerminalBody->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TerminalBody->SetCollisionObjectType(ECC_WorldDynamic);
    TerminalBody->SetCollisionResponseToAllChannels(ECR_Ignore);
    TerminalBody->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    TerminalBody->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    TerminalBody->SetCanEverAffectNavigation(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded())
    {
        TerminalBody->SetStaticMesh(Cube.Object);
    }

    StatusLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ContractStatus"));
    StatusLabel->SetupAttachment(SceneRoot);
    StatusLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 172.0f));
    StatusLabel->SetWorldSize(23.0f);
    StatusLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
    StatusLabel->SetTextRenderColor(FColor::Yellow);
    StatusLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACMPrototypeContractTerminal::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority())
    {
        CompanyCredits = FMath::Max(0, StartingCompanyCredits);
        ContractStage = ECMPrototypeContractStage::Available;
    }
    RefreshStatusLabel();
}

void ACMPrototypeContractTerminal::TryInteract(AColonialMercenariesCharacter* Player)
{
    if (!HasAuthority() || !IsValid(Player) || Player->IsIncapacitated()
        || FVector::DistSquared(Player->GetActorLocation(), GetActorLocation())
            > FMath::Square(300.0f))
    {
        return;
    }

    switch (ContractStage)
    {
    case ECMPrototypeContractStage::Available:
        if (!IsValid(IntelCache) || IntelCache->GetContractTerminal() != this)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("CM CONTRACT: Setup incomplete. Link terminal and intel cache in both directions."));
            return;
        }
        ContractStage = ECMPrototypeContractStage::Active;
        UE_LOG(LogTemp, Display,
            TEXT("CM CONTRACT ACCEPTED: Relay Intelligence Recovery. Company reward %d credits."),
            RewardCredits);
        break;

    case ECMPrototypeContractStage::IntelRecovered:
        // The terminal is the only authority that grants currency.
        // Repeated interaction after payment can't duplicate rewards.
        CompanyCredits = FMath::Clamp(
            static_cast<int64>(CompanyCredits) + RewardCredits,
            static_cast<int64>(0), static_cast<int64>(100000000));
        ContractStage = ECMPrototypeContractStage::Paid;
        UE_LOG(LogTemp, Display,
            TEXT("CM CONTRACT PAID: +%d credits, company balance = %d."),
            RewardCredits, CompanyCredits);
        break;

    default:
        return;
    }

    RefreshStatusLabel();
    ForceNetUpdate();
}

bool ACMPrototypeContractTerminal::TryRecoverIntel(
    AColonialMercenariesCharacter* Player, ACMPrototypeIntelCache* Cache)
{
    if (!HasAuthority() || !IsValid(Player) || Player->IsIncapacitated()
        || !IsValid(Cache) || Cache != IntelCache
        || Cache->GetContractTerminal() != this
        || ContractStage != ECMPrototypeContractStage::Active
        || FVector::DistSquared(Player->GetActorLocation(), Cache->GetActorLocation())
            > FMath::Square(300.0f))
    {
        return false;
    }

    ContractStage = ECMPrototypeContractStage::IntelRecovered;
    UE_LOG(LogTemp, Display,
        TEXT("CM CONTRACT: Relay intel recovered. Return to HQ for payment."));
    RefreshStatusLabel();
    ForceNetUpdate();
    return true;
}

void ACMPrototypeContractTerminal::RefreshStatusLabel()
{
    if (!IsValid(StatusLabel))
    {
        return;
    }

    FString Task;
    switch (ContractStage)
    {
    case ECMPrototypeContractStage::Available:
        Task = FString::Printf(TEXT("[F] ACCEPT: RELAY INTEL\nReward: %d credits"), RewardCredits);
        break;
    case ECMPrototypeContractStage::Active:
        Task = TEXT("ACTIVE: RECOVER RELAY INTEL\nFind the cache at the relay");
        break;
    case ECMPrototypeContractStage::IntelRecovered:
        Task = TEXT("[F] TURN IN RELAY INTEL\nReturn for payment");
        break;
    case ECMPrototypeContractStage::Paid:
        Task = TEXT("CONTRACT COMPLETE - PAID");
        break;
    default:
        break;
    }

    StatusLabel->SetText(FText::FromString(FString::Printf(
        TEXT("MERCENARY CONTRACT BOARD\n%s\nCompany funds: %d"),
        *Task, CompanyCredits)));
}

void ACMPrototypeContractTerminal::OnRep_ContractUpdated()
{
    RefreshStatusLabel();
}

void ACMPrototypeContractTerminal::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACMPrototypeContractTerminal, ContractStage);
    DOREPLIFETIME(ACMPrototypeContractTerminal, CompanyCredits);
}

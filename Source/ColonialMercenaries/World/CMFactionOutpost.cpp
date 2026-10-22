// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "World/CMFactionOutpost.h"

#include "Combat/CMCombatSoldier.h"
#include "Combat/CMFactionComponent.h"
#include "Combat/CMHealthComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ACMFactionOutpost::ACMFactionOutpost()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DeploymentDirection = CreateDefaultSubobject<UArrowComponent>(
		TEXT("DeploymentDirection"));
	DeploymentDirection->SetupAttachment(SceneRoot);
	DeploymentDirection->SetArrowColor(FColor::Green);
	DeploymentDirection->SetArrowSize(1.5f);

	// No imported art or Blueprints required. This simple installation
	// reads as a base in the test level while leaving navigation untouched.
	BasePlatform = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BasePlatform"));
	BasePlatform->SetupAttachment(SceneRoot);
	BasePlatform->SetRelativeLocation(FVector(0.0f, 0.0f, 6.0f));
	BasePlatform->SetRelativeScale3D(FVector(4.0f, 3.0f, 0.12f));
	BasePlatform->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BaseStructure = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseStructure"));
	BaseStructure->SetupAttachment(SceneRoot);
	BaseStructure->SetRelativeLocation(FVector(-90.0f, 0.0f, 95.0f));
	BaseStructure->SetRelativeScale3D(FVector(1.6f, 1.9f, 1.75f));
	BaseStructure->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		BasePlatform->SetStaticMesh(Cube.Object);
		BaseStructure->SetStaticMesh(Cube.Object);
	}

	StatusLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("OutpostStatus"));
	StatusLabel->SetupAttachment(SceneRoot);
	StatusLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 230.0f));
	StatusLabel->SetWorldSize(30.0f);
	StatusLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	StatusLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SoldierClass = ACMCombatSoldier::StaticClass();
}

void ACMFactionOutpost::BeginPlay()
{
	Super::BeginPlay();

	RefreshStatusLabel();

	if (!HasAuthority())
	{
		return;
	}

	if (!FactionId.IsValid() || !SoldierClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("CM outpost %s is disabled: assign a faction tag and soldier class."),
			*GetName());
		return;
	}

	// This pool is finite. Initial occupants are deployed separately, and
	// replacement waves must spend the reserve one soldier at a time.
	ReserveRemaining = FMath::Clamp(InitialReserves, 0, 100);
	const int32 StartingCount = FMath::Min(
		FMath::Clamp(InitialGarrisonSize, 0, 12),
		FMath::Clamp(TargetGarrisonSize, 1, 12));
	SpawnInfantry(StartingCount, false);

	const UWorld* World = GetWorld();
	if (World)
	{
		NextReinforcementTime = World->GetTimeSeconds()
			+ FMath::Max(ReinforcementCooldownSeconds, 1.0f);
	}

	GetWorldTimerManager().SetTimer(
		GarrisonCheckTimerHandle, this, &ACMFactionOutpost::CheckGarrison,
		FMath::Max(GarrisonCheckInterval, 0.2f), true);

	RefreshStatusLabel();
	ForceNetUpdate();
}

void ACMFactionOutpost::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(GarrisonCheckTimerHandle);
	FieldedSoldiers.Reset();
	Super::EndPlay(EndPlayReason);
}

bool ACMFactionOutpost::AddReserves(int32 NumberOfSoldiers)
{
	// Only authoritative game systems may resupply an installation.
	// No automatic regeneration or client-driven credits.
	if (!HasAuthority() || NumberOfSoldiers <= 0)
	{
		return false;
	}

	const int32 OldReserve = ReserveRemaining;
	ReserveRemaining = FMath::Clamp(ReserveRemaining + NumberOfSoldiers, 0, 1000);
	if (ReserveRemaining == OldReserve)
	{
		return false;
	}

	RefreshStatusLabel();
	ForceNetUpdate();
	return true;
}

int32 ACMFactionOutpost::SpawnInfantry(int32 Requested, bool bConsumeReserves)
{
	if (!HasAuthority() || !FactionId.IsValid() || !SoldierClass || !GetWorld())
	{
		return 0;
	}

	const int32 GarrisonLimit = FMath::Clamp(TargetGarrisonSize, 1, 12);
	const int32 AvailableSlots = FMath::Max(0, GarrisonLimit - ActiveGarrison);
	const int32 Budget = bConsumeReserves
		? FMath::Min(FMath::Max(ReserveRemaining, 0), AvailableSlots)
		: AvailableSlots;
	const int32 ToSpawn = FMath::Min(FMath::Max(Requested, 0), Budget);

	const int32 PerRow = FMath::Clamp(SoldiersPerRow, 1, 6);
	const float SideSpacing = FMath::Max(LateralSpacing, 100.0f);
	const float BackSpacing = FMath::Max(RowSpacing, 100.0f);
	const FVector Front = GetActorForwardVector();
	const FVector Right = GetActorRightVector();
	const FVector DeploymentStart = GetActorLocation()
		+ Front * FMath::Max(DeploymentForwardOffset, 150.0f)
		+ FVector(0.0f, 0.0f, FMath::Max(SpawnHeight, 0.0f));

	int32 DeployedThisWave = 0;
	for (int32 Index = 0; Index < ToSpawn; ++Index)
	{
		// Reuse a bounded array of formation slots as replacements arrive.
		// Only the owning outpost tracks its own spawned soldier references.
		const int32 SlotIndex = TotalSoldiersDeployed % GarrisonLimit;
		const int32 Row = SlotIndex / PerRow;
		const int32 Column = SlotIndex % PerRow;
		const int32 MembersInRow =
			FMath::Min(PerRow, GarrisonLimit - Row * PerRow);
		const float SideOffset = (static_cast<float>(Column)
			- 0.5f * static_cast<float>(MembersInRow - 1)) * SideSpacing;
		const FVector SpawnLocation = DeploymentStart
			+ Right * SideOffset - Front * (Row * BackSpacing);
		const FTransform SoldierTransform(GetActorRotation(), SpawnLocation);

		// Assign allegiance before the soldier's BeginPlay and replication.
		ACMCombatSoldier* Soldier = GetWorld()->SpawnActorDeferred<ACMCombatSoldier>(
			SoldierClass.Get(), SoldierTransform, this, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
		if (!IsValid(Soldier))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("CM outpost %s could not deploy soldier %d. Check clear NavMesh."),
				*GetName(), Index + 1);
			break;
		}

		if (UCMFactionComponent* Faction = Soldier->GetFactionComponent())
		{
			Faction->SetFactionId(FactionId);
		}
		UGameplayStatics::FinishSpawningActor(Soldier, SoldierTransform);

		FieldedSoldiers.Add(Soldier);
		++ActiveGarrison;
		++TotalSoldiersDeployed;
		++DeployedThisWave;
		if (bConsumeReserves)
		{
			--ReserveRemaining;
		}
	}

	if (DeployedThisWave > 0)
	{
		UE_LOG(LogTemp, Log,
			TEXT("CM outpost %s deployed %d soldiers (%s). Active=%d, Reserve=%d, Total=%d"),
			*GetName(), DeployedThisWave, *FactionId.ToString(),
			ActiveGarrison, ReserveRemaining, TotalSoldiersDeployed);
		RefreshStatusLabel();
		ForceNetUpdate();
	}
	return DeployedThisWave;
}

void ACMFactionOutpost::RefreshGarrisonState()
{
	// Death makes a soldier unavailable immediately; the body itself can
	// linger for its independent corpse lifetime. No global actor scans.
	for (int32 Index = FieldedSoldiers.Num() - 1; Index >= 0; --Index)
	{
		const ACMCombatSoldier* Soldier = FieldedSoldiers[Index].Get();
		const UCMHealthComponent* Health =
			IsValid(Soldier) ? Soldier->GetHealthComponent() : nullptr;
		if (!IsValid(Soldier) || !IsValid(Health) || Health->IsDead())
		{
			FieldedSoldiers.RemoveAtSwap(Index, 1, EAllowShrinking::No);
		}
	}

	const int32 LivingCount = FieldedSoldiers.Num();
	if (LivingCount != ActiveGarrison)
	{
		ActiveGarrison = LivingCount;
		RefreshStatusLabel();
		ForceNetUpdate();
	}
}

void ACMFactionOutpost::CheckGarrison()
{
	if (!HasAuthority() || !GetWorld())
	{
		return;
	}

	RefreshGarrisonState();

	if (ActiveGarrison >= FMath::Clamp(TargetGarrisonSize, 1, 12)
		|| ReserveRemaining <= 0
		|| GetWorld()->GetTimeSeconds() < NextReinforcementTime)
	{
		return;
	}

	const int32 Missing = FMath::Max(0,
		FMath::Clamp(TargetGarrisonSize, 1, 12) - ActiveGarrison);
	const int32 Requested = FMath::Min(
		FMath::Clamp(ReinforcementWaveSize, 1, 12), Missing);
	const int32 Deployed = SpawnInfantry(Requested, true);

	// A failed spawn should retry at a modest interval rather than
	// drain any soldiers or spam path/collision checks every frame.
	NextReinforcementTime = GetWorld()->GetTimeSeconds()
		+ (Deployed > 0
			? FMath::Max(ReinforcementCooldownSeconds, 1.0f)
			: FMath::Max(GarrisonCheckInterval, 2.0f));
}

void ACMFactionOutpost::RefreshStatusLabel()
{
	if (!IsValid(StatusLabel))
	{
		return;
	}

	const FString FactionName = FactionId.IsValid()
		? FactionId.GetTagName().ToString() : TEXT("UNASSIGNED");
	const FString ShortFaction = FactionName.Contains(TEXT("Red"))
		? TEXT("RED")
		: (FactionName.Contains(TEXT("Blue")) ? TEXT("BLUE") : FactionName);

	StatusLabel->SetTextRenderColor(ShortFaction == TEXT("RED")
		? FColor::Red
		: (ShortFaction == TEXT("BLUE") ? FColor::Cyan : FColor::White));

	const int32 GarrisonLimit = FMath::Clamp(TargetGarrisonSize, 1, 12);
	FString Status = TEXT("Garrison ready");
	if (ActiveGarrison < GarrisonLimit && ReserveRemaining <= 0)
	{
		Status = TEXT("RESERVES EXHAUSTED");
	}
	else if (ActiveGarrison < GarrisonLimit && ReserveRemaining > 0)
	{
		Status = TEXT("Awaiting reinforcements");
	}

	StatusLabel->SetText(FText::FromString(FString::Printf(
		TEXT("%s [%s]\nGarrison %d/%d  |  Reserve %d\n%s"),
		*OutpostName, *ShortFaction, ActiveGarrison, GarrisonLimit,
		ReserveRemaining, *Status)));
}

void ACMFactionOutpost::OnRep_OutpostStatus()
{
	RefreshStatusLabel();
}

void ACMFactionOutpost::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACMFactionOutpost, FactionId);
	DOREPLIFETIME(ACMFactionOutpost, ActiveGarrison);
	DOREPLIFETIME(ACMFactionOutpost, ReserveRemaining);
	DOREPLIFETIME(ACMFactionOutpost, TotalSoldiersDeployed);
}

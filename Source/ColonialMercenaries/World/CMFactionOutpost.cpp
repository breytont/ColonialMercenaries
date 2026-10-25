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
#include "GameFramework/Actor.h"
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
	CurrentPosture = StartingPosture;
	SquadPhase = CurrentPosture == ECMOutpostPosture::Defend
		? ECMOutpostSquadPhase::Defend : ECMOutpostSquadPhase::Assembly;
	PhaseStartedAt = GetWorld()->GetTimeSeconds();
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

	UpdateSquadOrders();
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
	ReserveRemaining = FMath::Clamp(ReserveRemaining + FMath::Min(NumberOfSoldiers, 1000), 0, 1000);
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

void ACMFactionOutpost::SetOutpostPosture(ECMOutpostPosture NewPosture)
{
	if (!HasAuthority() || CurrentPosture == NewPosture) { return; }
	CurrentPosture = NewPosture;
	SetSquadPhase(NewPosture == ECMOutpostPosture::Defend
		? ECMOutpostSquadPhase::Defend : ECMOutpostSquadPhase::Assembly);
	UpdateSquadOrders();
}

void ACMFactionOutpost::SetSquadPhase(ECMOutpostSquadPhase NewPhase)
{
	if (!HasAuthority() || SquadPhase == NewPhase || !GetWorld()) { return; }
	SquadPhase = NewPhase;
	PhaseStartedAt = GetWorld()->GetTimeSeconds();
	UE_LOG(LogTemp, Log, TEXT("CM outpost %s squad phase -> %s"),
		*GetName(), *UEnum::GetValueAsString(SquadPhase));
	RefreshStatusLabel();
	ForceNetUpdate();
}

FVector ACMFactionOutpost::GetSquadSlot(
	int32 Index, ECMOutpostSquadPhase Phase) const
{
	const FVector Front = GetActorForwardVector();
	const FVector Right = GetActorRightVector();
	FVector Anchor = GetActorLocation();
	switch (Phase)
	{
	case ECMOutpostSquadPhase::Assembly:
		Anchor += Front * FMath::Max(AssemblyForwardOffset, 0.0f);
		break;
	case ECMOutpostSquadPhase::Advance:
		Anchor = IsValid(AssaultObjective)
			? AssaultObjective->GetActorLocation()
			: GetActorLocation() + Front * FMath::Max(FallbackAdvanceDistance, 300.0f);
		break;
	case ECMOutpostSquadPhase::Retreat:
		Anchor -= Front * FMath::Max(WithdrawalBackOffset, 0.0f);
		break;
	case ECMOutpostSquadPhase::Defend:
		Anchor += Front * 250.0f;
		break;
	default:
		break;
	}

	const int32 PerRow = FMath::Clamp(SoldiersPerRow, 1, 6);
	const int32 Count = FMath::Max(1, FieldedSoldiers.Num());
	const int32 Row = Index / PerRow;
	const int32 Column = Index % PerRow;
	const int32 MembersInRow = FMath::Min(PerRow, Count - Row * PerRow);
	const float Side = (static_cast<float>(Column)
		- 0.5f * static_cast<float>(MembersInRow - 1))
		* FMath::Max(SquadSlotSpacing, 100.0f);
	return Anchor + Right * Side
		- Front * (Row * FMath::Max(RowSpacing, 100.0f));
}

void ACMFactionOutpost::UpdateSquadOrders()
{
	if (!HasAuthority() || !GetWorld()) { return; }

	const ECMSquadOrder Order =
		SquadPhase == ECMOutpostSquadPhase::Assembly ? ECMSquadOrder::Rally
		: SquadPhase == ECMOutpostSquadPhase::Retreat ? ECMSquadOrder::Disengage
		: SquadPhase == ECMOutpostSquadPhase::Defend ? ECMSquadOrder::Defend
		: ECMSquadOrder::Advance;

	// Both factions share contacts during EVERY squad phase, including
	// withdrawal. This is awareness only: a withdrawing soldier can
	// deliver local covering fire but can never abandon its retreat route
	// to chase the shared target.
	{
		ACharacter* NewContact = nullptr;
		bool bNewContactWasProvoked = false;
		const float AlertRadius = FMath::Max(DefensiveAssistRadius, 500.0f);
		for (const TWeakObjectPtr<ACMCombatSoldier>& Member : FieldedSoldiers)
		{
			const ACMCombatSoldier* Soldier = Member.Get();
			if (!IsValid(Soldier)) { continue; }

			bool bConfirmedAttacker = false;
			ACharacter* Candidate = Soldier->GetReportedThreat(bConfirmedAttacker);
			const UCMHealthComponent* ContactHealth = IsValid(Candidate)
				? Candidate->FindComponentByClass<UCMHealthComponent>() : nullptr;
			const UCMFactionComponent* CandidateFaction = IsValid(Candidate)
				? Candidate->FindComponentByClass<UCMFactionComponent>() : nullptr;
			const UCMFactionComponent* SoldierFaction = Soldier->GetFactionComponent();
			if (!IsValid(Candidate) || !IsValid(ContactHealth)
				|| ContactHealth->IsDead() || !IsValid(SoldierFaction))
			{
				continue;
			}
			const bool bHostile = SoldierFaction->IsHostileTo(CandidateFaction);
			const bool bUnalignedAggressor = bConfirmedAttacker
				&& (!IsValid(CandidateFaction)
					|| !CandidateFaction->GetFactionId().IsValid());
			if (!(bHostile || bUnalignedAggressor))
			{
				continue;
			}

			// A confirmed hit provides reliable contact even when fired
			// from outside the usual detection radius. Ordinary visual
			// reports must still originate within the local squad area.
			// The AI's duty-radius leash constrains its navigation, NOT
			// whether a squadmate may warn others about a distant attacker.
			if (!bConfirmedAttacker
				&& FVector::DistSquared2D(
					Soldier->GetActorLocation(), Candidate->GetActorLocation())
					> FMath::Square(AlertRadius))
			{
				continue;
			}
			// A rifleman that was actually hit is a higher-priority
			// report than a squadmate merely observing the opposing
			// faction. Otherwise an already-engaged squad can ignore
			// the player who just attacked its second/third member.
			if (bConfirmedAttacker)
			{
				NewContact = Candidate;
				bNewContactWasProvoked = bUnalignedAggressor;
				break;
			}
			if (!IsValid(NewContact))
			{
				NewContact = Candidate;
				bNewContactWasProvoked = bUnalignedAggressor;
			}
		}

		ACharacter* PreviousContact = SharedSquadContact.Get();
		const float Now = GetWorld()->GetTimeSeconds();
		if (IsValid(NewContact))
		{
			SharedSquadContact = NewContact;
			bSharedContactWasProvoked = bNewContactWasProvoked;
			SquadContactExpiresAt = Now
				+ FMath::Max(DefensiveContactMemorySeconds, 0.0f);
			if (PreviousContact != NewContact)
			{
				UE_LOG(LogTemp, Display,
					TEXT("CM SQUAD TARGET: %s switched contact to %s"),
					*GetName(), *NewContact->GetName());
			}
		}
		else
		{
			ACharacter* Remembered = SharedSquadContact.Get();
			const UCMHealthComponent* Health = IsValid(Remembered)
				? Remembered->FindComponentByClass<UCMHealthComponent>() : nullptr;
			if (!IsValid(Remembered) || !IsValid(Health) || Health->IsDead()
				|| Now >= SquadContactExpiresAt)
			{
				SharedSquadContact.Reset();
				bSharedContactWasProvoked = false;
			}
		}
	}
	const bool bHasContact = IsValid(SharedSquadContact.Get());
	if (bSquadContactActive != bHasContact)
	{
		bSquadContactActive = bHasContact;
		UE_LOG(LogTemp, Display,
			TEXT("CM SQUAD ALERT: %s %s%s"),
			*GetName(), bHasContact ? TEXT("engaging") : TEXT("contact cleared"),
			bHasContact && SharedSquadContact.IsValid()
				? *SharedSquadContact->GetName() : TEXT(""));
		RefreshStatusLabel();
		ForceNetUpdate();
	}

	for (int32 Index = 0; Index < FieldedSoldiers.Num(); ++Index)
	{
		ACMCombatSoldier* Soldier = FieldedSoldiers[Index].Get();
		if (!IsValid(Soldier)) { continue; }
		Soldier->SetSquadOrder(Order, GetSquadSlot(Index, SquadPhase));

		// Defense stays inside the base perimeter; assault assistance is
		// leashed to each member's objective slot. During retreat the
		// contact is informational only: soldiers remain bound to their
		// withdrawal slots and can opportunistically return fire.
		const FVector Center = SquadPhase == ECMOutpostSquadPhase::Defend
			? GetActorLocation() : GetSquadSlot(Index, SquadPhase);
		Soldier->SetSharedSquadContact(
			bHasContact ? SharedSquadContact.Get() : nullptr,
			bSharedContactWasProvoked, Center,
			FMath::Max(DefensiveAssistRadius, 500.0f));
	}
}

void ACMFactionOutpost::CheckGarrison()
{
	if (!HasAuthority() || !GetWorld()) { return; }

	RefreshGarrisonState();
	const float Now = GetWorld()->GetTimeSeconds();
	const int32 GarrisonLimit = FMath::Clamp(TargetGarrisonSize, 1, 12);

	if (CurrentPosture == ECMOutpostPosture::Defend)
	{
		SetSquadPhase(ECMOutpostSquadPhase::Defend);
	}
	else if (SquadPhase == ECMOutpostSquadPhase::Assembly)
	{
		if (ActiveGarrison > 0
			&& Now - PhaseStartedAt >= FMath::Max(AssemblySeconds, 0.0f))
		{
			SetSquadPhase(ECMOutpostSquadPhase::Advance);
		}
	}
	else if (SquadPhase == ECMOutpostSquadPhase::Advance)
	{
		if (ActiveGarrison <= FMath::Clamp(RetreatAtOrBelow, 0, GarrisonLimit - 1))
		{
			SetSquadPhase(ECMOutpostSquadPhase::Retreat);
		}
	}
	else if (SquadPhase == ECMOutpostSquadPhase::Retreat)
	{
		if (ActiveGarrison >= GarrisonLimit
			&& Now - PhaseStartedAt >= FMath::Max(RegroupSeconds, 0.0f))
		{
			SetSquadPhase(ECMOutpostSquadPhase::Assembly);
		}
	}

	if (ActiveGarrison < GarrisonLimit && ReserveRemaining > 0
		&& Now >= NextReinforcementTime)
	{
		const int32 Requested = FMath::Min(
			FMath::Clamp(ReinforcementWaveSize, 1, 12),
			GarrisonLimit - ActiveGarrison);
		const int32 Deployed = SpawnInfantry(Requested, true);
		NextReinforcementTime = Now
			+ (Deployed > 0
				? FMath::Max(ReinforcementCooldownSeconds, 1.0f)
				: FMath::Max(GarrisonCheckInterval, 2.0f));
	}

	UpdateSquadOrders();
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
	const TCHAR* PhaseText =
		SquadPhase == ECMOutpostSquadPhase::Assembly ? TEXT("ASSEMBLING")
		: SquadPhase == ECMOutpostSquadPhase::Advance ? TEXT("ADVANCING")
		: SquadPhase == ECMOutpostSquadPhase::Retreat ? TEXT("WITHDRAWING")
		: TEXT("DEFENDING");
	const FString PhaseStatus = bSquadContactActive
		? FString::Printf(TEXT("%s - CONTACT"), PhaseText)
		: FString(PhaseText);
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
		TEXT("%s [%s]\nGarrison %d/%d  |  Reserve %d\n%s  |  %s"),
		*OutpostName, *ShortFaction, ActiveGarrison, GarrisonLimit,
		ReserveRemaining, *PhaseStatus, *Status)));
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
	DOREPLIFETIME(ACMFactionOutpost, SquadPhase);
	DOREPLIFETIME(ACMFactionOutpost, bSquadContactActive);
}

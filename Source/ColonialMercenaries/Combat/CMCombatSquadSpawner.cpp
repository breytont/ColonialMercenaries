// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "Combat/CMCombatSquadSpawner.h"

#include "Combat/CMCombatSoldier.h"
#include "Combat/CMFactionComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ACMCombatSquadSpawner::ACMCombatSquadSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	// The spawn point itself has no runtime replicated state. Its soldiers do.
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	FacingArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("FacingArrow"));
	FacingArrow->SetupAttachment(SceneRoot);

	SoldierClass = ACMCombatSoldier::StaticClass();
}

void ACMCombatSquadSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SpawnSquad();
	}
}

void ACMCombatSquadSpawner::SpawnSquad()
{
	if (!FactionId.IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("CM squad spawner %s has no Faction Id. Choose Red or Blue in Details."),
			*GetName());
		return;
	}

	if (!SoldierClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("CM squad spawner %s has no Soldier Class."), *GetName());
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const int32 Count = FMath::Clamp(SoldierCount, 1, 12);
	const int32 PerRow = FMath::Clamp(SoldiersPerRow, 1, 6);
	const float SideSpacing = FMath::Max(LateralSpacing, 100.0f);
	const float BackSpacing = FMath::Max(RowSpacing, 100.0f);
	const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, SpawnHeight);

	int32 SuccessfullySpawned = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const int32 Column = Index % PerRow;
		const int32 Row = Index / PerRow;
		const int32 MembersInRow = FMath::Min(PerRow, Count - Row * PerRow);

		// Center each rank on the squad anchor. Additional ranks go backwards.
		const float SideOffset =
			(static_cast<float>(Column) - 0.5f * (MembersInRow - 1)) * SideSpacing;
		const FVector Position = Start
			+ GetActorRightVector() * SideOffset
			- GetActorForwardVector() * (Row * BackSpacing);
		const FTransform Transform(GetActorRotation(), Position);

		// Configure faction membership *before* BeginPlay and initial replication.
		// Ordinary SpawnActor would call BeginPlay before the faction was assigned.
		ACMCombatSoldier* Member = World->SpawnActorDeferred<ACMCombatSoldier>(
			SoldierClass.Get(), Transform, this, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!IsValid(Member))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("CM squad spawner %s could not create soldier %d."),
				*GetName(), Index + 1);
			continue;
		}

		if (UCMFactionComponent* Faction = Member->GetFactionComponent())
		{
			Faction->SetFactionId(FactionId);
		}

		UGameplayStatics::FinishSpawningActor(Member, Transform);
		++SuccessfullySpawned;
	}

	UE_LOG(LogTemp, Log, TEXT("CM squad spawner %s deployed %d soldiers for %s."),
		*GetName(), SuccessfullySpawned, *FactionId.ToString());
}

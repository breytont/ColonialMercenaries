// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "World/CMContestedPoint.h"

#include "Combat/CMCombatSoldier.h"
#include "Combat/CMFactionComponent.h"
#include "Combat/CMHealthComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

ACMContestedPoint::ACMContestedPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CaptureArea = CreateDefaultSubobject<USphereComponent>(TEXT("CaptureArea"));
	CaptureArea->SetupAttachment(SceneRoot);
	CaptureArea->InitSphereRadius(1600.0f);
	CaptureArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CaptureArea->SetCollisionResponseToAllChannels(ECR_Ignore);
	CaptureArea->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CaptureArea->SetGenerateOverlapEvents(false); // We query on a timer instead.

	StatusLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StatusLabel"));
	StatusLabel->SetupAttachment(SceneRoot);
	StatusLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 260.0f));
	StatusLabel->SetWorldSize(45.0f);
	StatusLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	StatusLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACMContestedPoint::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		OwnerFaction = InitialOwnerFaction;

		const float Interval = FMath::Max(CaptureUpdateInterval, 0.1f);
		GetWorldTimerManager().SetTimer(
			CaptureTimerHandle, this, &ACMContestedPoint::UpdateCapture,
			Interval, true);
	}

	RefreshStatusLabel();
}

void ACMContestedPoint::UpdateCapture()
{
	if (!HasAuthority() || !IsValid(CaptureArea))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// The query works even when capture volume overlap events are disabled.
	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectTypes;
	ObjectTypes.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams Params;

	World->OverlapMultiByObjectType(
		Overlaps, CaptureArea->GetComponentLocation(), FQuat::Identity,
		ObjectTypes,
		FCollisionShape::MakeSphere(CaptureArea->GetScaledSphereRadius()),
		Params);

	TSet<const ACMCombatSoldier*> CountedSoldiers;
	TMap<FGameplayTag, int32> OccupantCounts;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		const ACMCombatSoldier* Soldier = Cast<ACMCombatSoldier>(Overlap.GetActor());
		if (!IsValid(Soldier) || CountedSoldiers.Contains(Soldier))
		{
			continue;
		}
		CountedSoldiers.Add(Soldier);

		const UCMHealthComponent* Health = Soldier->GetHealthComponent();
		const UCMFactionComponent* Faction = Soldier->GetFactionComponent();
		if (!IsValid(Health) || Health->IsDead() || !IsValid(Faction))
		{
			continue;
		}

		const FGameplayTag FactionId = Faction->GetFactionId();
		if (FactionId.IsValid())
		{
			OccupantCounts.FindOrAdd(FactionId) += 1;
		}
	}

	const bool bWasContested = bContested;
	bContested = OccupantCounts.Num() > 1;
	if (bContested || OccupantCounts.IsEmpty())
	{
		// A contested or empty point holds its current progress for now.
		if (bWasContested != bContested)
		{
			RefreshStatusLabel();
			ForceNetUpdate();
		}
		return;
	}

	const FGameplayTag PresentFaction = OccupantCounts.CreateConstIterator().Key();
	if (PresentFaction == OwnerFaction)
	{
		// Defenders retain ownership. Any incomplete attack is canceled.
		if (CapturingFaction.IsValid() || CaptureProgress > 0.0f)
		{
			CapturingFaction = FGameplayTag();
			CaptureProgress = 0.0f;
			ForceNetUpdate();
		}
		RefreshStatusLabel();
		return;
	}

	if (CapturingFaction != PresentFaction)
	{
		CapturingFaction = PresentFaction;
		CaptureProgress = 0.0f;
	}

	const float Interval = FMath::Max(CaptureUpdateInterval, 0.1f);
	CaptureProgress = FMath::Clamp(
		CaptureProgress + Interval / FMath::Max(CaptureTimeSeconds, 0.1f),
		0.0f, 1.0f);

	if (CaptureProgress >= 1.0f)
	{
		OwnerFaction = PresentFaction;
		CapturingFaction = FGameplayTag();
		CaptureProgress = 0.0f;

		UE_LOG(LogTemp, Log, TEXT("CM territory point %s captured by %s"),
			*GetName(), *OwnerFaction.ToString());
		OnOwnerChanged.Broadcast(OwnerFaction);
		ForceNetUpdate();
	}

	RefreshStatusLabel();
}

void ACMContestedPoint::RefreshStatusLabel()
{
	if (!IsValid(StatusLabel))
	{
		return;
	}

	const FString OwnerText = OwnerFaction.IsValid()
		? OwnerFaction.GetTagName().ToString() : TEXT("Neutral");

	FString StateText = TEXT("Uncontested");
	if (bContested)
	{
		StateText = TEXT("CONTESTED - capture paused");
	}
	else if (CapturingFaction.IsValid())
	{
		StateText = FString::Printf(TEXT("Capturing: %s (%d%%)"),
			*CapturingFaction.GetTagName().ToString(),
			FMath::RoundToInt(CaptureProgress * 100.0f));
	}

	StatusLabel->SetText(FText::FromString(FString::Printf(
		TEXT("%s\nOwner: %s\n%s"),
		*PointName, *OwnerText, *StateText)));
}

void ACMContestedPoint::OnRep_OwnerFaction()
{
	OnOwnerChanged.Broadcast(OwnerFaction);
	RefreshStatusLabel();
}

void ACMContestedPoint::OnRep_CaptureState()
{
	RefreshStatusLabel();
}

void ACMContestedPoint::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACMContestedPoint, OwnerFaction);
	DOREPLIFETIME(ACMContestedPoint, CapturingFaction);
	DOREPLIFETIME(ACMContestedPoint, CaptureProgress);
	DOREPLIFETIME(ACMContestedPoint, bContested);
}

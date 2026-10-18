// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "Combat/CMCombatSoldier.h"

#include "AIController.h"
#include "Combat/CMFactionComponent.h"
#include "Combat/CMHealthComponent.h"
#include "ColonialMercenariesCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "TimerManager.h"

ACMCombatSoldier::ACMCombatSoldier()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	// Let UE's built-in AI Controller run pathfinding in the first prototype.
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(34.0f, 96.0f);
	// The stock Pawn capsule can ignore Visibility traces. Explicitly block
	// them so prototype hitscan shots actually register against soldiers.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetCanBeDamaged(true);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;

	FactionComponent = CreateDefaultSubobject<UCMFactionComponent>(TEXT("Faction"));
	HealthComponent = CreateDefaultSubobject<UCMHealthComponent>(TEXT("Health"));

	BodyVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeBody"));
	BodyVisual->SetupAttachment(GetCapsuleComponent());
	BodyVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyVisual->SetRelativeLocation(FVector(0.0f, 0.0f, -8.0f));
	BodyVisual->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.6f));

	// The simple cube is an engine asset, not an imported marketplace asset.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		BodyVisual->SetStaticMesh(Cube.Object);
	}

	FactionLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("FactionLabel"));
	FactionLabel->SetupAttachment(GetCapsuleComponent());
	FactionLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 125.0f));
	FactionLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	FactionLabel->SetWorldSize(42.0f);
	FactionLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Placeholder skeletal mesh intentionally hidden; can be replaced with
	// a Manny/Quinn character mesh through a derived Blueprint later.
	GetMesh()->SetVisibility(false);
}

void ACMCombatSoldier::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	HealthComponent->OnDied.AddDynamic(this, &ACMCombatSoldier::HandleDeath);
	HealthComponent->OnHealthChanged.AddDynamic(this, &ACMCombatSoldier::HandleHealthChanged);

	const FString FactionName = FactionComponent->GetFactionId().ToString();
	if (FactionName == TEXT("Faction.Prototype.Red"))
	{
		FactionLabel->SetText(FText::FromString(TEXT("RED")));
		FactionLabel->SetTextRenderColor(FColor::Red);
	}
	else if (FactionName == TEXT("Faction.Prototype.Blue"))
	{
		FactionLabel->SetText(FText::FromString(TEXT("BLUE")));
		FactionLabel->SetTextRenderColor(FColor::Cyan);
	}
	else
	{
		FactionLabel->SetText(FText::FromString(TEXT("NEUTRAL")));
		FactionLabel->SetTextRenderColor(FColor::White);
	}

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(
			DecisionTimerHandle, this, &ACMCombatSoldier::UpdateCombat,
			FMath::Max(DecisionInterval, 0.1f), true);
	}
}

bool ACMCombatSoldier::HasLineOfFire(const ACharacter* Target) const
{
	if (!IsValid(Target) || !GetWorld())
	{
		return false;
	}

	const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, AimHeight);
	const FVector End = Target->GetActorLocation()
		+ FVector(0.0f, 0.0f, AimHeight);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	FHitResult Hit;
	// A soldier can fire only if the first thing the ray strikes is its
	// intended enemy, not a wall or another soldier.
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
		Hit, Start, End, ECC_Visibility, Params);
	const bool bClear = bBlocked && Hit.GetActor() == Target;

	if (bDrawLineOfSight)
	{
		DrawDebugLine(GetWorld(), Start, bBlocked ? Hit.ImpactPoint : End,
			bClear ? FColor::Green : FColor::Red, false,
			FMath::Max(DecisionInterval, 0.1f), 0, 1.0f);
	}

	return bClear;
}

ACharacter* ACMCombatSoldier::FindClosestHostile() const
{
	UWorld* World = GetWorld();
	if (!World || !FactionComponent || !FactionComponent->GetFactionId().IsValid())
	{
		return nullptr;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectTypes;
	ObjectTypes.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	World->OverlapMultiByObjectType(
		Overlaps, GetActorLocation(), FQuat::Identity, ObjectTypes,
		FCollisionShape::MakeSphere(DetectionRange), QueryParams);

	ACharacter* ClosestVisible = nullptr;
	ACharacter* ClosestObstructed = nullptr;
	float VisibleDistanceSquared = FMath::Square(DetectionRange);
	float ObstructedDistanceSquared = FMath::Square(DetectionRange);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		ACharacter* Candidate = Cast<ACharacter>(Overlap.GetActor());
		if (!IsValid(Candidate) || Candidate == this)
		{
			continue;
		}

		// Soldiers and players share the same health and faction interfaces.
		// An unassigned player faction remains neutral. A prototype player
		// can be assigned Faction.Prototype.Blue in BP_ThirdPersonCharacter
		// to make red soldiers engage them (and vice versa).
		UCMHealthComponent* CandidateHealth = Candidate->FindComponentByClass<UCMHealthComponent>();
		UCMFactionComponent* CandidateFaction = Candidate->FindComponentByClass<UCMFactionComponent>();
		if (!IsValid(CandidateHealth) || CandidateHealth->IsDead()
			|| !FactionComponent->IsHostileTo(CandidateFaction))
		{
			continue;
		}

		const float DistSquared = FVector::DistSquared(
			GetActorLocation(), Candidate->GetActorLocation());
		if (DistSquared >= FMath::Square(DetectionRange))
		{
			continue;
		}

		// Prioritize an enemy we can actually shoot. Otherwise approach the
		// nearest known opposing soldier until a firing lane opens.
		if (HasLineOfFire(Candidate))
		{
			if (DistSquared < VisibleDistanceSquared)
			{
				VisibleDistanceSquared = DistSquared;
				ClosestVisible = Candidate;
			}
		}
		else if (DistSquared < ObstructedDistanceSquared)
		{
			ObstructedDistanceSquared = DistSquared;
			ClosestObstructed = Candidate;
		}
	}

	return ClosestVisible ? ClosestVisible : ClosestObstructed;
}

void ACMCombatSoldier::UpdateCombat()
{
	if (!HasAuthority() || HealthComponent->IsDead())
	{
		return;
	}

	AAIController* AI = Cast<AAIController>(GetController());
	if (!AI && !bReportedMissingController)
	{
		bReportedMissingController = true;
		UE_LOG(LogTemp, Warning,
			TEXT("CM combat soldier %s has no AIController; confirm Auto Possess AI is Placed in World or Spawned."),
			*GetName());
	}

	ACharacter* Target = FindClosestHostile();
	if (!IsValid(Target))
	{
		CurrentTarget.Reset();
		bLastMoveWasBlockedApproach = false;
		if (AI) { AI->StopMovement(); }
		return;
	}

	const float TimeNow = GetWorld()->GetTimeSeconds();
	const float DistanceSquared = FVector::DistSquared(
		GetActorLocation(), Target->GetActorLocation());
	const bool bClearShot = HasLineOfFire(Target);

	if (DistanceSquared <= FMath::Square(WeaponRange) && bClearShot)
	{
		// An unobstructed target is within weapon range: hold position, fire.
		if (AI) { AI->StopMovement(); }
		bLastMoveWasBlockedApproach = false;

		if (TimeNow >= NextShotTime)
		{
			FireAt(Target);
			NextShotTime = TimeNow + 1.0f / FMath::Max(ShotsPerSecond, 0.1f);
		}
	}
	else
	{
		// At range with no line of fire, move toward the target on NavMesh
		// rather than repeatedly firing into a wall. If we have a firing
		// lane but are out of range, approach only to normal weapon range.
		const bool bBlockedApproach = !bClearShot;
		const float AcceptanceRadius = bBlockedApproach
			? FMath::Clamp(BlockedApproachDistance, 50.0f, FMath::Max(50.0f, WeaponRange * 0.75f))
			: WeaponRange * 0.8f;

		if (AI)
		{
			const bool bNewTarget = CurrentTarget.Get() != Target;
			const bool bChangedApproach = bLastMoveWasBlockedApproach != bBlockedApproach;
			const bool bStoppedMoving = AI->GetMoveStatus() != EPathFollowingStatus::Moving;

			if (bNewTarget || bChangedApproach
				|| (bStoppedMoving && TimeNow >= NextMoveRequestTime))
			{
				const EPathFollowingRequestResult::Type MoveResult =
					AI->MoveToActor(Target, AcceptanceRadius);
				NextMoveRequestTime = TimeNow + FMath::Max(PathRetryInterval, 0.1f);
				if (MoveResult == EPathFollowingRequestResult::Failed)
				{
					if (!bReportedMoveFailure)
					{
						UE_LOG(LogTemp, Warning,
							TEXT("CM combat soldier %s cannot navigate to %s. Confirm connected NavMesh around obstacles."),
							*GetName(), *Target->GetName());
						bReportedMoveFailure = true;
					}
				}
				else
				{
					bReportedMoveFailure = false;
				}
			}
		}
		bLastMoveWasBlockedApproach = bBlockedApproach;
	}

	CurrentTarget = Target;
}

void ACMCombatSoldier::FireAt(ACharacter* Target)
{
	if (!HasAuthority() || !IsValid(Target))
	{
		return;
	}

	const UCMHealthComponent* TargetHealth = Target->FindComponentByClass<UCMHealthComponent>();
	if (!IsValid(TargetHealth) || TargetHealth->IsDead())
	{
		return;
	}

	const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, AimHeight);
	const FVector End = Target->GetActorLocation() + FVector(0.0f, 0.0f, AimHeight);

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);

	FHitResult Hit;
	GetWorld()->LineTraceSingleByChannel(
		Hit, Start, End, ECC_Visibility, TraceParams);

	// Full friendly fire: any intervening damageable actor can be struck,
	// regardless of whether the intended target is on an opposing faction.
	if (AActor* HitActor = Hit.GetActor())
	{
		if (IsValid(HitActor->FindComponentByClass<UCMHealthComponent>()))
		{
			UGameplayStatics::ApplyDamage(
				HitActor, ShotDamage, GetController(), this, UDamageType::StaticClass());
		}
	}

	MulticastDrawShot(Start, Hit.bBlockingHit ? Hit.ImpactPoint : End);
}

void ACMCombatSoldier::MulticastDrawShot_Implementation(FVector Start, FVector End)
{
	// Temporary combat visualization, not the final tracer/VFX system.
	DrawDebugLine(GetWorld(), Start, End, FColor::Yellow, false, 0.22f, 0, 2.0f);
}

void ACMCombatSoldier::HandleHealthChanged(float NewHealth, float NewMaxHealth)
{
	if (HasAuthority())
	{
		UE_LOG(LogTemp, Log, TEXT("CM combat soldier %s health: %.0f / %.0f"),
			*GetName(), NewHealth, NewMaxHealth);
	}
}

void ACMCombatSoldier::HandleDeath()
{
	GetWorldTimerManager().ClearTimer(DecisionTimerHandle);
	BodyVisual->SetVisibility(false);
	FactionLabel->SetVisibility(false);
	SetActorEnableCollision(false);

	if (HasAuthority())
	{
		if (AAIController* AI = Cast<AAIController>(GetController()))
		{
			AI->StopMovement();
		}
		SetLifeSpan(FMath::Max(CorpseLifetime, 0.1f));
	}
}

// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "Combat/CMCombatSoldier.h"

#include "AIController.h"
#include "Combat/CMFactionComponent.h"
#include "Combat/CMHealthComponent.h"
#include "ColonialMercenariesCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Net/UnrealNetwork.h"
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

	// The stock cube is retained only as an asset-loading fallback.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cube.Succeeded())
	{
		BodyVisual->SetStaticMesh(Cube.Object);
	}

	// Existing project mannequin and rifle Animation Blueprint are shared
	// with the player. Both can be overridden by a derived infantry BP.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMesh(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> RifleAnimBP(
		TEXT("/Game/ColonialMercenaries/Animation/ABP_CM_Rifle"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> RifleFireMontage(
		TEXT("/Game/ColonialMercenaries/Animation/AM_CM_RifleFire.AM_CM_RifleFire"));

	if (MannyMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(MannyMesh.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BodyVisual->SetVisibility(false);
	}
	if (RifleAnimBP.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(RifleAnimBP.Class);
	}
	if (RifleFireMontage.Succeeded())
	{
		FireMontage = RifleFireMontage.Object;
	}

	// The player and the NPCs use compatible hand_r attachments and
	// left-hand support IK. Meshes do not influence hit detection or cover.
	PrototypeRifleRoot = CreateDefaultSubobject<USceneComponent>(TEXT("InfantryRifleRoot"));
	PrototypeRifleRoot->SetupAttachment(GetMesh(), TEXT("hand_r"));
	PrototypeRifleRoot->SetRelativeLocation(FVector(4.0f, -8.0f, -3.0f));
	PrototypeRifleRoot->SetRelativeRotation(FRotator(0.0f, 0.0f, -90.0f));

	PrototypeRifleSupportGrip = CreateDefaultSubobject<USceneComponent>(
		TEXT("InfantryRifleSupportGrip"));
	PrototypeRifleSupportGrip->SetupAttachment(PrototypeRifleRoot);
	PrototypeRifleSupportGrip->SetRelativeLocation(FVector(29.0f, 0.0f, -8.0f));

	PrototypeRifleMuzzle = CreateDefaultSubobject<USceneComponent>(
		TEXT("InfantryRifleMuzzle"));
	PrototypeRifleMuzzle->SetupAttachment(PrototypeRifleRoot);
	PrototypeRifleMuzzle->SetRelativeLocation(FVector(70.0f, 0.0f, 0.0f));

	RifleReceiver = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("InfantryRifleReceiver"));
	RifleReceiver->SetupAttachment(PrototypeRifleRoot);
	RifleReceiver->SetRelativeLocation(FVector(10.0f, 0.0f, 0.0f));
	RifleReceiver->SetRelativeScale3D(FVector(0.5f, 0.08f, 0.09f));

	RifleBarrel = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("InfantryRifleBarrel"));
	RifleBarrel->SetupAttachment(PrototypeRifleRoot);
	RifleBarrel->SetRelativeLocation(FVector(48.0f, 0.0f, 0.0f));
	RifleBarrel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	RifleBarrel->SetRelativeScale3D(FVector(0.03f, 0.03f, 0.43f));

	RifleStock = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("InfantryRifleStock"));
	RifleStock->SetupAttachment(PrototypeRifleRoot);
	RifleStock->SetRelativeLocation(FVector(-24.0f, 0.0f, -2.0f));
	RifleStock->SetRelativeScale3D(FVector(0.22f, 0.065f, 0.085f));

	RifleGrip = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("InfantryRifleGrip"));
	RifleGrip->SetupAttachment(PrototypeRifleRoot);
	RifleGrip->SetRelativeLocation(FVector(2.0f, 0.0f, -12.0f));
	RifleGrip->SetRelativeScale3D(FVector(0.075f, 0.055f, 0.15f));

	for (UStaticMeshComponent* WeaponPart :
		{ RifleReceiver.Get(), RifleStock.Get(), RifleGrip.Get() })
	{
		WeaponPart->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (Cube.Succeeded())
		{
			WeaponPart->SetStaticMesh(Cube.Object);
		}
	}
	RifleBarrel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (Cylinder.Succeeded())
	{
		RifleBarrel->SetStaticMesh(Cylinder.Object);
	}

	FactionLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("FactionLabel"));
	FactionLabel->SetupAttachment(GetCapsuleComponent());
	FactionLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 125.0f));
	FactionLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	FactionLabel->SetWorldSize(42.0f);
	FactionLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// The mannequin is now the primary infantry body, not the old cube.
	GetMesh()->SetVisibility(MannyMesh.Succeeded());
}

FVector ACMCombatSoldier::GetPrototypeRifleSupportGripInRightHandSpace() const
{
	if (!IsValid(PrototypeRifleRoot) || !IsValid(PrototypeRifleSupportGrip))
	{
		return FVector::ZeroVector;
	}

	// Same bone-space convention as the player rifle's support-hand IK.
	return PrototypeRifleRoot->GetRelativeTransform().TransformPosition(
		PrototypeRifleSupportGrip->GetRelativeLocation());
}

void ACMCombatSoldier::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACMCombatSoldier, CombatAimRotation);
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
		// While firing, stop and face the actual target so upper-body rifle
		// aim and the arm-mounted gun follow its position rather than the
		// previous nav movement heading. NPC damage calculations remain in
		// FireAt's authoritative center-of-mass trace for this milestone.
		if (AI) { AI->StopMovement(); }
		const FVector ShotOrigin =
			GetActorLocation() + FVector(0.0f, 0.0f, AimHeight);
		const FVector AimAt =
			Target->GetActorLocation() + FVector(0.0f, 0.0f, AimHeight);
		const FVector Direction = (AimAt - ShotOrigin).GetSafeNormal();
		if (!Direction.IsNearlyZero())
		{
			CombatAimRotation = Direction.Rotation();
			SetActorRotation(FRotator(0.0f, CombatAimRotation.Yaw, 0.0f));
		}
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
	// Continue showing the existing networked debug tracer. NPC shots are
	// still resolved by the server, not driven by the montage.
	DrawDebugLine(GetWorld(), Start, End, FColor::Yellow, false, 0.22f, 0, 2.0f);

	// Montage runs cosmetically on each machine with a relevant soldier.
	// The original DefaultSlot in ABP_CM_Rifle remains after the aim layer,
	// allowing short upper-body recoil without interrupting locomotion.
	if (IsValid(FireMontage) && IsValid(GetMesh()))
	{
		if (UAnimInstance* Anim = GetMesh()->GetAnimInstance())
		{
			Anim->Montage_Play(FireMontage, 1.0f);
		}
	}
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
	GetMesh()->SetVisibility(false);
	if (IsValid(PrototypeRifleRoot))
	{
		PrototypeRifleRoot->SetVisibility(false, true);
	}
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

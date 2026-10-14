// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "Combat/CMCombatSoldier.h"

#include "AIController.h"
#include "Combat/CMFactionComponent.h"
#include "Combat/CMHealthComponent.h"
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

ACMCombatSoldier* ACMCombatSoldier::FindClosestHostile() const
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

	ACMCombatSoldier* BestEnemy = nullptr;
	float BestDistanceSquared = DetectionRange * DetectionRange;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		ACMCombatSoldier* Candidate = Cast<ACMCombatSoldier>(Overlap.GetActor());
		if (!IsValid(Candidate) || Candidate == this
			|| !Candidate->HealthComponent || Candidate->HealthComponent->IsDead()
			|| !FactionComponent->IsHostileTo(Candidate->FactionComponent))
		{
			continue;
		}

		const float DistSquared = FVector::DistSquared(
			GetActorLocation(), Candidate->GetActorLocation());
		if (DistSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistSquared;
			BestEnemy = Candidate;
		}
	}

	return BestEnemy;
}

void ACMCombatSoldier::UpdateCombat()
{
	if (!HasAuthority() || HealthComponent->IsDead())
	{
		return;
	}

	AAIController* AI = Cast<AAIController>(GetController());
	ACMCombatSoldier* Target = FindClosestHostile();

	if (!IsValid(Target))
	{
		CurrentTarget.Reset();
		if (AI) { AI->StopMovement(); }
		return;
	}

	const float DistSquared = FVector::DistSquared(
		GetActorLocation(), Target->GetActorLocation());

	if (DistSquared > FMath::Square(WeaponRange))
	{
		if (AI && (CurrentTarget.Get() != Target
			|| AI->GetMoveStatus() != EPathFollowingStatus::Moving))
		{
			AI->MoveToActor(Target, WeaponRange * 0.8f);
		}
	}
	else
	{
		if (AI) { AI->StopMovement(); }
		if (GetWorld()->GetTimeSeconds() >= NextShotTime)
		{
			FireAt(Target);
			NextShotTime = GetWorld()->GetTimeSeconds()
				+ 1.0f / FMath::Max(ShotsPerSecond, 0.1f);
		}
	}

	CurrentTarget = Target;
}

void ACMCombatSoldier::FireAt(ACMCombatSoldier* Target)
{
	if (!HasAuthority() || !IsValid(Target) || Target->HealthComponent->IsDead())
	{
		return;
	}

	const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, AimHeight);
	const FVector End = Target->GetActorLocation() + FVector(0.0f, 0.0f, Target->AimHeight);

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

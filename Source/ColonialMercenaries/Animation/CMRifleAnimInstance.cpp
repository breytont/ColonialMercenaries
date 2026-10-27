// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "Animation/CMRifleAnimInstance.h"

#include "ColonialMercenariesCharacter.h"
#include "Combat/CMCombatSoldier.h"
#include "Combat/CMHealthComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UCMRifleAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	RefreshCharacter();
}

void UCMRifleAnimInstance::RefreshCharacter()
{
	CachedCharacter = Cast<ACharacter>(TryGetPawnOwner());
}

void UCMRifleAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!CachedCharacter.IsValid())
	{
		RefreshCharacter();
	}

	const ACharacter* Character = CachedCharacter.Get();
	if (!IsValid(Character))
	{
		CMIsAiming = false;
		CMRifleAimAlpha = 0.0f;
		CMShoulderAimAlpha = 0.0f;
		CMGroundSpeed = 0.0f;
		CMMovementDirection = 0.0f;
		CMIsInAir = false;
		CMAimPitch = 0.0f;
		CMAimYaw = 0.0f;
		CMLeftHandGripInRightHandSpace = FVector::ZeroVector;
		return;
	}

	const FVector Velocity = Character->GetVelocity();
	const FVector PlanarVelocity(Velocity.X, Velocity.Y, 0.0f);
	CMGroundSpeed = PlanarVelocity.Size();

	if (CMGroundSpeed > KINDA_SMALL_NUMBER)
	{
		const FVector LocalVelocity =
			Character->GetActorTransform().InverseTransformVectorNoScale(PlanarVelocity);
		CMMovementDirection = FMath::RadiansToDegrees(
			FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
	}
	else
	{
		CMMovementDirection = 0.0f;
	}

	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	CMIsInAir = IsValid(Movement) && Movement->IsFalling();

	// The same authored AnimGraph and left-hand Two Bone IK can now be
	// used by the player and the factions' combat soldiers. The player
	// continues to drive its own RMB aim pose and full view rotation.
	FRotator AimRotation = Character->GetBaseAimRotation();
	if (const AColonialMercenariesCharacter* PlayerCharacter =
		Cast<AColonialMercenariesCharacter>(Character))
	{
		CMIsAiming = PlayerCharacter->IsAiming();
		CMLeftHandGripInRightHandSpace =
			PlayerCharacter->GetPrototypeRifleSupportGripInRightHandSpace();
	}
	else if (const ACMCombatSoldier* Soldier = Cast<ACMCombatSoldier>(Character))
	{
		// Infantry carries its rifle at the ready during movement and combat.
		// Its server-aimed orientation is replicated for correct pitch on
		// non-owning clients, whose AIControllers do not exist locally.
		const UCMHealthComponent* Health = Soldier->GetHealthComponent();
		CMIsAiming = !IsValid(Health) || !Health->IsDead();
		CMLeftHandGripInRightHandSpace =
			Soldier->GetPrototypeRifleSupportGripInRightHandSpace();
		AimRotation = Soldier->GetCombatAimRotation();
	}
	else
	{
		CMIsAiming = false;
		CMLeftHandGripInRightHandSpace = FVector::ZeroVector;
	}

	// The player always has a rifle equipped in this prototype.
	// Keep the existing upper-body rifle pose active at the ready even
	// when RMB is released. Otherwise the unarmed locomotion pose lowers
	// hand_r (and the attached real muzzle) toward the floor, making true
	// muzzle-forward hip shots fire into the ground.
	//
	// CMIsAiming retains its real RMB state for camera/ADS behavior. The
	// same barrel-forward weapon now works from both non-ADS and ADS.
	// A separate lower hip-ready pose and weapon-equipped state can be
	// authored once the equipment/locomotion system expands.
	const bool bPlayerWithRifle =
		Cast<AColonialMercenariesCharacter>(Character) != nullptr;
	const float DesiredRifleAlpha = (bPlayerWithRifle || CMIsAiming)
		? 1.0f : 0.0f;
	CMRifleAimAlpha = FMath::FInterpTo(
		CMRifleAimAlpha, DesiredRifleAlpha,
		FMath::Max(DeltaSeconds, 0.0f),
		FMath::Max(AimPoseBlendSpeed, 0.1f));

	// Distinguish the active rifle from *how* it is held. This scalar
	// lets the same AnimGraph blend an authored at-hip pose into the
	// existing ADS/AO_Rifle upper-body pose, without reverting to unarmed.
	CMShoulderAimAlpha = FMath::FInterpTo(
		CMShoulderAimAlpha, CMIsAiming ? 1.0f : 0.0f,
		FMath::Max(DeltaSeconds, 0.0f),
		FMath::Max(AimPoseBlendSpeed, 0.1f));

	CMAimPitch = FMath::Clamp(
		FRotator::NormalizeAxis(AimRotation.Pitch), -90.0f, 90.0f);
	CMAimYaw = FMath::FindDeltaAngleDegrees(
		Character->GetActorRotation().Yaw, AimRotation.Yaw);
}

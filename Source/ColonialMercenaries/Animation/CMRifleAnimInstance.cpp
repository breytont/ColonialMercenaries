// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "Animation/CMRifleAnimInstance.h"

#include "ColonialMercenariesCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UCMRifleAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	RefreshCharacter();
}

void UCMRifleAnimInstance::RefreshCharacter()
{
	CachedCharacter = Cast<AColonialMercenariesCharacter>(TryGetPawnOwner());
}

void UCMRifleAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!CachedCharacter.IsValid())
	{
		RefreshCharacter();
	}

	const AColonialMercenariesCharacter* Character = CachedCharacter.Get();
	if (!IsValid(Character))
	{
		CMIsAiming = false;
		CMRifleAimAlpha = 0.0f;
		CMGroundSpeed = 0.0f;
		CMMovementDirection = 0.0f;
		CMIsInAir = false;
		CMAimPitch = 0.0f;
		CMAimYaw = 0.0f;
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

	CMIsAiming = Character->IsAiming();
	CMRifleAimAlpha = FMath::FInterpTo(
		CMRifleAimAlpha, CMIsAiming ? 1.0f : 0.0f,
		FMath::Max(DeltaSeconds, 0.0f),
		FMath::Max(AimPoseBlendSpeed, 0.1f));

	// GetBaseAimRotation works with both locally controlled players and
	// simulated proxies (using Unreal's replicated remote view pitch).
	const FRotator AimRotation = Character->GetBaseAimRotation();
	CMAimPitch = FMath::Clamp(
		FRotator::NormalizeAxis(AimRotation.Pitch), -90.0f, 90.0f);
	CMAimYaw = FMath::FindDeltaAngleDegrees(
		Character->GetActorRotation().Yaw, AimRotation.Yaw);
}

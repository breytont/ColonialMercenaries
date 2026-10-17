// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "Combat/CMPrototypeWeaponComponent.h"
#include "ColonialMercenariesCharacter.h"
#include "Components/SceneComponent.h"

#include "Combat/CMHealthComponent.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"

UCMPrototypeWeaponComponent::UCMPrototypeWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCMPrototypeWeaponComponent::RequestFire()
{
	const AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	if (Owner->HasAuthority())
	{
		FireOnAuthority();
	}
	else
	{
		ServerRequestFire();
	}
}

void UCMPrototypeWeaponComponent::ServerRequestFire_Implementation()
{
	FireOnAuthority();
}

void UCMPrototypeWeaponComponent::FireOnAuthority()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!IsValid(Character) || !Character->HasAuthority() || !World
		|| !IsValid(Character->GetController()))
	{
		return;
	}

	const float Now = World->GetTimeSeconds();
	if (Now < NextAllowedShotTime)
	{
		return; // Server-side fire-rate enforcement.
	}
	NextAllowedShotTime = Now + FMath::Max(FireInterval, 0.05f);

	const FRotator ViewRotation = Character->GetController()->GetControlRotation();
	const float Range = FMath::Max(MaxRange, 100.0f);

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(Character);

	// A server-authoritative muzzle trace starts at the physical barrel tip
	// whenever the character has a usable visual weapon. This is also the
	// tracer origin, so shots can strike the floor or nearby cover naturally.
	FVector MuzzleOrigin = Character->GetActorLocation()
		+ FRotator(0.0f, ViewRotation.Yaw, 0.0f).RotateVector(MuzzleOffset);
	FVector ShotDirection = FVector::ZeroVector;

	if (bUseVisualRifleMuzzle)
	{
		if (const AColonialMercenariesCharacter* PlayerCharacter =
			Cast<AColonialMercenariesCharacter>(Character))
		{
			if (const USceneComponent* Muzzle = PlayerCharacter->GetPrototypeRifleMuzzle())
			{
				if (Muzzle->IsRegistered())
				{
					MuzzleOrigin = Muzzle->GetComponentLocation();
					if (bFireAlongMuzzleForward)
					{
						// Unreal scene-component forward is its local +X axis.
						// The prototype rifle barrel and muzzle marker are built on +X.
						ShotDirection = Muzzle->GetForwardVector().GetSafeNormal();
					}
				}
			}
		}
	}

	if (ShotDirection.IsNearlyZero())
	{
		// Configurable fallback: align from the muzzle toward a camera trace
		// for prototype aiming or when no equipped muzzle component is present.
		// This path still performs a second muzzle trace for obstruction.
		FVector ViewOrigin = Character->GetPawnViewLocation();
		if (const UCameraComponent* Camera = Character->FindComponentByClass<UCameraComponent>())
		{
			ViewOrigin = Camera->GetComponentLocation();
		}

		FHitResult AimHit;
		const FVector CameraEnd = ViewOrigin + ViewRotation.Vector() * Range;
		const bool bCameraHit = World->LineTraceSingleByChannel(
			AimHit, ViewOrigin, CameraEnd, ECC_Visibility, TraceParams);
		const FVector DesiredImpact = bCameraHit ? AimHit.ImpactPoint : CameraEnd;
		ShotDirection = (DesiredImpact - MuzzleOrigin).GetSafeNormal();
	}

	if (ShotDirection.IsNearlyZero())
	{
		return;
	}

	FHitResult ShotHit;
	const FVector MuzzleEnd = MuzzleOrigin + ShotDirection * Range;
	const bool bMuzzleHit = World->LineTraceSingleByChannel(
		ShotHit, MuzzleOrigin, MuzzleEnd, ECC_Visibility, TraceParams);
	const FVector TracerEnd = bMuzzleHit ? ShotHit.ImpactPoint : MuzzleEnd;

	if (AActor* HitActor = ShotHit.GetActor())
	{
		if (UCMHealthComponent* Health = HitActor->FindComponentByClass<UCMHealthComponent>())
		{
			if (!Health->IsDead())
			{
				UGameplayStatics::ApplyDamage(
					HitActor, FMath::Max(BaseDamage, 0.0f),
					Character->GetController(), Character, UDamageType::StaticClass());
			}
		}
	}

	MulticastShotTracer(MuzzleOrigin, TracerEnd);
}

void UCMPrototypeWeaponComponent::MulticastShotTracer_Implementation(
	FVector_NetQuantize Start, FVector_NetQuantize End)
{
	if (bDrawShotTracers && GetWorld())
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Cyan, false, 0.2f, 0, 2.5f);
	}
}

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
	const FVector Forward = ViewRotation.Vector();
	const float Range = FMath::Max(MaxRange, 100.0f);

	// First aim from the over-the-shoulder camera so a shot follows the
	// center of the player's screen, not the character's movement direction.
	// We use server-computed camera/controller state, never client hit claims.
	FVector ViewOrigin = Character->GetPawnViewLocation();
	if (const UCameraComponent* Camera = Character->FindComponentByClass<UCameraComponent>())
	{
		ViewOrigin = Camera->GetComponentLocation();
	}

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(Character);

	FHitResult AimHit;
	const FVector CameraEnd = ViewOrigin + Forward * Range;
	const bool bCameraHit = World->LineTraceSingleByChannel(
		AimHit, ViewOrigin, CameraEnd, ECC_Visibility, TraceParams);
	const FVector DesiredImpact = bCameraHit ? AimHit.ImpactPoint : CameraEnd;

	// A second trace from a plausible weapon location prevents third-person
	// cameras from granting shots through intervening walls or solid cover.
	FVector MuzzleOrigin = Character->GetActorLocation()
		+ FRotator(0.0f, ViewRotation.Yaw, 0.0f).RotateVector(MuzzleOffset);

	// Player characters have a muzzle transform attached to the hand-held
	// prototype rifle. Reuse it for hit collision and tracers so the gun
	// doesn't appear to fire out of the character's chest.
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
				}
			}
		}
	}
	const FVector ShotDirection = (DesiredImpact - MuzzleOrigin).GetSafeNormal();
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

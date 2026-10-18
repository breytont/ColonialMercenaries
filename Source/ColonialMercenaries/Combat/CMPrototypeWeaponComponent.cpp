// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "Combat/CMPrototypeWeaponComponent.h"
#include "ColonialMercenariesCharacter.h"
#include "Components/SceneComponent.h"

#include "Combat/CMHealthComponent.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"
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

bool UCMPrototypeWeaponComponent::TraceShot(
	FVector& OutStart, FVector& OutEnd, FHitResult& OutHit) const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!IsValid(Character) || !World || !IsValid(Character->GetController()))
	{
		return false;
	}

	const FRotator ViewRotation = Character->GetController()->GetControlRotation();
	const float Range = FMath::Max(MaxRange, 100.0f);

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(Character);

	// Actual gun muzzle first, including its physical orientation when the
	// pose is lowered, obstructed, or aiming above/below the horizon.
	OutStart = Character->GetActorLocation()
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
					OutStart = Muzzle->GetComponentLocation();
					if (bFireAlongMuzzleForward)
					{
						ShotDirection = Muzzle->GetForwardVector().GetSafeNormal();
					}
				}
			}
		}
	}

	if (ShotDirection.IsNearlyZero())
	{
		// Existing optional camera-assisted direction remains available for
		// designers, while still starting shots from the actual muzzle.
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
		ShotDirection = (DesiredImpact - OutStart).GetSafeNormal();
	}

	if (ShotDirection.IsNearlyZero())
	{
		return false;
	}

	const FVector ShotRangeEnd = OutStart + ShotDirection * Range;
	const bool bBlockingHit = World->LineTraceSingleByChannel(
		OutHit, OutStart, ShotRangeEnd, ECC_Visibility, TraceParams);
	OutEnd = bBlockingHit ? OutHit.ImpactPoint : ShotRangeEnd;
	return true;
}

bool UCMPrototypeWeaponComponent::GetPredictedShot(
	FVector& OutStart, FVector& OutEnd, bool& bBlockingHit) const
{
	FHitResult PredictedHit;
	bBlockingHit = false;
	if (!TraceShot(OutStart, OutEnd, PredictedHit))
	{
		return false;
	}
	bBlockingHit = PredictedHit.bBlockingHit;
	return true;
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

	// Client-side input is also guarded, but the authority must reject
	// delayed/forged RPCs from incapacitated players.
	if (const UCMHealthComponent* Health = Character->FindComponentByClass<UCMHealthComponent>())
	{
		if (Health->IsDead())
		{
			return;
		}
	}

	const float Now = World->GetTimeSeconds();
	if (Now < NextAllowedShotTime)
	{
		return;
	}
	NextAllowedShotTime = Now + FMath::Max(FireInterval, 0.05f);

	FVector MuzzleOrigin;
	FVector TracerEnd;
	FHitResult ShotHit;
	if (!TraceShot(MuzzleOrigin, TracerEnd, ShotHit))
	{
		return;
	}
	const bool bMuzzleHit = ShotHit.bBlockingHit;

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

	// Only server-approved shots trigger effects or recoil.
	MulticastShotFeedback(MuzzleOrigin, TracerEnd, bMuzzleHit);
	ClientApplyRecoil();
}

void UCMPrototypeWeaponComponent::MulticastShotFeedback_Implementation(
	FVector_NetQuantize Start, FVector_NetQuantize End, bool bBlockingHit)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Shot effects follow world positions generated by authoritative hitscan.
	// Visual-only debug geometry can be replaced with Niagara through the
	// BlueprintAssignable OnShotVisuals event.
	if (bDrawShotTracers)
	{
		DrawDebugLine(World, Start, End, FColor::Cyan, false, 0.18f, 0, 2.5f);
	}

	if (bDrawMuzzleFlash)
	{
		DrawDebugSphere(World, Start, 9.0f, 8, FColor::Orange, false, 0.07f, 0, 1.5f);
	}

	if (bDrawImpactMarks && bBlockingHit)
	{
		DrawDebugPoint(World, End, 13.0f, FColor::Yellow, false, 0.25f, 0);
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (FireMontage && IsValid(Character) && IsValid(Character->GetMesh()))
	{
		if (UAnimInstance* Anim = Character->GetMesh()->GetAnimInstance())
		{
			Anim->Montage_Play(FireMontage, 1.0f);
		}
	}

	// Location-based audio is audible to each local listener as the shot
	// multicast reaches their machine. Attenuation (if configured) controls
	// spatialization and falloff. No dedicated-server-only sound is required.
	if (GunshotSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, GunshotSound, FVector(Start), 1.0f, 1.0f, 0.0f, ShotAttenuation);
	}

	if (bBlockingHit && ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, ImpactSound, FVector(End), 1.0f, 1.0f, 0.0f, ShotAttenuation);
	}

	// Cheap local-client near miss: find closest point along the *resolved*
	// hitscan segment to the listener. No actor overlap or per-frame scanning.
	// Every machine evaluates only its own local player, so a distant whizz
	// is never played globally for all four co-op players.
	if (NearMissSound)
	{
		const APawn* LocalPawn = UGameplayStatics::GetPlayerPawn(this, 0);
		if (IsValid(LocalPawn) && LocalPawn != GetOwner())
		{
			const FVector EarPosition = LocalPawn->GetPawnViewLocation();
			const FVector ClosestPoint = FMath::ClosestPointOnSegment(
				EarPosition, FVector(Start), FVector(End));
			const float Distance = FVector::Dist(EarPosition, ClosestPoint);
			if (Distance >= FMath::Max(0.0f, NearMissMinDistance)
				&& Distance <= FMath::Max(NearMissMinDistance, NearMissMaxDistance))
			{
				UGameplayStatics::PlaySoundAtLocation(
					this, NearMissSound, ClosestPoint, 1.0f, 1.0f, 0.0f, ShotAttenuation);
			}
		}
	}

	OnShotVisuals.Broadcast(FVector(Start), FVector(End), bBlockingHit);
}

void UCMPrototypeWeaponComponent::ClientApplyRecoil_Implementation()
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!IsValid(Character) || !Character->IsLocallyControlled())
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	if (!IsValid(PlayerController))
	{
		return;
	}

	// Cosmetic first-person controller feedback only. This cannot affect
	// the *already resolved* server trace for the current shot.
	if (RecoilPitchDegrees > 0.0f)
	{
		PlayerController->AddPitchInput(-RecoilPitchDegrees);
	}
	if (RecoilYawDegrees > 0.0f)
	{
		PlayerController->AddYawInput(
			FMath::FRandRange(-RecoilYawDegrees, RecoilYawDegrees));
	}
}

// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/NetSerialization.h"
#include "CMPrototypeWeaponComponent.generated.h"

class UAnimMontage;
class USoundBase;
class USoundAttenuation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FCMPrototypeShotVisuals, FVector, Start, FVector, End, bool, bBlockingHit);

/**
 * Initial player-controlled hitscan firearm.
 *
 * Input is handled by the owning character. Clients request a shot, but
 * the server determines the aim ray, obstruction, damage and fire-rate limit.
 * This temporary weapon component can later delegate to data-driven weapon
 * definitions without changing player input or damage ownership.
 */
UCLASS(ClassGroup=(ColonialMercenaries), meta=(BlueprintSpawnableComponent))
class COLONIALMERCENARIES_API UCMPrototypeWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCMPrototypeWeaponComponent();

	/** Call when the owning player presses Fire. The server decides the hit. */
	UFUNCTION(BlueprintCallable, Category="Colonial Mercenaries|Weapon")
	void RequestFire();

	/** Cosmetic event dispatched once per approved shot on relevant clients.
	 * Blueprints can attach Niagara muzzle flashes, decals, and hit indicators.
	 */
	UPROPERTY(BlueprintAssignable, Category="Colonial Mercenaries|Weapon|Presentation")
	FCMPrototypeShotVisuals OnShotVisuals;

protected:
	/** Base damage of a successful hit on an actor with a Health component. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Weapon|Tuning",
		meta=(ClampMin="0.0", UIMin="0.0"))
	float BaseDamage = 40.0f;

	/** Maximum range in Unreal units. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Weapon|Tuning",
		meta=(ClampMin="100.0", UIMin="100.0"))
	float MaxRange = 15000.0f;

	/** Minimum seconds between server-approved shots; no ammo system yet. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Weapon|Tuning",
		meta=(ClampMin="0.05", UIMin="0.05"))
	float FireInterval = 0.2f;

	/** Prototype shot origin relative to character center, rotated with yaw. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Weapon|Tuning")
	FVector MuzzleOffset = FVector(35.0f, 20.0f, 55.0f);

	/** Use the player character's visible rifle muzzle for shot origin.
	 *  Disable to fall back to the fixed Muzzle Offset for troubleshooting.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Weapon|Tuning")
	bool bUseVisualRifleMuzzle = true;

	/** When using the visual muzzle, fire along its +X / Forward axis.
	 * This is physical barrel-direction aiming, not camera auto-correction.
	 * Disable to use the earlier camera-assisted muzzle-to-target trace.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Weapon|Tuning")
	bool bFireAlongMuzzleForward = true;

	/** Display temporary cyan tracer lines. Weapon VFX come later. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Weapon|Debug")
	bool bDrawShotTracers = true;

	/** Optional firing montage on the mannequin (uses DefaultSlot). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Weapon|Presentation")
	TObjectPtr<UAnimMontage> FireMontage;

	/** 3D gunshot emitted at the actual muzzle position. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Weapon|Audio")
	TObjectPtr<USoundBase> GunshotSound;

	/** 3D impact emitted only when the ray hits a blocking object. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Weapon|Audio")
	TObjectPtr<USoundBase> ImpactSound;

	/** Optional incoming near-miss sound heard by a local player near a
	 * shot segment. Hitscan approximation; no ballistic travel time yet.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Weapon|Audio")
	TObjectPtr<USoundBase> NearMissSound;

	/** Near miss plays only in this distance band from a listener's view. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Weapon|Audio",
		meta=(ClampMin="0.0"))
	float NearMissMinDistance = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Weapon|Audio",
		meta=(ClampMin="0.0"))
	float NearMissMaxDistance = 230.0f;

	/** Optional shared attenuation asset for falloff, spatialization and occlusion. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Weapon|Audio")
	TObjectPtr<USoundAttenuation> ShotAttenuation;

	/** Owning player's instantaneous upward camera kick per approved shot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Weapon|Recoil",
		meta=(ClampMin="0.0", UIMin="0.0"))
	float RecoilPitchDegrees = 0.65f;

	/** Maximum random horizontal camera kick; defaults to subtle. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Weapon|Recoil",
		meta=(ClampMin="0.0", UIMin="0.0"))
	float RecoilYawDegrees = 0.12f;

	/** Optional temporary diagnostic flash at the gun barrel tip. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Weapon|Debug")
	bool bDrawMuzzleFlash = true;

	/** Optional diagnostic mark at impact. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Weapon|Debug")
	bool bDrawImpactMarks = true;

	UFUNCTION(Server, Reliable)
	void ServerRequestFire();

	UFUNCTION(NetMulticast, Unreliable)
	/** All audible/visible feedback is cosmetic. Shot validation stays on server. */
	void MulticastShotFeedback(
		FVector_NetQuantize Start, FVector_NetQuantize End, bool bBlockingHit);

	/** Sent only to the owning client after the server accepts a shot. */
	UFUNCTION(Client, Unreliable)
	void ClientApplyRecoil();

private:
	float NextAllowedShotTime = 0.0f;

	void FireOnAuthority();
};

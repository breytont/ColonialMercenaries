// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/NetSerialization.h"
#include "CMPrototypeWeaponComponent.generated.h"

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

	UFUNCTION(Server, Reliable)
	void ServerRequestFire();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastShotTracer(FVector_NetQuantize Start, FVector_NetQuantize End);

private:
	float NextAllowedShotTime = 0.0f;

	void FireOnAuthority();
};

// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CMRifleAnimInstance.generated.h"

/**
 * Read-only bridge from the networked Colonial Mercenaries characters to
 * animation Blueprints. Weapon pose authoring stays in AnimGraph assets.
 *
 * No world-level scans, networking RPCs, or actor Tick. Unreal updates
 * animation instances as part of its standard skeletal animation pipeline.
 */
UCLASS(Blueprintable, BlueprintType)
class COLONIALMERCENARIES_API UCMRifleAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** Smooth 0..1 upper-body rifle-ready/aiming blend. An equipped player
	 * keeps the rifle raised even when RMB shoulder aim is released.
	 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="Colonial Mercenaries|Rifle Animation")
	float CMRifleAimAlpha = 0.0f;

	/** RMB shoulder-aim state; independent of rifle-ready pose visibility. */
	UPROPERTY(Transient, BlueprintReadOnly, Category="Colonial Mercenaries|Rifle Animation")
	bool CMIsAiming = false;

	/** Ground-plane speed, in Unreal units per second. */
	UPROPERTY(Transient, BlueprintReadOnly, Category="Colonial Mercenaries|Rifle Animation")
	float CMGroundSpeed = 0.0f;

	/** Horizontal movement relative to the character: forward 0, right 90. */
	UPROPERTY(Transient, BlueprintReadOnly, Category="Colonial Mercenaries|Rifle Animation")
	float CMMovementDirection = 0.0f;

	/** Movement component reports falling or jumping. */
	UPROPERTY(Transient, BlueprintReadOnly, Category="Colonial Mercenaries|Rifle Animation")
	bool CMIsInAir = false;

	/** Controller aim pitch, normalized to -90..90 degrees. */
	UPROPERTY(Transient, BlueprintReadOnly, Category="Colonial Mercenaries|Rifle Animation")
	float CMAimPitch = 0.0f;

	/** Horizontal delta between character facing and control aim direction. */
	UPROPERTY(Transient, BlueprintReadOnly, Category="Colonial Mercenaries|Rifle Animation")
	float CMAimYaw = 0.0f;

	/** Two Bone IK effector location for hand_l in hand_r bone space.
	 * Feed into an IK node with Effector Location Space = Bone Space and
	 * Effector Target = hand_r, after the rifle aiming Layered Blend.
	 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="Colonial Mercenaries|Rifle Animation")
	FVector CMLeftHandGripInRightHandSpace = FVector::ZeroVector;

	/** Higher rates transition into the equipped rifle pose more quickly. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Rifle Animation|Tuning",
		meta=(ClampMin="0.1", UIMin="0.1"))
	float AimPoseBlendSpeed = 10.0f;

private:
	/** Not persisted: refreshed if animation ownership changes. */
	// Compatible with both playable mercenaries and humanoid faction infantry.
	TWeakObjectPtr<class ACharacter> CachedCharacter;

	void RefreshCharacter();
};

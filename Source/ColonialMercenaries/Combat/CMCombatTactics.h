// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CMCombatTactics.generated.h"

/**
 * Replaceable decision strategy for an infantry role.
 * AI navigation, squad orders, health and weapon authority remain on the soldier.
 * Soldier Blueprints can choose a different derived class without copying the NPC.
 */
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class COLONIALMERCENARIES_API UCMCombatTactics : public UObject
{
	GENERATED_BODY()

public:
	/** True when an engaged soldier should leave its current firing position. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Colonial Mercenaries|AI|Tactics")
	bool WantsReposition(bool bCanShoot, float TimeSinceLastMove, float DistanceToThreat,
		float WeaponRange) const;
	virtual bool WantsReposition_Implementation(bool bCanShoot, float TimeSinceLastMove,
		float DistanceToThreat, float WeaponRange) const;

	/** World-space navigation suggestion, clamped to the current squad's duty area. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Colonial Mercenaries|AI|Tactics")
	FVector ChoosePosition(FVector SelfPosition, FVector ThreatPosition,
		FVector DutyCenter, float LeashRadius, float WeaponRange, int32 SideSeed,
		bool bCanShoot) const;
	virtual FVector ChoosePosition_Implementation(FVector SelfPosition, FVector ThreatPosition,
		FVector DutyCenter, float LeashRadius, float WeaponRange, int32 SideSeed,
		bool bCanShoot) const;

	/** How quickly a blocked/out-of-range soldier may try a new firing lane.
	 * This is separate from the slower cosmetic between-burst movement.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Colonial Mercenaries|AI|Tactics")
	float GetNoShotRepositionCooldown() const;
	virtual float GetNoShotRepositionCooldown_Implementation() const;

	/** Shot cone half-angle in degrees; each shot is resolved by the server.
	 * Distance and movement change dispersion without arbitrary hit rolls.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Colonial Mercenaries|AI|Accuracy")
	float GetShotSpreadDegrees(float DistanceToTarget, float WeaponRange,
		bool bMoving) const;
	virtual float GetShotSpreadDegrees_Implementation(float DistanceToTarget,
		float WeaponRange, bool bMoving) const;

	/** Soft cooldown between deliberate tactical moves. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Colonial Mercenaries|AI|Tactics")
	float GetRepositionCooldown() const;
	virtual float GetRepositionCooldown_Implementation() const;
};

/** Default balanced rifleman; future Assault / Marksman / Support profiles
 * can derive from UCMCombatTactics in C++ or Blueprint and replace these rules.
 */
UCLASS(Blueprintable, BlueprintType)
class COLONIALMERCENARIES_API UCMRiflemanCombatTactics : public UCMCombatTactics
{
	GENERATED_BODY()

public:
	virtual bool WantsReposition_Implementation(bool bCanShoot, float TimeSinceLastMove,
		float DistanceToThreat, float WeaponRange) const override;
	virtual FVector ChoosePosition_Implementation(FVector SelfPosition, FVector ThreatPosition,
		FVector DutyCenter, float LeashRadius, float WeaponRange, int32 SideSeed,
		bool bCanShoot) const override;
	virtual float GetRepositionCooldown_Implementation() const override;
	virtual float GetNoShotRepositionCooldown_Implementation() const override;
	virtual float GetShotSpreadDegrees_Implementation(float DistanceToTarget,
		float WeaponRange, bool bMoving) const override;

protected:
	/** Riflemen can move laterally between bursts, rather than standing still. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|AI|Rifleman",
		meta=(ClampMin="0.0"))
	float LateralMoveDistance = 260.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|AI|Rifleman",
		meta=(ClampMin="0.5"))
	float RepositionCooldown = 4.5f;

	/** Faster search for a firing lane when the soldier cannot shoot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|AI|Rifleman",
		meta=(ClampMin="0.3"))
	float NoShotRepositionCooldown = 1.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|AI|Rifleman",
		meta=(ClampMin="0.2", ClampMax="0.95"))
	float PreferredRangeRatio = 0.72f;

	/** Rifleman spread at point-blank range (degrees). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|AI|Accuracy",
		meta=(ClampMin="0.0", ClampMax="30.0"))
	float CloseRangeSpreadDegrees = 1.0f;

	/** Spread at maximum effective rifle range. Lower means more accurate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|AI|Accuracy",
		meta=(ClampMin="0.0", ClampMax="30.0"))
	float LongRangeSpreadDegrees = 5.0f;

	/** Additional spread when the muzzle is moving. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|AI|Accuracy",
		meta=(ClampMin="0.0", ClampMax="30.0"))
	float MovingSpreadPenaltyDegrees = 2.0f;
};

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

protected:
	/** Riflemen can move laterally between bursts, rather than standing still. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|AI|Rifleman",
		meta=(ClampMin="0.0"))
	float LateralMoveDistance = 260.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|AI|Rifleman",
		meta=(ClampMin="0.5"))
	float RepositionCooldown = 4.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|AI|Rifleman",
		meta=(ClampMin="0.2", ClampMax="0.95"))
	float PreferredRangeRatio = 0.72f;
};

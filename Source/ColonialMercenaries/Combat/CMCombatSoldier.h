// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CMCombatSoldier.generated.h"

class ACMCombatSoldier;
class UCMFactionComponent;
class UCMHealthComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Temporary prototype soldier for independent faction combat.
 *
 * The authoritative server scans for hostile soldiers on a timer, requests
 * AI navigation, and applies ordinary Unreal damage to raycast hits.
 * This is intentionally not the final modular weapon or AI architecture.
 */
UCLASS(Blueprintable)
class COLONIALMERCENARIES_API ACMCombatSoldier : public ACharacter
{
	GENERATED_BODY()

public:
	ACMCombatSoldier();

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Combat")
	UCMFactionComponent* GetFactionComponent() const { return FactionComponent; }

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Combat")
	UCMHealthComponent* GetHealthComponent() const { return HealthComponent; }

protected:
	virtual void BeginPlay() override;

	/** Server-driven tactical decisions (no actor Tick). */
	void UpdateCombat();

	/** Find the nearest living enemy within detection range. */
	ACMCombatSoldier* FindClosestHostile() const;

	/** True when a Visibility trace hits the intended target first. */
	bool HasLineOfFire(const ACMCombatSoldier* Target) const;

	/** Fire a prototype hitscan shot; damage is always decided by the server. */
	void FireAt(ACMCombatSoldier* Target);

	/** Cosmetic tracer, displayed by server and connected clients. */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastDrawShot(FVector Start, FVector End);

	UFUNCTION()
	void HandleDeath();

	/** Log successful damage on the authority for test verification. */
	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float NewMaxHealth);

	/** A simple, replaceable visual so the prototype works without new assets. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Visual")
	TObjectPtr<UStaticMeshComponent> BodyVisual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Visual")
	TObjectPtr<UTextRenderComponent> FactionLabel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Combat")
	TObjectPtr<UCMFactionComponent> FactionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Combat")
	TObjectPtr<UCMHealthComponent> HealthComponent;

	/** How frequently this NPC reevaluates nearby combatants. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Colonial Mercenaries|Combat|Tuning", meta=(ClampMin="0.1", UIMin="0.1"))
	float DecisionInterval = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Colonial Mercenaries|Combat|Tuning", meta=(ClampMin="100.0", UIMin="100.0"))
	float DetectionRange = 6500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Colonial Mercenaries|Combat|Tuning", meta=(ClampMin="100.0", UIMin="100.0"))
	float WeaponRange = 950.0f;

	/** How close we navigate toward an enemy when terrain or another actor blocks fire. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Colonial Mercenaries|Combat|Tuning",
		meta=(ClampMin="50.0", UIMin="50.0"))
	float BlockedApproachDistance = 175.0f;

	/** Delay between failed/completed path requests; prevents repeated MoveTo requests. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Colonial Mercenaries|Combat|Tuning",
		meta=(ClampMin="0.1", UIMin="0.1"))
	float PathRetryInterval = 1.5f;

	/** Optional red/green visibility-ray visualization for local prototype testing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Colonial Mercenaries|Combat|Debug")
	bool bDrawLineOfSight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Colonial Mercenaries|Combat|Tuning", meta=(ClampMin="0.0", UIMin="0.0"))
	float ShotDamage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Colonial Mercenaries|Combat|Tuning", meta=(ClampMin="0.1", UIMin="0.1"))
	float ShotsPerSecond = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Colonial Mercenaries|Combat|Tuning", meta=(ClampMin="1.0", UIMin="1.0"))
	float WalkSpeed = 320.0f;

	/** Point on the character that fires / aims, relative to the capsule origin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Colonial Mercenaries|Combat|Tuning")
	float AimHeight = 40.0f;

	/** Time until a dead actor is removed; future versions will leave salvage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Colonial Mercenaries|Combat|Tuning", meta=(ClampMin="0.1"))
	float CorpseLifetime = 7.0f;

private:
	FTimerHandle DecisionTimerHandle;
	TWeakObjectPtr<ACMCombatSoldier> CurrentTarget;
	float NextShotTime = 0.0f;
	float NextMoveRequestTime = 0.0f;
	bool bLastMoveWasBlockedApproach = false;
	bool bReportedMissingController = false;
	bool bReportedMoveFailure = false;
};

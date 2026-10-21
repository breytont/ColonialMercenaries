// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CMCombatSoldier.generated.h"

class ACMCombatSoldier;
class UCMFactionComponent;
class UCMHealthComponent;
class UStaticMeshComponent;
class USceneComponent;
class UAnimMontage;
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

	/** Animation bridge: grip marker in right-hand bone space. */
	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Combat|Animation")
	FVector GetPrototypeRifleSupportGripInRightHandSpace() const;

	/** Server-driven firing orientation, replicated to animate client NPCs. */
	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Combat|Animation")
	FRotator GetCombatAimRotation() const { return CombatAimRotation; }

protected:
	virtual void BeginPlay() override;

	/** Server-driven tactical decisions (no actor Tick). */
	void UpdateCombat();

	/** Find nearest hostile soldier or active player within detection range. */
	ACharacter* FindClosestHostile() const;

	/** True when a Visibility trace hits the intended target first. */
	bool HasLineOfFire(const ACharacter* Target) const;

	/** Fire a prototype hitscan shot; damage is always decided by the server. */
	void FireAt(ACharacter* Target);

	/** Cosmetic tracer, displayed by server and connected clients. */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastDrawShot(FVector Start, FVector End);

	UFUNCTION()
	void HandleDeath();

	/** Log successful damage on the authority for test verification. */
	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float NewMaxHealth);

	/** Retained as a fallback if the mannequin asset is unavailable. Hidden when
	 * the skeletal mannequin is configured. Existing level actor identities,
	 * collision profiles, and squad spawners are unchanged.
	 */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Visual")
	TObjectPtr<UStaticMeshComponent> BodyVisual;

	/** Prototype gun follows the mannequin's right hand. All weapon shapes
	 * are cosmetic and non-colliding, separate from server damage tracing.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Visual|Rifle")
	TObjectPtr<USceneComponent> PrototypeRifleRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Visual|Rifle")
	TObjectPtr<USceneComponent> PrototypeRifleSupportGrip;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Visual|Rifle")
	TObjectPtr<USceneComponent> PrototypeRifleMuzzle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Visual|Rifle")
	TObjectPtr<UStaticMeshComponent> RifleReceiver;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Visual|Rifle")
	TObjectPtr<UStaticMeshComponent> RifleBarrel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Visual|Rifle")
	TObjectPtr<UStaticMeshComponent> RifleStock;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Visual|Rifle")
	TObjectPtr<UStaticMeshComponent> RifleGrip;

	/** Uses the existing third-person rifle montage; can be overridden in
	 * the faction infantry Blueprint for later weapon types.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Visual|Animation")
	TObjectPtr<UAnimMontage> FireMontage;

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

	/** Presentation only: server updates when a visible enemy is engaged.
	 * Does not determine damage, navigation, or the actual trace.
	 */
	UPROPERTY(Replicated, Transient)
	FRotator CombatAimRotation = FRotator::ZeroRotator;

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	FTimerHandle DecisionTimerHandle;
	TWeakObjectPtr<ACharacter> CurrentTarget;
	float NextShotTime = 0.0f;
	float NextMoveRequestTime = 0.0f;
	bool bLastMoveWasBlockedApproach = false;
	bool bReportedMissingController = false;
	bool bReportedMoveFailure = false;
};

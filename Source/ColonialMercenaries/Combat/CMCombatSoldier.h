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
class USoundBase;
class USoundAttenuation;
class FLifetimeProperty;
class UTextRenderComponent;
class AAIController;
class AController;
class UDamageType;
class UCMCombatTactics;

/** High-level movement/combat intent assigned by an authoritative squad leader. */
UENUM(BlueprintType)
enum class ECMSquadOrder : uint8
{
	Independent UMETA(DisplayName="Independent"),
	Rally UMETA(DisplayName="Assemble"),
	Advance UMETA(DisplayName="Advance"),
	Defend UMETA(DisplayName="Defend"),
	Disengage UMETA(DisplayName="Withdraw")
};

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

	/** Outpost/squad leader command; ignored on clients. Independent preserves original AI. */
	UFUNCTION(BlueprintCallable, Category="Colonial Mercenaries|Combat|Squad")
	void SetSquadOrder(ECMSquadOrder NewOrder, FVector AssignedPosition);

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Combat|Squad")
	ECMSquadOrder GetSquadOrder() const { return SquadOrder; }

	/** Current visible opponent being engaged, for the owning outpost's
	 * short-range squad contact reports. May be null or stale; validate it.
	 */
	ACharacter* GetEngagedTarget() const { return CurrentTarget.Get(); }

	/** Server-only squad contact. The owning outpost verifies faction and
	 * proximity. Allows a neutral shooter only after confirmed incoming damage.
	 */
	void SetSharedSquadContact(ACharacter* Threat, bool bConfirmedAttacker,
		FVector DutyCenter, float AssistRadius);

	/** Report a personally engaged enemy or a recent damage instigator.
	 * The outpost uses this for BOTH factions and ALL non-retreat orders.
	 */
	ACharacter* GetReportedThreat(bool& bOutConfirmedAttacker) const;

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
	ACharacter* FindClosestHostile(float SearchRadius = 0.0f) const;

	/** Squad orders take priority over independent pursuit (server only). */
	bool UpdateOrderedCombat(AAIController* AI);

	/** One role-independent firing path and pluggable position selection. */
	void AimAndFireAt(ACharacter* Target, AAIController* AI, float Now);
	bool TryTacticalMove(ACharacter* Target, bool bHasShot, AAIController* AI,
		FVector DutyCenter, float LeashRadius, float Now);
	bool IsThreatValid(ACharacter* Candidate, bool bConfirmedAttacker) const;

	UFUNCTION()
	void HandleIncomingDamage(AActor* DamagedActor, float Damage,
		const UDamageType* DamageType, AController* InstigatedBy,
		AActor* DamageCauser);

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

	/** Short, client-only debug glow at the attached rifle muzzle.
	 * The current NPC ballistics trace still originates at torso height.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category="Colonial Mercenaries|Visual|Effects")
	bool bDrawMuzzleFlash = true;

	/** Uses checked-in temporary SoundWave; can be replaced per soldier BP. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category="Colonial Mercenaries|Audio")
	TObjectPtr<USoundBase> GunshotSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category="Colonial Mercenaries|Audio")
	TObjectPtr<USoundAttenuation> GunshotAttenuation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Visual")
	TObjectPtr<UTextRenderComponent> FactionLabel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Combat")
	TObjectPtr<UCMFactionComponent> FactionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Combat")
	TObjectPtr<UCMHealthComponent> HealthComponent;

	/** Combat role logic is a replaceable Blueprint/C++ UObject class.
	 * The default rifleman tactics are intentionally small and data-driven.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|AI|Role")
	TSubclassOf<UCMCombatTactics> CombatTacticsClass;

	UPROPERTY(Transient)
	TObjectPtr<UCMCombatTactics> CombatTactics;

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

	UPROPERTY(Replicated, Transient, BlueprintReadOnly,
		Category="Colonial Mercenaries|Combat|Squad")
	ECMSquadOrder SquadOrder = ECMSquadOrder::Independent;

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	FTimerHandle DecisionTimerHandle;
	TWeakObjectPtr<ACharacter> CurrentTarget;
	TWeakObjectPtr<ACharacter> SharedSquadTarget;
	TWeakObjectPtr<ACharacter> RecentAttacker;
	float AttackerMemoryExpiresAt = 0.0f;
	bool bAttackerWasUnaligned = false;
	bool bLoggedFailedDamageAttribution = false;
	bool bSharedConfirmedAttacker = false;
	FVector DutyCenter = FVector::ZeroVector;
	float SquadAssistRadius = 1900.0f;
	bool bWasAssisting = false;

	FVector TacticalGoal = FVector::ZeroVector;
	TWeakObjectPtr<ACharacter> TacticalMoveTarget;
	bool bTacticalMoveActive = false;
	float TacticalMoveExpiresAt = 0.0f;
	float LastTacticalMoveAt = 0.0f;
	float NextTacticalMoveAt = 0.0f;
	int32 TacticalSideSeed = 0;
	FVector SquadPosition = FVector::ZeroVector;
	bool bSquadMoveDirty = false;
	float NextShotTime = 0.0f;
	float NextMoveRequestTime = 0.0f;
	bool bLastMoveWasBlockedApproach = false;
	bool bReportedMissingController = false;
	bool bReportedMoveFailure = false;
};

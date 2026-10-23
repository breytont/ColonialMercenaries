// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "CMFactionOutpost.generated.h"

class ACMCombatSoldier;
class ACharacter;
class FLifetimeProperty;
class UArrowComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Strategic stance selected for the outpost before or during PIE. */
UENUM(BlueprintType)
enum class ECMOutpostPosture : uint8
{
	Assault UMETA(DisplayName="Assault Objective"),
	Defend UMETA(DisplayName="Defend Outpost")
};

/** Current command phase of this outpost's infantry detachment. */
UENUM(BlueprintType)
enum class ECMOutpostSquadPhase : uint8
{
	Assembly UMETA(DisplayName="Assembling"),
	Advance UMETA(DisplayName="Advancing"),
	Retreat UMETA(DisplayName="Withdrawing"),
	Defend UMETA(DisplayName="Defending")
};

/**
 * Placeable faction installation with a finite infantry reserve.
 *
 * The server deploys an initial garrison and replaces casualties in small
 * batches until its reserve is exhausted. The AI soldiers remain independent:
 * squad coordination, objective assignments, and supply convoys are separate
 * milestones. Clients only receive the replicated status and soldier actors.
 */
UCLASS(Blueprintable)
class COLONIALMERCENARIES_API ACMFactionOutpost : public AActor
{
	GENERATED_BODY()

public:
	ACMFactionOutpost();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Faction Outpost")
	int32 GetActiveGarrison() const { return ActiveGarrison; }

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Faction Outpost")
	int32 GetReserveRemaining() const { return ReserveRemaining; }

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Faction Outpost")
	int32 GetTotalSoldiersDeployed() const { return TotalSoldiersDeployed; }

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Faction Outpost")
	FGameplayTag GetFactionId() const { return FactionId; }

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Faction Outpost|Squad")
	ECMOutpostSquadPhase GetSquadPhase() const { return SquadPhase; }

	/** Server-side command hook for future contracts and strategic AI. */
	UFUNCTION(BlueprintCallable, Category="Colonial Mercenaries|Faction Outpost|Squad")
	void SetOutpostPosture(ECMOutpostPosture NewPosture);

	/** Server-only entry point for future convoy/logistics replenishment.
	 * No automatic resupply takes place in this milestone.
	 */
	UFUNCTION(BlueprintCallable, Category="Colonial Mercenaries|Faction Outpost")
	bool AddReserves(int32 NumberOfSoldiers);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Components")
	TObjectPtr<UArrowComponent> DeploymentDirection;

	/** Simple no-collision installation placeholders, not final base art. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Components")
	TObjectPtr<UStaticMeshComponent> BasePlatform;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Components")
	TObjectPtr<UStaticMeshComponent> BaseStructure;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Components")
	TObjectPtr<UTextRenderComponent> StatusLabel;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Identity")
	FString OutpostName = TEXT("Forward Outpost");

	/** Choose Faction.Prototype.Red or Faction.Prototype.Blue in the level. */
	UPROPERTY(EditInstanceOnly, ReplicatedUsing=OnRep_OutpostStatus,
		BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Identity")
	FGameplayTag FactionId;

	/** Defaults to the existing animated humanoid CMCombatSoldier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Garrison")
	TSubclassOf<ACMCombatSoldier> SoldierClass;

	/** Free starting soldiers; they do not consume the reserve. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Garrison",
		meta=(ClampMin="0", ClampMax="12", UIMin="0", UIMax="12"))
	int32 InitialGarrisonSize = 3;

	/** Maximum number of living soldiers controlled by this outpost. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Garrison",
		meta=(ClampMin="1", ClampMax="12", UIMin="1", UIMax="12"))
	int32 TargetGarrisonSize = 3;

	/** Reserve pool; only reinforcement soldiers consume these resources. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Garrison",
		meta=(ClampMin="0", ClampMax="100", UIMin="0", UIMax="30"))
	int32 InitialReserves = 9;

	/** Replacement soldiers deployed per wave, up to missing garrison slots. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Reinforcements",
		meta=(ClampMin="1", ClampMax="12", UIMin="1", UIMax="6"))
	int32 ReinforcementWaveSize = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Reinforcements",
		meta=(ClampMin="1.0", UIMin="5.0"))
	float ReinforcementCooldownSeconds = 20.0f;

	/** Server checks a small list of this base's own soldiers, not all actors. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Reinforcements",
		meta=(ClampMin="0.2", UIMin="0.5"))
	float GarrisonCheckInterval = 1.0f;

	/** How far in front of the installation infantry deploys (centimeters). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Deployment",
		meta=(ClampMin="150.0", UIMin="200.0"))
	float DeploymentForwardOffset = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Deployment",
		meta=(ClampMin="1", ClampMax="6"))
	int32 SoldiersPerRow = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Deployment",
		meta=(ClampMin="100.0"))
	float LateralSpacing = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Deployment",
		meta=(ClampMin="100.0"))
	float RowSpacing = 180.0f;

	/** Character capsule origin is above the walking surface. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Deployment",
		meta=(ClampMin="0.0"))
	float SpawnHeight = 100.0f;

	/** Coordinates this outpost's infantry; does not affect old squad spawners. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Squad")
	ECMOutpostPosture StartingPosture = ECMOutpostPosture::Assault;

	/** Assign the central CMContestedPoint in the map; if unset, march forward. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Squad")
	TObjectPtr<AActor> AssaultObjective;

	/** Only used when no AssaultObjective has been assigned. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Squad",
		meta=(ClampMin="300.0"))
	float FallbackAdvanceDistance = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Squad",
		meta=(ClampMin="0.0"))
	float AssemblySeconds = 5.0f;

	/** Retreat if the number of surviving members reaches this threshold. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Squad",
		meta=(ClampMin="0", ClampMax="11"))
	int32 RetreatAtOrBelow = 1;

	/** Must regain target strength and wait this long before assembling again. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Squad",
		meta=(ClampMin="0.0"))
	float RegroupSeconds = 4.0f;

	/** Separation between ordered infantry near rally/target locations. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Squad",
		meta=(ClampMin="100.0"))
	float SquadSlotSpacing = 230.0f;

	/** Intended staging position ahead of outpost, before attack begins. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Squad",
		meta=(ClampMin="0.0"))
	float AssemblyForwardOffset = 600.0f;

	/** Center distance behind the outpost for withdrawal. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Squad",
		meta=(ClampMin="0.0"))
	float WithdrawalBackOffset = 350.0f;

	/** Response leash around the outpost for Defend, or the assigned
	 * formation slot for Advance/Rally. Works for either faction.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Squad|Defense",
		meta=(ClampMin="500.0", UIMin="1000.0"))
	float DefensiveAssistRadius = 1900.0f;

	/** Keep a verified contact briefly while riflemen reposition for LOS. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Faction Outpost|Squad|Defense",
		meta=(ClampMin="0.0", UIMin="2.0"))
	float DefensiveContactMemorySeconds = 4.0f;

	UPROPERTY(ReplicatedUsing=OnRep_OutpostStatus, BlueprintReadOnly,
		Category="Colonial Mercenaries|Faction Outpost|Squad|State")
	bool bSquadContactActive = false;

	UPROPERTY(ReplicatedUsing=OnRep_OutpostStatus, BlueprintReadOnly,
		Category="Colonial Mercenaries|Faction Outpost|Squad|State")
	ECMOutpostSquadPhase SquadPhase = ECMOutpostSquadPhase::Assembly;

	/** Replicated summary, safe to use in later contracts and faction UI. */
	UPROPERTY(ReplicatedUsing=OnRep_OutpostStatus, BlueprintReadOnly,
		Category="Colonial Mercenaries|Faction Outpost|State")
	int32 ActiveGarrison = 0;

	UPROPERTY(ReplicatedUsing=OnRep_OutpostStatus, BlueprintReadOnly,
		Category="Colonial Mercenaries|Faction Outpost|State")
	int32 ReserveRemaining = 0;

	UPROPERTY(ReplicatedUsing=OnRep_OutpostStatus, BlueprintReadOnly,
		Category="Colonial Mercenaries|Faction Outpost|State")
	int32 TotalSoldiersDeployed = 0;

private:
	FTimerHandle GarrisonCheckTimerHandle;
	float NextReinforcementTime = 0.0f;
	float PhaseStartedAt = 0.0f;
	ECMOutpostPosture CurrentPosture = ECMOutpostPosture::Assault;
	TWeakObjectPtr<ACharacter> SharedSquadContact;
	bool bSharedContactWasProvoked = false;
	float SquadContactExpiresAt = 0.0f;

	/** Authority only. Weak references do not prolong soldier lifetime. */
	TArray<TWeakObjectPtr<ACMCombatSoldier>> FieldedSoldiers;

	int32 SpawnInfantry(int32 Requested, bool bConsumeReserves);
	void CheckGarrison();
	void RefreshGarrisonState();
	void RefreshStatusLabel();
	void SetSquadPhase(ECMOutpostSquadPhase NewPhase);
	void UpdateSquadOrders();
	FVector GetSquadSlot(int32 Index, ECMOutpostSquadPhase Phase) const;

	UFUNCTION()
	void OnRep_OutpostStatus();
};

// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "CMFactionOutpost.generated.h"

class ACMCombatSoldier;
class FLifetimeProperty;
class UArrowComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

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

	/** Authority only. Weak references do not prolong soldier lifetime. */
	TArray<TWeakObjectPtr<ACMCombatSoldier>> FieldedSoldiers;

	int32 SpawnInfantry(int32 Requested, bool bConsumeReserves);
	void CheckGarrison();
	void RefreshGarrisonState();
	void RefreshStatusLabel();

	UFUNCTION()
	void OnRep_OutpostStatus();
};

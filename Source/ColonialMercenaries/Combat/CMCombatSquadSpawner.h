// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "CMCombatSquadSpawner.generated.h"

class ACMCombatSoldier;
class UArrowComponent;
class USceneComponent;

/**
 * Editor-placeable prototype squad deployment point.
 *
 * Spawns a small formation of existing combat soldiers on the authority when
 * the level starts. The soldiers own their own replicated combat state and
 * autonomous AI. This does not introduce squad orders or squad-level tactics.
 */
UCLASS(Blueprintable)
class COLONIALMERCENARIES_API ACMCombatSquadSpawner : public AActor
{
	GENERATED_BODY()

public:
	ACMCombatSquadSpawner();

protected:
	virtual void BeginPlay() override;

	/** Spawn initial squad members on the server only. */
	void SpawnSquad();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Squad")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Editor-only directional reference for the formation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Squad")
	TObjectPtr<UArrowComponent> FacingArrow;

	/** An unset faction disables spawning to prevent accidental neutral units. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Squad")
	FGameplayTag FactionId;

	/** Can be replaced by a derived soldier Blueprint without changing C++. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Squad")
	TSubclassOf<ACMCombatSoldier> SoldierClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Squad|Formation",
		meta=(ClampMin="1", ClampMax="12", UIMin="1", UIMax="12"))
	int32 SoldierCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Squad|Formation",
		meta=(ClampMin="1", ClampMax="6", UIMin="1", UIMax="6"))
	int32 SoldiersPerRow = 3;

	/** Distance between soldiers across the formation, in Unreal units. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Squad|Formation",
		meta=(ClampMin="100.0", UIMin="100.0"))
	float LateralSpacing = 220.0f;

	/** Distance between ranks, in Unreal units. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Squad|Formation",
		meta=(ClampMin="100.0", UIMin="100.0"))
	float RowSpacing = 220.0f;

	/** Character origins are near their capsule centers, not their feet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Squad|Formation",
		meta=(ClampMin="0.0", UIMin="0.0"))
	float SpawnHeight = 100.0f;
};

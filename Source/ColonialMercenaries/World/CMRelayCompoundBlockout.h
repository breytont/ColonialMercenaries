// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CMRelayCompoundBlockout.generated.h"

class USceneComponent;
class UInstancedStaticMeshComponent;

/**
 * Editor-placeable graybox of the Contested Frontier relay compound.
 *
 * One actor owns reusable, collision-enabled modular geometry. The geometry
 * is regenerated in OnConstruction when dimensions are adjusted in Details.
 * It is deliberately art-free, has no Tick, and changes no AI/gameplay code.
 *
 * Origin should be set at the central relay (world X/Y = 0). Geometry is
 * authored in centimeters, with height measured relative to the actor pivot.
 */
UCLASS(Blueprintable, meta=(DisplayName="CM Relay Compound Blockout"))
class COLONIALMERCENARIES_API ACMRelayCompoundBlockout : public AActor
{
	GENERATED_BODY()

public:
	ACMRelayCompoundBlockout();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Relay Blockout")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Relay Blockout")
	TObjectPtr<UInstancedStaticMeshComponent> PerimeterWalls;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Relay Blockout")
	TObjectPtr<UInstancedStaticMeshComponent> CoverPieces;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Relay Blockout")
	TObjectPtr<UInstancedStaticMeshComponent> Buildings;

	/** Compound half extents: default overall size 44 x 36 metres. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Relay Blockout|Layout",
		meta=(ClampMin="1600.0", UIMin="1600.0", UIMax="4000.0"))
	float HalfExtentX = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Relay Blockout|Layout",
		meta=(ClampMin="1400.0", UIMin="1400.0", UIMax="3500.0"))
	float HalfExtentY = 1800.0f;

	/** Four cardinal entrance gaps (1000 cm total by default). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Relay Blockout|Layout",
		meta=(ClampMin="300.0", UIMin="300.0", UIMax="900.0"))
	float EntranceHalfWidth = 500.0f;

	/** Height of outer cover walls, from local ground level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Relay Blockout|Layout",
		meta=(ClampMin="70.0", UIMin="70.0", UIMax="300.0"))
	float WallHeight = 160.0f;

	/** Height of internal hard cover; ~110 cm by default. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Relay Blockout|Layout",
		meta=(ClampMin="60.0", UIMin="60.0", UIMax="180.0"))
	float CoverHeight = 110.0f;

	/**
	 * Existing Relay_Foundation top sits 20cm above terrain.
	 * Inner cover and the utility hut are raised by this amount.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Relay Blockout|Layout",
		meta=(ClampMin="0.0", UIMin="0.0", UIMax="100.0"))
	float FoundationTopOffset = 20.0f;

	/** Optional hut on the north side; can be toggled off to improve sight lines. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Relay Blockout|Layout")
	bool bPlaceUtilityHut = true;
};

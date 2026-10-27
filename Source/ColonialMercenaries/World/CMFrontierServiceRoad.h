// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CMFrontierServiceRoad.generated.h"

class USceneComponent;
class UInstancedStaticMeshComponent;

/**
 * Nonblocking visual graybox path between installations. Segment elevations
 * follow walkable terrain via traces when the actor is constructed in-editor.
 * Geometry is for planning only: Landscape Splines and materials come later.
 *
 * Contested Frontier default: place the actor at (0,-8400,0), yaw 0.
 * The road runs 6600cm north (+Y), from HQ's north gate to relay's
 * south entrance. No player, AI, contract, or navmesh behavior is changed.
 */
UCLASS(Blueprintable, meta=(DisplayName="CM Frontier Service Road"))
class COLONIALMERCENARIES_API ACMFrontierServiceRoad : public AActor
{
    GENERATED_BODY()

public:
    ACMFrontierServiceRoad();
    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Service Road")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Service Road")
    TObjectPtr<UInstancedStaticMeshComponent> RoadSurface;

    /** Road length along actor +Y, centimetres. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Service Road|Layout",
        meta=(ClampMin="500.0", UIMin="500.0", UIMax="15000.0"))
    float LengthCm = 6600.0f;

    /** Exterior-to-exterior width. A future vehicle can use this corridor. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Service Road|Layout",
        meta=(ClampMin="300.0", UIMin="300.0", UIMax="1000.0"))
    float WidthCm = 600.0f;

    /** Gentle optional bend in local X at midpoint, zero at both gateways. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Service Road|Layout",
        meta=(ClampMin="-800.0", UIMin="-400.0", UIMax="400.0", ClampMax="800.0"))
    float MidpointLateralOffsetCm = 180.0f;

    /** Number of short instances used to conform to slopes. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Service Road|Layout",
        meta=(ClampMin="8", ClampMax="80", UIMin="16", UIMax="48"))
    int32 SegmentCount = 33;

    /** Gap from the actual landscape to the visual road's bottom face. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Service Road|Layout",
        meta=(ClampMin="0.0", UIMin="0.0", UIMax="25.0"))
    float RoadClearanceCm = 3.0f;

private:
    FVector TerrainAdjustedWorldPoint(float T) const;
};

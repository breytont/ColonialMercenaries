// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CMMercenaryHeadquartersBlockout.generated.h"

class USceneComponent;
class UArrowComponent;
class UInstancedStaticMeshComponent;
class UTextRenderComponent;

/**
 * Editor-placeable, adjustable headquarters graybox for Contested Frontier.
 * A walkable open yard, two gated approaches, an operations building, a
 * sheltered vehicle bay and storage placeholders. No contracts or economy
 * are implemented here; those will be separate gameplay actors.
 *
 * Uses built-in Engine cube instancing only, with no Tick or replication.
 */
UCLASS(Blueprintable, meta=(DisplayName="CM Mercenary Headquarters Blockout"))
class COLONIALMERCENARIES_API ACMMercenaryHeadquartersBlockout : public AActor
{
    GENERATED_BODY()

public:
    ACMMercenaryHeadquartersBlockout();
    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Headquarters|Components")
    TObjectPtr<USceneComponent> SceneRoot;

    /** Points north toward the relay compound at this level's default orientation. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Headquarters|Components")
    TObjectPtr<UArrowComponent> PrimaryGateDirection;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Headquarters|Components")
    TObjectPtr<UInstancedStaticMeshComponent> GroundMarkers;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Headquarters|Components")
    TObjectPtr<UInstancedStaticMeshComponent> PerimeterBarriers;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Headquarters|Components")
    TObjectPtr<UInstancedStaticMeshComponent> Facilities;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Headquarters|Components")
    TObjectPtr<UInstancedStaticMeshComponent> Storage;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Headquarters|Components")
    TObjectPtr<UTextRenderComponent> HeadquartersLabel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Headquarters|Components")
    TObjectPtr<UTextRenderComponent> OperationsLabel;

    /** Compound footprint (default 36m wide by 32m deep). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Headquarters|Layout",
        meta=(ClampMin="1300.0", UIMin="1300.0", UIMax="2500.0"))
    float HalfWidthCm = 1800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Headquarters|Layout",
        meta=(ClampMin="1300.0", UIMin="1300.0", UIMax="2500.0"))
    float HalfDepthCm = 1600.0f;

    /** North entrance facing the relay; total default width 10 metres. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Headquarters|Layout",
        meta=(ClampMin="350.0", UIMin="350.0", UIMax="800.0"))
    float NorthGateHalfWidthCm = 500.0f;

    /** Eastern service entrance; total default width 8 metres. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Headquarters|Layout",
        meta=(ClampMin="300.0", UIMin="300.0", UIMax="650.0"))
    float EastGateHalfWidthCm = 400.0f;

    /** Short physical barriers define the HQ without enclosing the player. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Headquarters|Layout",
        meta=(ClampMin="70.0", UIMin="70.0", UIMax="240.0"))
    float BarrierHeightCm = 130.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Headquarters|Facilities")
    bool bShowOperationsModule = true;

    /** Covered vehicle bay with an open side, not a closed vehicle garage. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Headquarters|Facilities")
    bool bShowVehicleCanopy = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Headquarters|Facilities")
    bool bShowStorage = true;
};

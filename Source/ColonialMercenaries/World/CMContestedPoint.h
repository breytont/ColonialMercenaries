// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "CMContestedPoint.generated.h"

class USceneComponent;
class USphereComponent;
class UTextRenderComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCMPointOwnerChanged, FGameplayTag, NewOwner);

/**
 * Prototype local objective: nearby, living infantry contest its ownership.
 *
 * The server evaluates an area on a timer; no expensive per-frame Tick.
 * Only the resulting state replicates to clients. Larger strategic systems
 * can later consume OnOwnerChanged and persist GetOwnerFaction().
 */
UCLASS(Blueprintable)
class COLONIALMERCENARIES_API ACMContestedPoint : public AActor
{
	GENERATED_BODY()

public:
	ACMContestedPoint();

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Territory")
	FGameplayTag GetOwnerFaction() const { return OwnerFaction; }

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Territory")
	FGameplayTag GetCapturingFaction() const { return CapturingFaction; }

	/** Progress toward capturing (0 to 1). Pauses while factions contest the point. */
	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Territory")
	float GetCaptureProgress() const { return CaptureProgress; }

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Territory")
	bool IsContested() const { return bContested; }

	/** Fired on the authoritative server and on clients after owner replication. */
	UPROPERTY(BlueprintAssignable, Category="Colonial Mercenaries|Territory")
	FCMPointOwnerChanged OnOwnerChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	/** Identifies the location in the prototype's floating status display. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Territory")
	FString PointName = TEXT("Contested Point");

	/** Optional starting owner. Leave unset for a neutral objective. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Territory")
	FGameplayTag InitialOwnerFaction;

	/** Adjust Sphere Radius on this component to set the capture-zone size. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Territory")
	TObjectPtr<USphereComponent> CaptureArea;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Territory")
	TObjectPtr<UTextRenderComponent> StatusLabel;

	/** Seconds required for one uncontested faction to capture the objective. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Territory|Tuning",
		meta=(ClampMin="0.1", UIMin="0.1"))
	float CaptureTimeSeconds = 8.0f;

	/** Frequency of server capture calculations. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Territory|Tuning",
		meta=(ClampMin="0.1", UIMin="0.1"))
	float CaptureUpdateInterval = 0.5f;

	UPROPERTY(ReplicatedUsing=OnRep_OwnerFaction, BlueprintReadOnly, Category="Colonial Mercenaries|Territory|State")
	FGameplayTag OwnerFaction;

	UPROPERTY(ReplicatedUsing=OnRep_CaptureState, BlueprintReadOnly, Category="Colonial Mercenaries|Territory|State")
	FGameplayTag CapturingFaction;

	UPROPERTY(ReplicatedUsing=OnRep_CaptureState, BlueprintReadOnly, Category="Colonial Mercenaries|Territory|State")
	float CaptureProgress = 0.0f;

	UPROPERTY(ReplicatedUsing=OnRep_CaptureState, BlueprintReadOnly, Category="Colonial Mercenaries|Territory|State")
	bool bContested = false;

private:
	UPROPERTY(VisibleAnywhere, Category="Colonial Mercenaries|Territory")
	TObjectPtr<USceneComponent> SceneRoot;

	FTimerHandle CaptureTimerHandle;

	void UpdateCapture();
	void RefreshStatusLabel();

	UFUNCTION()
	void OnRep_OwnerFaction();

	UFUNCTION()
	void OnRep_CaptureState();
};

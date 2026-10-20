// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "CMPrototypeVitalsWidget.generated.h"

class UCMHealthComponent;
class UProgressBar;
class UTextBlock;
class UImage;

/**
 * Local-only combat status HUD. Health comes from the replicated health
 * component; the proximity prompt and short hit flash are cosmetic.
 * A light timer refreshes temporary UI, not an Actor Tick.
 */
UCLASS(Blueprintable)
class COLONIALMERCENARIES_API UCMPrototypeVitalsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetTrackedHealth(UCMHealthComponent* NewHealth);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float NewMaxHealth);

	UFUNCTION()
	void HandleDowned();

	UFUNCTION()
	void HandleRevived();

	void RefreshDisplay();
	void RefreshInteractionAndHitFlash();

	UPROPERTY(Transient)
	TObjectPtr<UCMHealthComponent> TrackedHealth;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> VitalsLabel;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InteractionLabel;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> ReviveProgressBar;

	UPROPERTY(Transient)
	TObjectPtr<UImage> DamageOverlay;

	FTimerHandle HUDRefreshTimerHandle;
	float LastDisplayedHealth = -1.0f;
	float DamageFlashEndTime = 0.0f;
	float DamageFlashSeconds = 0.45f;
};

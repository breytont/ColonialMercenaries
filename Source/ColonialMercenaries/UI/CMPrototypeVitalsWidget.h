// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CMPrototypeVitalsWidget.generated.h"

class UCMHealthComponent;
class UTextBlock;

/** Read-only per-player prototype HUD for replicated health and incapacitation. */
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

	UPROPERTY(Transient)
	TObjectPtr<UCMHealthComponent> TrackedHealth;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> VitalsLabel;
};

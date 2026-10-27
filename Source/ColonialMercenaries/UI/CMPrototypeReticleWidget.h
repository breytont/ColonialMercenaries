// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CMPrototypeReticleWidget.generated.h"

class UCanvasPanel;
class UCanvasPanelSlot;
class UTextBlock;
class UCMPrototypeWeaponComponent;

/**
 * Local-only UI: projects the actual physical muzzle's potential hit point.
 * Never sends input, claims hits, or decides damage.
 *
 * NativeTick is justified for a visible reticle that must respond smoothly
 * to camera/weapon animation. No actor Tick or global actor scans are used.
 */
UCLASS(Blueprintable)
class COLONIALMERCENARIES_API UCMPrototypeReticleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Assign the local player's weapon. No ownership/replication required. */
	void SetWeaponComponent(UCMPrototypeWeaponComponent* InWeapon);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Optional override to hide the marker when the rifle is lowered.
	 * False by default: hip fire now has its own circular aiming indicator.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Reticle")
	bool bAimOnly = false;

	/** Reticle font size in Slate points. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Reticle",
		meta=(ClampMin="8.0", UIMin="8.0"))
	int32 ReticleFontSize = 26;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCMPrototypeWeaponComponent> WeaponComponent;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanelSlot> ReticleSlot;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ReticleText;

	/** Avoid rebuilding Slate text every frame while aim state is stable. */
	bool bLastRenderedAimState = false;
	bool bHasRenderedAimState = false;
};

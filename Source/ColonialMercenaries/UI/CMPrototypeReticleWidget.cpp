// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "UI/CMPrototypeReticleWidget.h"

#include "ColonialMercenariesCharacter.h"
#include "Combat/CMPrototypeWeaponComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"

void UCMPrototypeReticleWidget::SetWeaponComponent(
	UCMPrototypeWeaponComponent* InWeapon)
{
	WeaponComponent = InWeapon;
}

TSharedRef<SWidget> UCMPrototypeReticleWidget::RebuildWidget()
{
	// Build UMG children BEFORE Super::RebuildWidget assembles the Slate tree.
	// Creating the root during NativeConstruct can leave a blank SNullWidget.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("WeaponReticleCanvas"));
		WidgetTree->RootWidget = Canvas;
	}

	if (WidgetTree && !IsValid(ReticleText))
	{
		if (UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget))
		{
			ReticleText = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(), TEXT("PhysicalMuzzleCrosshair"));
			ReticleText->SetText(FText::FromString(TEXT("+")));
			ReticleSlot = Canvas->AddChildToCanvas(ReticleText);
		}
	}

	return Super::RebuildWidget();
}

void UCMPrototypeReticleWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(ReticleText))
	{
		ReticleText->SetJustification(ETextJustify::Center);
		const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", ReticleFontSize);
		ReticleText->SetFont(Font);
		ReticleText->SetShadowOffset(FVector2D(1.0f, 1.0f));
		ReticleText->SetShadowColorAndOpacity(
			FLinearColor(0.0f, 0.0f, 0.0f, 0.95f));
		ReticleText->SetColorAndOpacity(
			FSlateColor(FLinearColor(0.3f, 1.0f, 0.95f, 1.0f)));
	}

	if (IsValid(ReticleSlot))
	{
		ReticleSlot->SetAutoSize(true);
		ReticleSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		ReticleSlot->SetZOrder(10);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UCMPrototypeReticleWidget::NativeTick(
	const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!IsValid(ReticleText) || !IsValid(ReticleSlot))
	{
		return;
	}

	APlayerController* Player = GetOwningPlayer();
	const AColonialMercenariesCharacter* Character =
		IsValid(Player) ? Cast<AColonialMercenariesCharacter>(Player->GetPawn()) : nullptr;

	if (!IsValid(Character) || (bAimOnly && !Character->IsAiming())
		|| !IsValid(WeaponComponent))
	{
		ReticleText->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	FVector ShotStart;
	FVector ShotEnd;
	bool bBlockingHit = false;
	if (!WeaponComponent->GetPredictedShot(ShotStart, ShotEnd, bBlockingHit))
	{
		ReticleText->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	FVector2D WidgetPosition;
	if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
		Player, ShotEnd, WidgetPosition, true))
	{
		ReticleText->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	// The indicator follows projected muzzle collision, not screen center.
	// It may move away from the camera reticle or offscreen when a barrel
	// points into cover, the floor, or away from the camera.
	ReticleSlot->SetPosition(WidgetPosition);
	ReticleText->SetColorAndOpacity(FSlateColor(bBlockingHit
		? FLinearColor(1.0f, 0.83f, 0.38f, 1.0f)
		: FLinearColor(0.3f, 1.0f, 0.95f, 1.0f)));
	ReticleText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

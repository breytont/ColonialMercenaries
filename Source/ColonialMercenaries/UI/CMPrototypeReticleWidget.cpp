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
			ReticleText->SetText(FText::FromString(TEXT("○")));
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
		ReticleText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
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

	// Both modes use a circular marker projected from the TRUE muzzle
	// trace. This ring is a visual stability cue; it never steers a shot.
	// Aimed walking is slower and displays a tighter circle. Every
	// server-confirmed shot widens it temporarily before it recovers.
	const bool bAiming = Character->IsAiming();
	const float BaseCircleScale = bAiming ? AimedCircleScale : HipCircleScale;
	const float Bloom = WeaponComponent->GetLocalReticleBloom();
	const float Speed = Character->GetVelocity().Size2D();
	const float MovementExpansion = FMath::Clamp(
		Speed / 500.0f, 0.0f, 1.0f) * (bAiming ? 0.10f : 0.25f);
	const float CircleScale = FMath::Clamp(
		BaseCircleScale + Bloom * MaxAdditionalBloomScale
		+ MovementExpansion, 0.3f, 3.0f);
	ReticleText->SetRenderScale(FVector2D(CircleScale, CircleScale));

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

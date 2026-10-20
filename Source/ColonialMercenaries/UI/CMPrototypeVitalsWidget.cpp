// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "UI/CMPrototypeVitalsWidget.h"

#include "ColonialMercenariesCharacter.h"
#include "Combat/CMHealthComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> UCMPrototypeVitalsWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		return Super::RebuildWidget();
	}

	UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Canvas)
	{
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("VitalsCanvas"));
		WidgetTree->RootWidget = Canvas;
	}

	// Damage overlay fills the whole local-player viewport, but is
	// completely non-interactive and transparent outside brief hits.
	if (!IsValid(DamageOverlay))
	{
		DamageOverlay = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("DamageFlash"));
		DamageOverlay->SetBrush(*FCoreStyle::Get().GetBrush("WhiteBrush"));
		DamageOverlay->SetColorAndOpacity(FLinearColor(0.85f, 0.05f, 0.04f, 0.30f));
		DamageOverlay->SetRenderOpacity(0.0f);
		UCanvasPanelSlot* DamageCanvasSlot = Canvas->AddChildToCanvas(DamageOverlay);
		if (DamageCanvasSlot)
		{
			DamageCanvasSlot->SetAnchors(FAnchors(FVector2D::ZeroVector, FVector2D(1.0f, 1.0f)));
			DamageCanvasSlot->SetOffsets(FMargin(0.0f));
			DamageCanvasSlot->SetZOrder(-10);
		}
	}

	if (!IsValid(HealthBar))
	{
		HealthBar = WidgetTree->ConstructWidget<UProgressBar>(
			UProgressBar::StaticClass(), TEXT("PlayerHealthBar"));
		UCanvasPanelSlot* HealthCanvasSlot = Canvas->AddChildToCanvas(HealthBar);
		if (HealthCanvasSlot)
		{
			HealthCanvasSlot->SetPosition(FVector2D(22.0f, 23.0f));
			HealthCanvasSlot->SetSize(FVector2D(220.0f, 16.0f));
			HealthCanvasSlot->SetZOrder(10);
		}
	}

	if (!IsValid(VitalsLabel))
	{
		VitalsLabel = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("PlayerVitalsText"));
		UCanvasPanelSlot* VitalsCanvasSlot = Canvas->AddChildToCanvas(VitalsLabel);
		if (VitalsCanvasSlot)
		{
			VitalsCanvasSlot->SetPosition(FVector2D(22.0f, 45.0f));
			VitalsCanvasSlot->SetAutoSize(true);
			VitalsCanvasSlot->SetZOrder(11);
		}
	}

	if (!IsValid(InteractionLabel))
	{
		InteractionLabel = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("ReviveInteractionHint"));
		UCanvasPanelSlot* InteractionCanvasSlot = Canvas->AddChildToCanvas(InteractionLabel);
		if (InteractionCanvasSlot)
		{
			InteractionCanvasSlot->SetAnchors(FAnchors(0.5f, 0.78f));
			InteractionCanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			InteractionCanvasSlot->SetAutoSize(true);
			InteractionCanvasSlot->SetZOrder(12);
		}
	}

	if (!IsValid(ReviveProgressBar))
	{
		ReviveProgressBar = WidgetTree->ConstructWidget<UProgressBar>(
			UProgressBar::StaticClass(), TEXT("ReviveHoldProgress"));
		UCanvasPanelSlot* ProgressCanvasSlot = Canvas->AddChildToCanvas(ReviveProgressBar);
		if (ProgressCanvasSlot)
		{
			ProgressCanvasSlot->SetAnchors(FAnchors(0.5f, 0.82f));
			ProgressCanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			ProgressCanvasSlot->SetSize(FVector2D(220.0f, 11.0f));
			ProgressCanvasSlot->SetZOrder(12);
		}
	}
	return Super::RebuildWidget();
}

void UCMPrototypeVitalsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(VitalsLabel))
	{
		VitalsLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 17));
		VitalsLabel->SetShadowOffset(FVector2D(1.0f, 1.0f));
		VitalsLabel->SetShadowColorAndOpacity(FLinearColor::Black);
	}

	if (IsValid(InteractionLabel))
	{
		InteractionLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 20));
		InteractionLabel->SetJustification(ETextJustify::Center);
		InteractionLabel->SetShadowOffset(FVector2D(1.0f, 1.0f));
		InteractionLabel->SetShadowColorAndOpacity(FLinearColor::Black);
		InteractionLabel->SetVisibility(ESlateVisibility::Hidden);
	}

	if (IsValid(ReviveProgressBar))
	{
		ReviveProgressBar->SetFillColorAndOpacity(
			FLinearColor(0.30f, 0.95f, 0.8f, 1.0f));
		ReviveProgressBar->SetVisibility(ESlateVisibility::Hidden);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	RefreshDisplay();
	RefreshInteractionAndHitFlash();

	// UI only: a low-frequency local refresh for proximity, server-timed
	// progress, and brief damage fade. No Actor Tick or world-wide scans.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(HUDRefreshTimerHandle, this,
			&UCMPrototypeVitalsWidget::RefreshInteractionAndHitFlash,
			0.10f, true);
	}
}

void UCMPrototypeVitalsWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HUDRefreshTimerHandle);
	}
	SetTrackedHealth(nullptr);
	Super::NativeDestruct();
}

void UCMPrototypeVitalsWidget::SetTrackedHealth(UCMHealthComponent* NewHealth)
{
	if (TrackedHealth == NewHealth)
	{
		RefreshDisplay();
		return;
	}

	if (IsValid(TrackedHealth))
	{
		TrackedHealth->OnHealthChanged.RemoveDynamic(
			this, &UCMPrototypeVitalsWidget::HandleHealthChanged);
		TrackedHealth->OnDied.RemoveDynamic(
			this, &UCMPrototypeVitalsWidget::HandleDowned);
		TrackedHealth->OnRevived.RemoveDynamic(
			this, &UCMPrototypeVitalsWidget::HandleRevived);
	}

	TrackedHealth = NewHealth;
	LastDisplayedHealth = IsValid(TrackedHealth)
		? TrackedHealth->GetCurrentHealth() : -1.0f;
	if (IsValid(TrackedHealth))
	{
		TrackedHealth->OnHealthChanged.AddDynamic(
			this, &UCMPrototypeVitalsWidget::HandleHealthChanged);
		TrackedHealth->OnDied.AddDynamic(
			this, &UCMPrototypeVitalsWidget::HandleDowned);
		TrackedHealth->OnRevived.AddDynamic(
			this, &UCMPrototypeVitalsWidget::HandleRevived);
	}
	RefreshDisplay();
}

void UCMPrototypeVitalsWidget::HandleHealthChanged(float NewHealth, float NewMaxHealth)
{
	if (LastDisplayedHealth >= 0.0f && NewHealth < LastDisplayedHealth)
	{
		if (UWorld* World = GetWorld())
		{
			DamageFlashEndTime = World->GetTimeSeconds() + DamageFlashSeconds;
		}
	}
	LastDisplayedHealth = NewHealth;
	RefreshDisplay();
	RefreshInteractionAndHitFlash();
}

void UCMPrototypeVitalsWidget::HandleDowned()
{
	RefreshDisplay();
	RefreshInteractionAndHitFlash();
}

void UCMPrototypeVitalsWidget::HandleRevived()
{
	RefreshDisplay();
	RefreshInteractionAndHitFlash();
}

void UCMPrototypeVitalsWidget::RefreshDisplay()
{
	if (!IsValid(VitalsLabel) || !IsValid(HealthBar))
	{
		return;
	}

	if (!IsValid(TrackedHealth))
	{
		VitalsLabel->SetText(FText::FromString(TEXT("HP --")));
		HealthBar->SetPercent(0.0f);
		return;
	}

	const float MaxHealth = FMath::Max(TrackedHealth->GetMaxHealth(), 1.0f);
	const float HealthRatio = FMath::Clamp(
		TrackedHealth->GetCurrentHealth() / MaxHealth, 0.0f, 1.0f);
	HealthBar->SetPercent(HealthRatio);
	HealthBar->SetFillColorAndOpacity(HealthRatio > 0.33f
		? FLinearColor(0.3f, 0.86f, 0.66f, 1.0f)
		: FLinearColor(1.0f, 0.28f, 0.22f, 1.0f));

	const FString HealthText = TrackedHealth->IsDead()
		? TEXT("HP 0  |  INCAPACITATED")
		: FString::Printf(TEXT("HP  %d / %d"),
			FMath::CeilToInt(TrackedHealth->GetCurrentHealth()),
			FMath::CeilToInt(MaxHealth));
	VitalsLabel->SetText(FText::FromString(HealthText));
	VitalsLabel->SetColorAndOpacity(FSlateColor(TrackedHealth->IsDead()
		? FLinearColor(1.0f, 0.32f, 0.22f, 1.0f)
		: FLinearColor(0.8f, 0.96f, 0.87f, 1.0f)));
}

void UCMPrototypeVitalsWidget::RefreshInteractionAndHitFlash()
{
	if (IsValid(DamageOverlay))
	{
		const UWorld* World = GetWorld();
		const float SecondsLeft = World
			? FMath::Max(0.0f, DamageFlashEndTime - World->GetTimeSeconds()) : 0.0f;
		DamageOverlay->SetRenderOpacity(
			FMath::Clamp(SecondsLeft / FMath::Max(DamageFlashSeconds, 0.01f),
				0.0f, 1.0f));
	}

	if (!IsValid(InteractionLabel) || !IsValid(ReviveProgressBar))
	{
		return;
	}

	const APlayerController* Player = GetOwningPlayer();
	const AColonialMercenariesCharacter* Character = IsValid(Player)
		? Cast<AColonialMercenariesCharacter>(Player->GetPawn()) : nullptr;

	if (!IsValid(Character) || !IsValid(TrackedHealth))
	{
		InteractionLabel->SetVisibility(ESlateVisibility::Hidden);
		ReviveProgressBar->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	if (TrackedHealth->IsDead())
	{
		InteractionLabel->SetText(FText::FromString(
			TEXT("INCAPACITATED  -  Await teammate or respawn")));
		InteractionLabel->SetColorAndOpacity(
			FSlateColor(FLinearColor(1.0f, 0.40f, 0.28f, 1.0f)));
		InteractionLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
		ReviveProgressBar->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	if (Character->IsRevivingTeammate())
	{
		const float Progress = Character->GetReviveProgress();
		InteractionLabel->SetText(FText::FromString(FString::Printf(
			TEXT("REVIVING TEAMMATE  %d%%"), FMath::RoundToInt(Progress * 100.0f))));
		InteractionLabel->SetColorAndOpacity(
			FSlateColor(FLinearColor(0.45f, 1.0f, 0.83f, 1.0f)));
		InteractionLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
		ReviveProgressBar->SetPercent(Progress);
		ReviveProgressBar->SetVisibility(ESlateVisibility::HitTestInvisible);
		return;
	}

	ReviveProgressBar->SetVisibility(ESlateVisibility::Hidden);
	if (IsValid(Character->FindReviveCandidate()))
	{
		InteractionLabel->SetText(FText::FromString(
			TEXT("HOLD [E]  -  REVIVE TEAMMATE")));
		InteractionLabel->SetColorAndOpacity(
			FSlateColor(FLinearColor(0.91f, 0.97f, 0.94f, 1.0f)));
		InteractionLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		InteractionLabel->SetVisibility(ESlateVisibility::Hidden);
	}
}

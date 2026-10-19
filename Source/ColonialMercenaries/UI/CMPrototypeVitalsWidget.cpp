// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "UI/CMPrototypeVitalsWidget.h"

#include "Combat/CMHealthComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> UCMPrototypeVitalsWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("VitalsCanvas"));
		WidgetTree->RootWidget = Canvas;
	}

	if (WidgetTree && !IsValid(VitalsLabel))
	{
		if (UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget))
		{
			VitalsLabel = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(), TEXT("PlayerVitalsText"));
			UCanvasPanelSlot* VitalsCanvasSlot = Canvas->AddChildToCanvas(VitalsLabel);
			if (VitalsCanvasSlot)
			{
				VitalsCanvasSlot->SetPosition(FVector2D(22.0f, 22.0f));
				VitalsCanvasSlot->SetAutoSize(true);
				VitalsCanvasSlot->SetZOrder(20);
			}
		}
	}

	return Super::RebuildWidget();
}

void UCMPrototypeVitalsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(VitalsLabel))
	{
		VitalsLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 19));
		VitalsLabel->SetShadowOffset(FVector2D(1.0f, 1.0f));
		VitalsLabel->SetShadowColorAndOpacity(
			FLinearColor(0.0f, 0.0f, 0.0f, 0.95f));
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
	RefreshDisplay();
}

void UCMPrototypeVitalsWidget::NativeDestruct()
{
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
	RefreshDisplay();
}

void UCMPrototypeVitalsWidget::HandleDowned()
{
	RefreshDisplay();
}

void UCMPrototypeVitalsWidget::HandleRevived()
{
	RefreshDisplay();
}

void UCMPrototypeVitalsWidget::RefreshDisplay()
{
	if (!IsValid(VitalsLabel))
	{
		return;
	}

	if (!IsValid(TrackedHealth))
	{
		VitalsLabel->SetText(FText::FromString(TEXT("HP --")));
		return;
	}

	if (TrackedHealth->IsDead())
	{
		VitalsLabel->SetText(FText::FromString(TEXT(
			"INCAPACITATED  |  Teammate: press E to revive")));
		VitalsLabel->SetColorAndOpacity(
			FSlateColor(FLinearColor(1.0f, 0.32f, 0.22f, 1.0f)));
	}
	else
	{
		VitalsLabel->SetText(FText::FromString(FString::Printf(
			TEXT("HP  %d / %d"),
			FMath::CeilToInt(TrackedHealth->GetCurrentHealth()),
			FMath::CeilToInt(TrackedHealth->GetMaxHealth()))));
		VitalsLabel->SetColorAndOpacity(
			FSlateColor(FLinearColor(0.8f, 0.96f, 0.87f, 1.0f)));
	}
}

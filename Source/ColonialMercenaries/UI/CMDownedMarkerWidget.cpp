// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "UI/CMDownedMarkerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> UCMDownedMarkerWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		Label = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("TeammateDownedLabel"));
		WidgetTree->RootWidget = Label;
	}
	return Super::RebuildWidget();
}

void UCMDownedMarkerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(Label))
	{
		Label->SetText(FText::FromString(TEXT("DOWNED")));
		Label->SetJustification(ETextJustify::Center);
		Label->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 21));
		Label->SetColorAndOpacity(
			FSlateColor(FLinearColor(1.0f, 0.36f, 0.20f, 1.0f)));
		Label->SetShadowOffset(FVector2D(1.0f, 1.0f));
		Label->SetShadowColorAndOpacity(FLinearColor::Black);
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "UI/CMPrototypeMissionTrackerWidget.h"

#include "ColonialMercenariesCharacter.h"
#include "World/CMPrototypeContractTerminal.h"
#include "World/CMPrototypeIntelCache.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> UCMPrototypeMissionTrackerWidget::RebuildWidget()
{
    // Native HUD follows the same pre-Slate widget construction pattern
    // as the existing Vitals and Reticle UI. No Blueprint asset required.
    if (!WidgetTree)
    {
        return Super::RebuildWidget();
    }

    UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
    if (!Canvas)
    {
        Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(
            UCanvasPanel::StaticClass(), TEXT("MissionTrackerCanvas"));
        WidgetTree->RootWidget = Canvas;
    }

    if (!IsValid(ObjectiveLabel))
    {
        UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(), TEXT("MissionTrackerBackground"));
        Panel->SetBrushColor(FLinearColor(0.015f, 0.028f, 0.035f, 0.83f));
        Panel->SetPadding(FMargin(13.0f, 9.0f, 13.0f, 10.0f));

        UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel);
        PanelSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
        PanelSlot->SetAlignment(FVector2D(1.0f, 0.0f));
        PanelSlot->SetPosition(FVector2D(-22.0f, 22.0f));
        PanelSlot->SetSize(FVector2D(340.0f, 144.0f));

        UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(), TEXT("MissionTrackerContent"));
        Panel->SetContent(Content);

        TitleLabel = WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(), TEXT("MissionTitle"));
        ObjectiveLabel = WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(), TEXT("MissionObjective"));
        DetailLabel = WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(), TEXT("MissionDetail"));
        CompanyFundsLabel = WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(), TEXT("MissionCompanyFunds"));

        for (UTextBlock* TextWidget : {
            TitleLabel.Get(), ObjectiveLabel.Get(), DetailLabel.Get(),
            CompanyFundsLabel.Get() })
        {
            TextWidget->SetAutoWrapText(true);
            UVerticalBoxSlot* Row = Content->AddChildToVerticalBox(TextWidget);
            if (Row)
            {
                Row->SetPadding(FMargin(0.0f, 3.0f));
            }
        }
    }
    return Super::RebuildWidget();
}

void UCMPrototypeMissionTrackerWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (IsValid(TitleLabel))
    {
        TitleLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 15));
        TitleLabel->SetColorAndOpacity(
            FSlateColor(FLinearColor(0.40f, 0.94f, 0.77f, 1.0f)));
    }
    if (IsValid(ObjectiveLabel))
    {
        ObjectiveLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
        ObjectiveLabel->SetColorAndOpacity(
            FSlateColor(FLinearColor(0.97f, 0.98f, 0.96f, 1.0f)));
    }
    if (IsValid(DetailLabel))
    {
        DetailLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 13));
        DetailLabel->SetColorAndOpacity(
            FSlateColor(FLinearColor(0.75f, 0.83f, 0.84f, 1.0f)));
    }
    if (IsValid(CompanyFundsLabel))
    {
        CompanyFundsLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 13));
        CompanyFundsLabel->SetColorAndOpacity(
            FSlateColor(FLinearColor(0.95f, 0.83f, 0.51f, 1.0f)));
    }

    SetVisibility(ESlateVisibility::HitTestInvisible);
    RefreshMissionTracker();
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            MissionRefreshTimerHandle, this,
            &UCMPrototypeMissionTrackerWidget::RefreshMissionTracker,
            FMath::Max(RefreshIntervalSeconds, 0.15f), true);
    }
}

void UCMPrototypeMissionTrackerWidget::NativeDestruct()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MissionRefreshTimerHandle);
    }
    TrackedTerminal = nullptr;
    Super::NativeDestruct();
}

void UCMPrototypeMissionTrackerWidget::RefreshMissionTracker()
{
    if (!IsValid(TitleLabel) || !IsValid(ObjectiveLabel)
        || !IsValid(DetailLabel) || !IsValid(CompanyFundsLabel))
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!IsValid(TrackedTerminal) && World)
    {
        // Only search until this local widget locates the level's single
        // prototype terminal. No global scans on the normal HUD refresh.
        for (TActorIterator<ACMPrototypeContractTerminal> It(World); It; ++It)
        {
            TrackedTerminal = *It;
            break;
        }
    }

    if (!IsValid(TrackedTerminal))
    {
        // Lvl_FactionTest has no contract actors: don't clutter its HUD.
        SetVisibility(ESlateVisibility::Collapsed);
        return;
    }
    SetVisibility(ESlateVisibility::HitTestInvisible);

    const ACMPrototypeIntelCache* IntelCache = TrackedTerminal->GetIntelCache();
    const AColonialMercenariesCharacter* LocalCharacter =
        Cast<AColonialMercenariesCharacter>(GetOwningPlayerPawn());

    FString ObjectiveText;
    FString DetailsText;
    const ECMPrototypeContractStage Stage = TrackedTerminal->GetContractStage();

    switch (Stage)
    {
    case ECMPrototypeContractStage::Available:
        ObjectiveText = TEXT("No active contract");
        DetailsText = TEXT("Visit Mercenary HQ to accept a job");
        break;

    case ECMPrototypeContractStage::Active:
        ObjectiveText = TEXT("Recover relay intelligence");
        if (IsValid(IntelCache) && IsValid(LocalCharacter))
        {
            const int32 Metres = FMath::RoundToInt(
                FVector::Dist(LocalCharacter->GetActorLocation(),
                    IntelCache->GetActorLocation()) / 100.0f);
            DetailsText = FString::Printf(
                TEXT("Intel cache at relay | %dm away"), Metres);
        }
        else
        {
            DetailsText = TEXT("Retrieve the intel package at the relay");
        }
        break;

    case ECMPrototypeContractStage::IntelRecovered:
        if (!IsValid(IntelCache) || !IsValid(IntelCache->GetCarrier()))
        {
            ObjectiveText = TEXT("Recover the dropped intel");
            DetailsText = TEXT("A teammate can retrieve the package");
            if (IsValid(IntelCache) && IsValid(LocalCharacter))
            {
                const int32 Metres = FMath::RoundToInt(
                    FVector::Dist(LocalCharacter->GetActorLocation(),
                        IntelCache->GetActorLocation()) / 100.0f);
                DetailsText = FString::Printf(
                    TEXT("Dropped package | %dm away"), Metres);
            }
        }
        else if (IntelCache->GetCarrier() == LocalCharacter)
        {
            ObjectiveText = TEXT("Deliver intel to Mercenary HQ");
            const int32 Metres = FMath::RoundToInt(
                FVector::Dist(LocalCharacter->GetActorLocation(),
                    TrackedTerminal->GetActorLocation()) / 100.0f);
            DetailsText = FString::Printf(
                TEXT("You carry the package | HQ %dm away"), Metres);
        }
        else
        {
            ObjectiveText = TEXT("Escort the intel carrier");
            DetailsText = TEXT("Teammate has package | Return to HQ");
        }
        break;

    case ECMPrototypeContractStage::Paid:
        ObjectiveText = TEXT("Contract complete");
        DetailsText = TEXT("Relay intel delivered | Payment received");
        break;

    default:
        ObjectiveText = TEXT("Contract status unavailable");
        break;
    }

    TitleLabel->SetText(FText::FromString(TEXT("RELAY INTELLIGENCE RECOVERY")));
    ObjectiveLabel->SetText(FText::FromString(ObjectiveText));
    DetailLabel->SetText(FText::FromString(DetailsText));
    CompanyFundsLabel->SetText(FText::FromString(FString::Printf(
        TEXT("COMPANY FUNDS   %d CR"), TrackedTerminal->GetCompanyCredits())));
}

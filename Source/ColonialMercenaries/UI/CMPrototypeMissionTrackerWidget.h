// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "CMPrototypeMissionTrackerWidget.generated.h"

class ACMPrototypeContractTerminal;
class UTextBlock;

/**
 * Local-only, data-driven prototype mission tracker. Reads replicated
 * contract/cache state and never changes gameplay or calls server RPCs.
 * The small UI refresh timer avoids a per-frame Tick.
 */
UCLASS(Blueprintable)
class COLONIALMERCENARIES_API UCMPrototypeMissionTrackerWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /** HUD refresh only: no actor Tick or repeated world scans once found. */
    UPROPERTY(EditDefaultsOnly, Category="Colonial Mercenaries|Mission HUD",
        meta=(ClampMin="0.15", UIMin="0.25", UIMax="2.0"))
    float RefreshIntervalSeconds = 0.35f;

private:
    void RefreshMissionTracker();

    /** Cached after an initial discovery of the placed contract actor. */
    UPROPERTY(Transient)
    TObjectPtr<ACMPrototypeContractTerminal> TrackedTerminal;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TitleLabel;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ObjectiveLabel;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> DetailLabel;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> CompanyFundsLabel;

    FTimerHandle MissionRefreshTimerHandle;
};

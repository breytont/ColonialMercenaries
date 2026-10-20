// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CMDownedMarkerWidget.generated.h"

class UTextBlock;

/** Screen-space marker above an incapacitated co-op player. */
UCLASS(Blueprintable)
class COLONIALMERCENARIES_API UCMDownedMarkerWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Label;
};

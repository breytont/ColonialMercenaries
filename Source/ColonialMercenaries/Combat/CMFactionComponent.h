// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CMFactionComponent.generated.h"

/**
 * Replicated faction membership for actors (infantry first; vehicles later).
 * The prototype considers different valid factions hostile. A strategic
 * relationship service will replace that temporary rule in a later milestone.
 */
UCLASS(ClassGroup=(ColonialMercenaries), meta=(BlueprintSpawnableComponent))
class COLONIALMERCENARIES_API UCMFactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCMFactionComponent();

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Factions")
	FGameplayTag GetFactionId() const { return FactionId; }

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Factions")
	bool IsHostileTo(const UCMFactionComponent* Other) const;

protected:
	/** Editable in actor instances or Blueprint defaults. An unset tag is neutral. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category="Colonial Mercenaries|Factions")
	FGameplayTag FactionId;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "Combat/CMFactionComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UCMFactionComponent::UCMFactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCMFactionComponent::SetFactionId(FGameplayTag NewFactionId)
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner) || !Owner->HasAuthority())
	{
		return;
	}

	if (FactionId != NewFactionId)
	{
		FactionId = NewFactionId;
		Owner->ForceNetUpdate();
	}
}

bool UCMFactionComponent::IsHostileTo(const UCMFactionComponent* Other) const
{
	// Initial prototype rule only: valid, different faction IDs are opponents.
	return IsValid(Other) && FactionId.IsValid() && Other->FactionId.IsValid()
		&& FactionId != Other->FactionId;
}

void UCMFactionComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCMFactionComponent, FactionId);
}

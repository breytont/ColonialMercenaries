// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "Combat/CMFactionComponent.h"
#include "Net/UnrealNetwork.h"

UCMFactionComponent::UCMFactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
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

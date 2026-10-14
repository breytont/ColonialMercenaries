// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#include "Combat/CMHealthComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UCMHealthComponent::UCMHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCMHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner(); IsValid(Owner) && Owner->HasAuthority())
	{
		CurrentHealth = FMath::Max(MaxHealth, 1.0f);
		Owner->OnTakeAnyDamage.AddDynamic(this, &UCMHealthComponent::HandleAnyDamage);
	}
}

void UCMHealthComponent::HandleAnyDamage(AActor* DamagedActor, float Damage,
	const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || Damage <= 0.0f || IsDead())
	{
		return;
	}

	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth);
	if (CurrentHealth != PreviousHealth)
	{
		OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
		if (CurrentHealth <= 0.0f && !bDeathEventFired)
		{
			bDeathEventFired = true;
			OnDied.Broadcast();
		}
	}
}

void UCMHealthComponent::OnRep_CurrentHealth()
{
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	if (CurrentHealth <= 0.0f && !bDeathEventFired)
	{
		bDeathEventFired = true;
		OnDied.Broadcast();
	}
}

void UCMHealthComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCMHealthComponent, CurrentHealth);
}

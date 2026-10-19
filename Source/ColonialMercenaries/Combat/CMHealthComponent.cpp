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

bool UCMHealthComponent::RestoreHealth(float HealthAfterRecovery)
{
	const AActor* Owner = GetOwner();
	if (!IsValid(Owner) || !Owner->HasAuthority() || !FMath::IsFinite(HealthAfterRecovery)
		|| HealthAfterRecovery <= 0.0f)
	{
		return false;
	}

	const bool bWasDead = IsDead();
	const float Restored = FMath::Clamp(HealthAfterRecovery, 1.0f, FMath::Max(MaxHealth, 1.0f));
	if (FMath::IsNearlyEqual(CurrentHealth, Restored))
	{
		return true;
	}

	CurrentHealth = Restored;
	bDeathEventFired = false;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	if (bWasDead)
	{
		OnRevived.Broadcast();
	}
	GetOwner()->ForceNetUpdate();
	return true;
}

void UCMHealthComponent::OnRep_CurrentHealth()
{
	const bool bWasDead = bDeathEventFired;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	if (CurrentHealth <= 0.0f && !bDeathEventFired)
	{
		bDeathEventFired = true;
		OnDied.Broadcast();
	}
	else if (CurrentHealth > 0.0f && bWasDead)
	{
		bDeathEventFired = false;
		OnRevived.Broadcast();
	}
}

void UCMHealthComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCMHealthComponent, CurrentHealth);
}

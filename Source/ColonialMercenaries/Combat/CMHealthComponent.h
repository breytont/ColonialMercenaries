// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CMHealthComponent.generated.h"

class AController;
class UDamageType;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCMHealthChangedSignature, float, CurrentHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCMDiedSignature);

/**
 * Reusable server-authoritative damage and replicated health.
 * Attach to a replicated Actor; damage the owner with Unreal ApplyDamage.
 * No polling/Tick required.
 */
UCLASS(ClassGroup=(ColonialMercenaries), meta=(BlueprintSpawnableComponent))
class COLONIALMERCENARIES_API UCMHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCMHealthComponent();

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Health")
	bool IsDead() const { return CurrentHealth <= 0.0f; }

	/** Also broadcasts on clients when the replicated health changes. */
	UPROPERTY(BlueprintAssignable, Category="Colonial Mercenaries|Health")
	FCMHealthChangedSignature OnHealthChanged;

	/** Fires on the server upon lethal damage and on clients upon replication. */
	UPROPERTY(BlueprintAssignable, Category="Colonial Mercenaries|Health")
	FCMDiedSignature OnDied;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	/** Tunable for each soldier, vehicle, or equipment Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Health", meta=(ClampMin="1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(ReplicatedUsing=OnRep_CurrentHealth, BlueprintReadOnly, Category="Colonial Mercenaries|Health")
	float CurrentHealth = 100.0f;

private:
	UFUNCTION()
	void HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void OnRep_CurrentHealth();

	/** Replication callback state; not gameplay-authoritative. */
	bool bDeathEventFired = false;
};

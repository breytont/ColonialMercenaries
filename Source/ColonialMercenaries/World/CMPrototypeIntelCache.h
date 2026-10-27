// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CMPrototypeIntelCache.generated.h"

class AColonialMercenariesCharacter;
class ACMPrototypeContractTerminal;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class FLifetimeProperty;

/** In-world, server-authoritative objective used by the first contract. */
UCLASS(Blueprintable, meta=(DisplayName="CM Prototype Intel Cache"))
class COLONIALMERCENARIES_API ACMPrototypeIntelCache : public AActor
{
    GENERATED_BODY()

public:
    ACMPrototypeIntelCache();
    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Contracts")
    ACMPrototypeContractTerminal* GetContractTerminal() const
    {
        return ContractTerminal;
    }

    void TryInteract(AColonialMercenariesCharacter* Player);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Contracts|Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Contracts|Components")
    TObjectPtr<UStaticMeshComponent> CacheBody;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Contracts|Components")
    TObjectPtr<UTextRenderComponent> StatusLabel;

    /** Set this to the contract terminal in Mercenary HQ (editor actor reference). */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Contracts|Setup")
    TObjectPtr<ACMPrototypeContractTerminal> ContractTerminal;

    UPROPERTY(ReplicatedUsing=OnRep_Recovered, BlueprintReadOnly,
        Category="Colonial Mercenaries|Contracts|State")
    bool bRecovered = false;

private:
    UFUNCTION()
    void OnRep_Recovered();
    void RefreshStatusLabel();
};

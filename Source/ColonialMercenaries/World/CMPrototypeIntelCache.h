// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CMPrototypeIntelCache.generated.h"

class AColonialMercenariesCharacter;
class ACMPrototypeContractTerminal;
class AController;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class FLifetimeProperty;

/**
 * Physical world item. The company shares the contract, but an individual
 * player physically carries the intel until delivery or incapacitation.
 * No full inventory framework is required for this first mission.
 */
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

    UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Contracts")
    AColonialMercenariesCharacter* GetCarrier() const
    {
        return Carrier;
    }

    UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Contracts")
    bool IsCarriedBy(const AColonialMercenariesCharacter* Player) const;

    /** Only a linked HQ terminal can complete delivery on the server. */
    bool FinalizeDelivery(AColonialMercenariesCharacter* Player);

    /** Validated through the owning character's interaction RPC. */
    void TryInteract(AColonialMercenariesCharacter* Player);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Contracts|Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Contracts|Components")
    TObjectPtr<UStaticMeshComponent> CacheBody;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Contracts|Components")
    TObjectPtr<UTextRenderComponent> StatusLabel;

    /** Set this to the contract terminal in Mercenary HQ (editor actor reference). */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Contracts|Setup")
    TObjectPtr<ACMPrototypeContractTerminal> ContractTerminal;

    /** Server-selected physical carrier. Replicated to every player. */
    UPROPERTY(ReplicatedUsing=OnRep_Carrier, BlueprintReadOnly,
        Category="Colonial Mercenaries|Contracts|State")
    TObjectPtr<AColonialMercenariesCharacter> Carrier;

    UPROPERTY(ReplicatedUsing=OnRep_Delivered, BlueprintReadOnly,
        Category="Colonial Mercenaries|Contracts|State")
    bool bDelivered = false;

private:
    /** The actual player Controller may disappear before its Pawn. */
    TWeakObjectPtr<AController> CarrierController;

    UFUNCTION()
    void OnRep_Carrier();

    UFUNCTION()
    void OnRep_Delivered();

    UFUNCTION()
    void OnCarrierIncapacitated();

    UFUNCTION()
    void OnCarrierActorDestroyed(AActor* DestroyedActor);

    UFUNCTION()
    void OnCarrierControllerDestroyed(AActor* DestroyedActor);

    void UnbindCarrierDelegates();
    void DropPhysicalIntel();
    void RefreshPresentation();
};

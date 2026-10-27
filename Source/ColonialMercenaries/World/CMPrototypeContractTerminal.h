// Copyright (c) Colonial Mercenaries contributors. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CMPrototypeContractTerminal.generated.h"

class AColonialMercenariesCharacter;
class ACMPrototypeIntelCache;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class FLifetimeProperty;

/** Small initial company-wide contract loop; no persistence in this milestone. */
UENUM(BlueprintType)
enum class ECMPrototypeContractStage : uint8
{
    Available UMETA(DisplayName="Available"),
    Active UMETA(DisplayName="Accepted"),
    IntelRecovered UMETA(DisplayName="Intel Recovered"),
    Paid UMETA(DisplayName="Paid")
};

/**
 * World-placed HQ terminal, authoritative shared contract and company ledger.
 * One reward is issued for the entire company, never per interacting player.
 * The existing faction war continues independently of this state machine.
 */
UCLASS(Blueprintable, meta=(DisplayName="CM Prototype Contract Terminal"))
class COLONIALMERCENARIES_API ACMPrototypeContractTerminal : public AActor
{
    GENERATED_BODY()

public:
    ACMPrototypeContractTerminal();

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Contracts")
    ECMPrototypeContractStage GetContractStage() const { return ContractStage; }

    UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Contracts")
    int32 GetCompanyCredits() const { return CompanyCredits; }

    /** Read-only objective pointer for the local HUD. No gameplay authority. */
    UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Contracts")
    ACMPrototypeIntelCache* GetIntelCache() const { return IntelCache; }

    /** Called by an authenticated, server-owned player's interaction RPC. */
    void TryInteract(AColonialMercenariesCharacter* Player);

    /** Intel cache can only complete the job linked to this contract. */
    bool TryRecoverIntel(AColonialMercenariesCharacter* Player,
        ACMPrototypeIntelCache* Cache);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Contracts|Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Contracts|Components")
    TObjectPtr<UStaticMeshComponent> TerminalBody;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Contracts|Components")
    TObjectPtr<UTextRenderComponent> StatusLabel;

    /** Assign the instance of CM Prototype Intel Cache at the relay. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Contracts|Setup")
    TObjectPtr<ACMPrototypeIntelCache> IntelCache;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Contracts|Terms",
        meta=(ClampMin="1", ClampMax="1000000"))
    int32 RewardCredits = 1500;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Colonial Mercenaries|Contracts|Terms",
        meta=(ClampMin="0", ClampMax="100000000"))
    int32 StartingCompanyCredits = 0;

    UPROPERTY(ReplicatedUsing=OnRep_ContractUpdated, BlueprintReadOnly,
        Category="Colonial Mercenaries|Contracts|State")
    ECMPrototypeContractStage ContractStage = ECMPrototypeContractStage::Available;

    UPROPERTY(ReplicatedUsing=OnRep_ContractUpdated, BlueprintReadOnly,
        Category="Colonial Mercenaries|Contracts|State")
    int32 CompanyCredits = 0;

private:
    UFUNCTION()
    void OnRep_ContractUpdated();

    void RefreshStatusLabel();
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "ColonialMercenariesCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UCMPrototypeWeaponComponent;
class UCMHealthComponent;
class UCMFactionComponent;
class UCMPrototypeVitalsWidget;
class UCMPrototypeReticleWidget;
class USceneComponent;
class UStaticMeshComponent;
class UWidgetComponent;
struct FInputActionValue;
class FLifetimeProperty;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AColonialMercenariesCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** Temporary firearm. Kept separate from input so weapons can evolve. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCMPrototypeWeaponComponent> PrototypeWeapon;

	/** Reusable replicated player health; zero means incapacitated for players. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCMHealthComponent> HealthComponent;

	/** Editable test allegiance (unassigned means neutral to prototype factions). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCMFactionComponent> FactionComponent;

	/** Network-visible screen-space marker above a downed teammate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UWidgetComponent> DownedMarker;

	/** Editable hand_r attachment: adjust the transform in the character BP. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> PrototypeRifleRoot;

	/** Simple engine-mesh rifle parts; no third-party assets needed. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> PrototypeRifleReceiver;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> PrototypeRifleBarrel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> PrototypeRifleStock;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> PrototypeRifleGrip;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> PrototypeRifleSight;

	/** Visual anchor for later aligning gunshot VFX and actual projectiles. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> PrototypeRifleMuzzle;

	/** Left-hand grip marker attached to the rifle, independent of mannequin animation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> PrototypeRifleSupportGrip;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	/** Press to fire one server-validated prototype shot (IA_Fire). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Combat")
	TObjectPtr<UInputAction> FireAction;

	/** Hold to face the look direction and move into shoulder aim (IA_Aim). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Combat")
	TObjectPtr<UInputAction> AimAction;

	/** Hold near a downed teammate to revive (IA_Revive, mapped to E). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Combat")
	TObjectPtr<UInputAction> ReviveAction;

	/** First contract interaction (IA_Interact, recommended F in IMC_Default). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Interaction")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Aiming",
		meta=(ClampMin="100.0"))
	float AimedCameraArmLength = 275.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Aiming")
	FVector AimedCameraSocketOffset = FVector(0.0f, 55.0f, 65.0f);

	/** Seconds for a smooth camera transition; zero gives an instant switch. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Aiming",
		meta=(ClampMin="0.0", UIMin="0.0"))
	float AimCameraBlendDuration = 0.22f;

	/** Downed teammates have this much time to receive a revive. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Casualties",
		meta=(ClampMin="1.0"))
	float BleedOutSeconds = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Casualties",
		meta=(ClampMin="1.0"))
	float ReviveHealth = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Casualties",
		meta=(ClampMin="50.0"))
	float ReviveRange = 230.0f;

	/** Hold-E time to revive, validated by the server. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Casualties",
		meta=(ClampMin="0.2"))
	float ReviveHoldSeconds = 1.6f;

	UPROPERTY(EditDefaultsOnly, Category="Colonial Mercenaries|Casualties")
	TSubclassOf<UCMPrototypeVitalsWidget> VitalsWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UCMPrototypeVitalsWidget> ActiveVitalsWidget;

	/** Local-only physical muzzle reticle; replaceable with a UMG Blueprint. */
	UPROPERTY(EditDefaultsOnly, Category="Colonial Mercenaries|Aiming|Reticle")
	TSubclassOf<UCMPrototypeReticleWidget> ReticleWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UCMPrototypeReticleWidget> ActiveReticleWidget;

public:

	/** Constructor */
	AColonialMercenariesCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Combat")
	bool IsAiming() const { return bIsAiming; }

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Combat")
	UCMPrototypeWeaponComponent* GetPrototypeWeapon() const { return PrototypeWeapon; }

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Combat")
	USceneComponent* GetPrototypeRifleMuzzle() const { return PrototypeRifleMuzzle; }

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Combat")
	UCMHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Combat")
	UCMFactionComponent* GetFactionComponent() const { return FactionComponent; }

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Combat")
	bool IsIncapacitated() const;

	/** Nearby downed teammate with line of sight; local HUD queries at 5-10 Hz. */
	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Casualties")
	AColonialMercenariesCharacter* FindReviveCandidate() const;

	/** Cosmetic progress from replicated start time; completion is server-only. */
	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Casualties")
	float GetReviveProgress() const;

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Casualties")
	bool IsRevivingTeammate() const { return IsValid(ActiveReviveTarget); }

	/** IK effector position relative to the mannequin hand_r bone.
	 * The support grip rides with the weapon; IK is solved against hand_r
	 * within the AnimGraph rather than tracking a previous-frame world pose.
	 */
	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Combat")
	FVector GetPrototypeRifleSupportGripInRightHandSpace() const;

protected:

	virtual void BeginPlay() override;
	virtual void PawnClientRestart() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void EnsureLocalReticleWidget();
	void EnsureLocalVitalsWidget();

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	void Fire();

	/** Pick the nearest nearby prototype contract object without a global scan. */
	void InteractWithNearbyContract();

	/** The client cannot own contract actors; route input via owned Character. */
	UFUNCTION(Server, Reliable)
	void ServerTryContractInteract(AActor* TargetActor);
	void StartReviveHold();
	void CancelReviveHold();
	void StartReviveHoldOnAuthority();
	void CancelReviveHoldOnAuthority();
	void UpdateReviveHoldOnAuthority();
	bool CanReviveCandidate(const AColonialMercenariesCharacter* Candidate) const;

	UFUNCTION(Server, Reliable)
	void ServerStartReviveHold();

	UFUNCTION(Server, Reliable)
	void ServerCancelReviveHold();

	UFUNCTION()
	void HandlePlayerDowned();

	UFUNCTION()
	void HandlePlayerRevived();

	void FinishBleedOut();

	void BeginAim();
	void EndAim();
	void SetAiming(bool bNewAiming);
	void UpdateAimingPresentation();

	/** A short-lived camera timer, only active while entering/exiting aim. */
	void UpdateAimCameraBlend();

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bNewAiming);

	UFUNCTION()
	void OnRep_Aiming();

	UPROPERTY(ReplicatedUsing=OnRep_Aiming, BlueprintReadOnly,
		Category="Colonial Mercenaries|Combat", meta=(AllowPrivateAccess="true"))
	bool bIsAiming = false;

	float NormalCameraArmLength = 400.0f;
	FVector NormalCameraSocketOffset = FVector::ZeroVector;
	FTimerHandle AimCameraBlendTimerHandle;
	FTimerHandle BleedOutTimerHandle;
	FTimerHandle ReviveHoldTimerHandle;

	/** Owner-only HUD state, not client-authoritative revive permission. */
	UPROPERTY(Replicated, Transient)
	TObjectPtr<AColonialMercenariesCharacter> ActiveReviveTarget;

	UPROPERTY(Replicated, Transient)
	float ReviveStartServerTime = 0.0f;
	float AimCameraBlendStartTime = 0.0f;
	float AimCameraBlendStartArm = 400.0f;
	FVector AimCameraBlendStartOffset = FVector::ZeroVector;

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};


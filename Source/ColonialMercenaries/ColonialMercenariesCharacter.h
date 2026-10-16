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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Aiming",
		meta=(ClampMin="100.0"))
	float AimedCameraArmLength = 275.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Colonial Mercenaries|Aiming")
	FVector AimedCameraSocketOffset = FVector(0.0f, 55.0f, 15.0f);

public:

	/** Constructor */
	AColonialMercenariesCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Combat")
	bool IsAiming() const { return bIsAiming; }

	UFUNCTION(BlueprintPure, Category="Colonial Mercenaries|Combat")
	UCMPrototypeWeaponComponent* GetPrototypeWeapon() const { return PrototypeWeapon; }

protected:

	virtual void BeginPlay() override;

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	void Fire();
	void BeginAim();
	void EndAim();
	void SetAiming(bool bNewAiming);
	void UpdateAimingPresentation();

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bNewAiming);

	UFUNCTION()
	void OnRep_Aiming();

	UPROPERTY(ReplicatedUsing=OnRep_Aiming, BlueprintReadOnly,
		Category="Colonial Mercenaries|Combat", meta=(AllowPrivateAccess="true"))
	bool bIsAiming = false;

	float NormalCameraArmLength = 400.0f;
	FVector NormalCameraSocketOffset = FVector::ZeroVector;

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


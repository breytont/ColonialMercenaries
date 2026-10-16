// Copyright Epic Games, Inc. All Rights Reserved.

#include "ColonialMercenariesCharacter.h"
#include "Combat/CMPrototypeWeaponComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ColonialMercenaries.h"

AColonialMercenariesCharacter::AColonialMercenariesCharacter()
{
	bReplicates = true;
	PrototypeWeapon = CreateDefaultSubobject<UCMPrototypeWeaponComponent>(TEXT("PrototypeWeapon"));
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AColonialMercenariesCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Respect camera defaults configured in BP_ThirdPersonCharacter.
	NormalCameraArmLength = CameraBoom->TargetArmLength;
	NormalCameraSocketOffset = CameraBoom->SocketOffset;
	UpdateAimingPresentation();
}

void AColonialMercenariesCharacter::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AColonialMercenariesCharacter, bIsAiming);
}

void AColonialMercenariesCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AColonialMercenariesCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AColonialMercenariesCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AColonialMercenariesCharacter::Look);

		// These actions are assigned in BP_ThirdPersonCharacter and mapped
		// through IMC_Default. Existing move, look and jump remain unchanged.
		if (FireAction)
		{
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started,
				this, &AColonialMercenariesCharacter::Fire);
		}
		if (AimAction)
		{
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started,
				this, &AColonialMercenariesCharacter::BeginAim);
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed,
				this, &AColonialMercenariesCharacter::EndAim);
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Canceled,
				this, &AColonialMercenariesCharacter::EndAim);
		}
	}
	else
	{
		UE_LOG(LogColonialMercenaries, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AColonialMercenariesCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AColonialMercenariesCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AColonialMercenariesCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AColonialMercenariesCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AColonialMercenariesCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AColonialMercenariesCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void AColonialMercenariesCharacter::Fire()
{
	if (IsLocallyControlled() && IsValid(PrototypeWeapon))
	{
		PrototypeWeapon->RequestFire();
	}
}

void AColonialMercenariesCharacter::BeginAim()
{
	if (IsLocallyControlled())
	{
		SetAiming(true);
	}
}

void AColonialMercenariesCharacter::EndAim()
{
	if (IsLocallyControlled())
	{
		SetAiming(false);
	}
}

void AColonialMercenariesCharacter::SetAiming(bool bNewAiming)
{
	if (bIsAiming == bNewAiming)
	{
		return;
	}

	// Predict aiming presentation instantly for the local player.
	bIsAiming = bNewAiming;
	UpdateAimingPresentation();

	if (!HasAuthority())
	{
		ServerSetAiming(bNewAiming);
	}
}

void AColonialMercenariesCharacter::ServerSetAiming_Implementation(bool bNewAiming)
{
	bIsAiming = bNewAiming;
	UpdateAimingPresentation();
}

void AColonialMercenariesCharacter::OnRep_Aiming()
{
	UpdateAimingPresentation();
}

void AColonialMercenariesCharacter::UpdateAimingPresentation()
{
	if (!IsValid(CameraBoom))
	{
		return;
	}

	bUseControllerRotationYaw = bIsAiming;
	GetCharacterMovement()->bOrientRotationToMovement = !bIsAiming;

	CameraBoom->TargetArmLength = bIsAiming
		? FMath::Max(AimedCameraArmLength, 100.0f) : NormalCameraArmLength;
	CameraBoom->SocketOffset = bIsAiming
		? AimedCameraSocketOffset : NormalCameraSocketOffset;
}

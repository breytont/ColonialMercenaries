// Copyright Epic Games, Inc. All Rights Reserved.

#include "ColonialMercenariesCharacter.h"
#include "Combat/CMPrototypeWeaponComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
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

	// Lightweight placeholder rifle built entirely from stock engine meshes.
	// All parts share an attachment to the mannequin's right hand. Designers
	// can edit PrototypeRifleRoot's transform in BP_ThirdPersonCharacter.
	PrototypeRifleRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PrototypeRifleRoot"));
	PrototypeRifleRoot->SetupAttachment(GetMesh(), TEXT("hand_r"));
	PrototypeRifleRoot->SetRelativeLocation(FVector(4.0f, -8.0f, -3.0f));
	PrototypeRifleRoot->SetRelativeRotation(FRotator(0.0f, 0.0f, -90.0f));

	PrototypeRifleReceiver = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeRifleReceiver"));
	PrototypeRifleReceiver->SetupAttachment(PrototypeRifleRoot);
	PrototypeRifleReceiver->SetRelativeLocation(FVector(10.0f, 0.0f, 0.0f));
	PrototypeRifleReceiver->SetRelativeScale3D(FVector(0.5f, 0.08f, 0.09f));

	PrototypeRifleBarrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeRifleBarrel"));
	PrototypeRifleBarrel->SetupAttachment(PrototypeRifleRoot);
	PrototypeRifleBarrel->SetRelativeLocation(FVector(48.0f, 0.0f, 0.0f));
	PrototypeRifleBarrel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	PrototypeRifleBarrel->SetRelativeScale3D(FVector(0.03f, 0.03f, 0.43f));

	PrototypeRifleStock = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeRifleStock"));
	PrototypeRifleStock->SetupAttachment(PrototypeRifleRoot);
	PrototypeRifleStock->SetRelativeLocation(FVector(-24.0f, 0.0f, -2.0f));
	PrototypeRifleStock->SetRelativeScale3D(FVector(0.22f, 0.065f, 0.085f));

	PrototypeRifleGrip = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeRifleGrip"));
	PrototypeRifleGrip->SetupAttachment(PrototypeRifleRoot);
	PrototypeRifleGrip->SetRelativeLocation(FVector(2.0f, 0.0f, -12.0f));
	PrototypeRifleGrip->SetRelativeScale3D(FVector(0.075f, 0.055f, 0.15f));

	PrototypeRifleSight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeRifleSight"));
	PrototypeRifleSight->SetupAttachment(PrototypeRifleRoot);
	PrototypeRifleSight->SetRelativeLocation(FVector(38.0f, 0.0f, 7.0f));
	PrototypeRifleSight->SetRelativeScale3D(FVector(0.05f, 0.025f, 0.06f));

	PrototypeRifleMuzzle = CreateDefaultSubobject<USceneComponent>(TEXT("PrototypeRifleMuzzle"));
	PrototypeRifleMuzzle->SetupAttachment(PrototypeRifleRoot);
	PrototypeRifleMuzzle->SetRelativeLocation(FVector(70.0f, 0.0f, 0.0f));

	// Left-hand support point travels with the gun, including pitch aiming.
	// In the animation graph, a Two Bone IK node can resolve hand_l toward
	// this point in hand_r bone space without one-frame world-space lag.
	PrototypeRifleSupportGrip = CreateDefaultSubobject<USceneComponent>(TEXT("PrototypeRifleSupportGrip"));
	PrototypeRifleSupportGrip->SetupAttachment(PrototypeRifleRoot);
	PrototypeRifleSupportGrip->SetRelativeLocation(FVector(29.0f, 0.0f, -8.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeShape(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderShape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	for (UStaticMeshComponent* Part : {
		PrototypeRifleReceiver.Get(), PrototypeRifleStock.Get(),
		PrototypeRifleGrip.Get(), PrototypeRifleSight.Get() })
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (CubeShape.Succeeded())
		{
			Part->SetStaticMesh(CubeShape.Object);
		}
	}
	PrototypeRifleBarrel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (CylinderShape.Succeeded())
	{
		PrototypeRifleBarrel->SetStaticMesh(CylinderShape.Object);
	}

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

FVector AColonialMercenariesCharacter::GetPrototypeRifleSupportGripInRightHandSpace() const
{
	if (!IsValid(PrototypeRifleRoot) || !IsValid(PrototypeRifleSupportGrip))
	{
		return FVector::ZeroVector;
	}

	// RifleRoot attaches directly to the skeletal hand_r socket. Its local
	// transform maps rifle coordinates into hand_r bone coordinates.
	return PrototypeRifleRoot->GetRelativeTransform().TransformPosition(
		PrototypeRifleSupportGrip->GetRelativeLocation());
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

	// Camera presentation is local. Remote proxies only need the replicated
	// aim state for body rotation; they do not need their cameras updated.
	if (!IsLocallyControlled())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(AimCameraBlendTimerHandle);

	const float TargetArm = bIsAiming
		? FMath::Max(AimedCameraArmLength, 100.0f) : NormalCameraArmLength;
	const FVector TargetOffset = bIsAiming
		? AimedCameraSocketOffset : NormalCameraSocketOffset;

	if (!GetWorld() || AimCameraBlendDuration <= KINDA_SMALL_NUMBER)
	{
		CameraBoom->TargetArmLength = TargetArm;
		CameraBoom->SocketOffset = TargetOffset;
		return;
	}

	AimCameraBlendStartArm = CameraBoom->TargetArmLength;
	AimCameraBlendStartOffset = CameraBoom->SocketOffset;
	AimCameraBlendStartTime = GetWorld()->GetTimeSeconds();

	if (FMath::IsNearlyEqual(AimCameraBlendStartArm, TargetArm, 0.01f)
		&& AimCameraBlendStartOffset.Equals(TargetOffset, 0.01f))
	{
		return;
	}

	// Only runs during the short transition; no permanent camera Tick.
	GetWorldTimerManager().SetTimer(AimCameraBlendTimerHandle, this,
		&AColonialMercenariesCharacter::UpdateAimCameraBlend, 1.0f / 60.0f, true);
}

void AColonialMercenariesCharacter::UpdateAimCameraBlend()
{
	if (!GetWorld() || !IsValid(CameraBoom))
	{
		GetWorldTimerManager().ClearTimer(AimCameraBlendTimerHandle);
		return;
	}

	const float Duration = FMath::Max(AimCameraBlendDuration, 0.001f);
	const float Elapsed = GetWorld()->GetTimeSeconds() - AimCameraBlendStartTime;
	const float Alpha = FMath::Clamp(Elapsed / Duration, 0.0f, 1.0f);
	const float Eased = Alpha * Alpha * (3.0f - 2.0f * Alpha);

	const float TargetArm = bIsAiming
		? FMath::Max(AimedCameraArmLength, 100.0f) : NormalCameraArmLength;
	const FVector TargetOffset = bIsAiming
		? AimedCameraSocketOffset : NormalCameraSocketOffset;

	CameraBoom->TargetArmLength =
		FMath::Lerp(AimCameraBlendStartArm, TargetArm, Eased);
	CameraBoom->SocketOffset =
		FMath::Lerp(AimCameraBlendStartOffset, TargetOffset, Eased);

	if (Alpha >= 1.0f)
	{
		GetWorldTimerManager().ClearTimer(AimCameraBlendTimerHandle);
	}
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "ColonialMercenariesCharacter.h"
#include "Combat/CMPrototypeWeaponComponent.h"
#include "Combat/CMHealthComponent.h"
#include "Combat/CMFactionComponent.h"
#include "World/CMPrototypeContractTerminal.h"
#include "World/CMPrototypeIntelCache.h"
#include "Engine/OverlapResult.h"
#include "UI/CMPrototypeVitalsWidget.h"
#include "UI/CMDownedMarkerWidget.h"
#include "Components/WidgetComponent.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "UI/CMPrototypeReticleWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
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
	HealthComponent = CreateDefaultSubobject<UCMHealthComponent>(TEXT("Health"));
	FactionComponent = CreateDefaultSubobject<UCMFactionComponent>(TEXT("Faction"));

	DownedMarker = CreateDefaultSubobject<UWidgetComponent>(TEXT("DownedMarker"));
	DownedMarker->SetupAttachment(GetCapsuleComponent());
	DownedMarker->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	DownedMarker->SetWidgetSpace(EWidgetSpace::Screen);
	DownedMarker->SetWidgetClass(UCMDownedMarkerWidget::StaticClass());
	DownedMarker->SetDrawAtDesiredSize(true);
	DownedMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DownedMarker->SetVisibility(false);
	VitalsWidgetClass = UCMPrototypeVitalsWidget::StaticClass();
	SetCanBeDamaged(true);
	ReticleWidgetClass = UCMPrototypeReticleWidget::StaticClass();
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	// Player capsules must be struck by soldiers' Visibility hitscan.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		
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
	if (IsValid(HealthComponent))
	{
		HealthComponent->OnDied.AddDynamic(this, &AColonialMercenariesCharacter::HandlePlayerDowned);
		HealthComponent->OnRevived.AddDynamic(this, &AColonialMercenariesCharacter::HandlePlayerRevived);
	}
}

void AColonialMercenariesCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	EnsureLocalReticleWidget();
	EnsureLocalVitalsWidget();
}

void AColonialMercenariesCharacter::EnsureLocalReticleWidget()
{
	// Each owning player's UI is entirely local. No server HUD actor,
	// replicated widget state, or GameMode/Level Blueprint involvement.
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!IsValid(PlayerController) || !PlayerController->IsLocalController()
		|| !IsLocallyControlled() || !ReticleWidgetClass)
	{
		return;
	}

	if (!IsValid(ActiveReticleWidget))
	{
		ActiveReticleWidget = CreateWidget<UCMPrototypeReticleWidget>(
			PlayerController, ReticleWidgetClass);
	}
	if (IsValid(ActiveReticleWidget))
	{
		ActiveReticleWidget->SetWeaponComponent(PrototypeWeapon);
		if (!ActiveReticleWidget->IsInViewport())
		{
			ActiveReticleWidget->AddToPlayerScreen(10);
		}
	}
}

void AColonialMercenariesCharacter::EnsureLocalVitalsWidget()
{
	APlayerController* LocalPlayer = Cast<APlayerController>(GetController());
	if (!IsValid(LocalPlayer) || !LocalPlayer->IsLocalController()
		|| !IsLocallyControlled() || !IsValid(HealthComponent) || !VitalsWidgetClass)
	{
		return;
	}

	if (!IsValid(ActiveVitalsWidget))
	{
		ActiveVitalsWidget = CreateWidget<UCMPrototypeVitalsWidget>(LocalPlayer, VitalsWidgetClass);
	}
	if (IsValid(ActiveVitalsWidget))
	{
		ActiveVitalsWidget->SetTrackedHealth(HealthComponent);
		if (!ActiveVitalsWidget->IsInViewport())
		{
			ActiveVitalsWidget->AddToPlayerScreen(11);
		}
	}
}

void AColonialMercenariesCharacter::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(BleedOutTimerHandle);
	GetWorldTimerManager().ClearTimer(ReviveHoldTimerHandle);
	if (IsValid(ActiveReticleWidget))
	{
		ActiveReticleWidget->RemoveFromParent();
		ActiveReticleWidget = nullptr;
	}
	if (IsValid(ActiveVitalsWidget))
	{
		ActiveVitalsWidget->RemoveFromParent();
		ActiveVitalsWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AColonialMercenariesCharacter::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AColonialMercenariesCharacter, bIsAiming);
	DOREPLIFETIME_CONDITION(AColonialMercenariesCharacter, ActiveReviveTarget, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AColonialMercenariesCharacter, ReviveStartServerTime, COND_OwnerOnly);
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
		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started,
				this, &AColonialMercenariesCharacter::InteractWithNearbyContract);
		}
		if (FireAction)
		{
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started,
				this, &AColonialMercenariesCharacter::Fire);
		}
		if (ReviveAction)
		{
			EnhancedInputComponent->BindAction(ReviveAction, ETriggerEvent::Started,
				this, &AColonialMercenariesCharacter::StartReviveHold);
			EnhancedInputComponent->BindAction(ReviveAction, ETriggerEvent::Completed,
				this, &AColonialMercenariesCharacter::CancelReviveHold);
			EnhancedInputComponent->BindAction(ReviveAction, ETriggerEvent::Canceled,
				this, &AColonialMercenariesCharacter::CancelReviveHold);
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

	// Also covers local possession flows where input is initialized before
	// the client restart callback. Guarded against duplicate UI creation.
	EnsureLocalReticleWidget();
	EnsureLocalVitalsWidget();
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
	if (IsLocallyControlled() && !IsIncapacitated() && IsValid(PrototypeWeapon))
	{
		PrototypeWeapon->RequestFire();
	}
}

void AColonialMercenariesCharacter::InteractWithNearbyContract()
{
	if (!IsLocallyControlled() || IsIncapacitated() || !GetWorld())
	{
		return;
	}

	// Query only nearby WorldDynamic interactive prototypes. The existing
	// collision-enabled NPC and terrain actors are not enumerated globally.
	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectTypes;
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	GetWorld()->OverlapMultiByObjectType(
		Overlaps, GetActorLocation(), FQuat::Identity, ObjectTypes,
		FCollisionShape::MakeSphere(260.0f), Params);

	AActor* ClosestContractObject = nullptr;
	float BestDistSq = FMath::Square(300.0f);
	for (const FOverlapResult& Hit : Overlaps)
	{
		AActor* Candidate = Hit.GetActor();
		if (!IsValid(Candidate)
			|| (!Candidate->IsA<ACMPrototypeContractTerminal>()
				&& !Candidate->IsA<ACMPrototypeIntelCache>()))
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(
			GetActorLocation(), Candidate->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			ClosestContractObject = Candidate;
		}
	}

	if (IsValid(ClosestContractObject))
	{
		ServerTryContractInteract(ClosestContractObject);
	}
}

void AColonialMercenariesCharacter::ServerTryContractInteract_Implementation(
	AActor* TargetActor)
{
	// Server owns all decisions and does not trust the supplied actor,
	// position, line-of-sight, character health or contract progress.
	if (!HasAuthority() || !IsValid(TargetActor) || !GetWorld()
		|| TargetActor->GetWorld() != GetWorld() || IsIncapacitated()
		|| !IsValid(GetController()))
	{
		return;
	}

	const bool bTerminal = TargetActor->IsA<ACMPrototypeContractTerminal>();
	const bool bIntel = TargetActor->IsA<ACMPrototypeIntelCache>();
	if ((!bTerminal && !bIntel)
		|| FVector::DistSquared(
			GetActorLocation(), TargetActor->GetActorLocation())
			> FMath::Square(300.0f))
	{
		return;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CMContractInteraction), false);
	Params.AddIgnoredActor(this);
	FHitResult Hit;
	const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 75.0f);
	const FVector End = TargetActor->GetActorLocation()
		+ FVector(0.0f, 0.0f, bIntel ? 45.0f : 70.0f);
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
		Hit, Start, End, ECC_Visibility, Params);
	if (!bBlocked || Hit.GetActor() != TargetActor)
	{
		return;
	}

	if (ACMPrototypeContractTerminal* Terminal =
		Cast<ACMPrototypeContractTerminal>(TargetActor))
	{
		Terminal->TryInteract(this);
	}
	else if (ACMPrototypeIntelCache* Cache =
		Cast<ACMPrototypeIntelCache>(TargetActor))
	{
		Cache->TryInteract(this);
	}
}

bool AColonialMercenariesCharacter::IsIncapacitated() const
{
	return IsValid(HealthComponent) && HealthComponent->IsDead();
}

void AColonialMercenariesCharacter::HandlePlayerDowned()
{
	// Leave the pawn in place for a teammate's revive. Do not Destroy it
	// until the authoritative bleed-out timer elapses.
	if (IsValid(GetCharacterMovement()))
	{
		GetCharacterMovement()->DisableMovement();
	}
	bIsAiming = false;
	UpdateAimingPresentation();
	if (IsValid(DownedMarker))
	{
		DownedMarker->SetVisibility(true);
	}

	if (HasAuthority() && GetWorld())
	{
		CancelReviveHoldOnAuthority();
		GetWorldTimerManager().SetTimer(BleedOutTimerHandle, this,
			&AColonialMercenariesCharacter::FinishBleedOut,
			FMath::Max(BleedOutSeconds, 1.0f), false);
	}
}

void AColonialMercenariesCharacter::HandlePlayerRevived()
{
	GetWorldTimerManager().ClearTimer(BleedOutTimerHandle);
	if (IsValid(DownedMarker))
	{
		DownedMarker->SetVisibility(false);
	}
	if (IsValid(GetCharacterMovement()))
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void AColonialMercenariesCharacter::FinishBleedOut()
{
	if (!HasAuthority() || !IsIncapacitated() || !GetWorld())
	{
		return;
	}

	AController* PlayerController = GetController();
	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	if (IsValid(PlayerController) && IsValid(GameMode))
	{
		// PlayerStart is the temporary company-HQ spawn proxy until the
		// headquarters/medical expense/equipment recovery systems exist.
		PlayerController->UnPossess();
		GameMode->RestartPlayer(PlayerController);
		Destroy();
	}
}

bool AColonialMercenariesCharacter::CanReviveCandidate(
	const AColonialMercenariesCharacter* Candidate) const
{
	if (!IsValid(Candidate) || Candidate == this || !Candidate->IsIncapacitated()
		|| IsIncapacitated() || !GetWorld())
	{
		return false;
	}

	if (FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation())
		> FMath::Square(FMath::Max(ReviveRange, 50.0f)))
	{
		return false;
	}

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);
	FHitResult VisibilityHit;
	const bool bBlockingHit = GetWorld()->LineTraceSingleByChannel(
		VisibilityHit, GetPawnViewLocation(),
		Candidate->GetActorLocation() + FVector(0.0f, 0.0f, 45.0f),
		ECC_Visibility, TraceParams);
	return !bBlockingHit || VisibilityHit.GetActor() == Candidate;
}

AColonialMercenariesCharacter* AColonialMercenariesCharacter::FindReviveCandidate() const
{
	if (IsIncapacitated() || !GetWorld())
	{
		return nullptr;
	}

	AColonialMercenariesCharacter* Closest = nullptr;
	float ClosestDistanceSq = FMath::Square(FMath::Max(ReviveRange, 50.0f));

	// Client and server both have the replicated co-op pawn actors.
	// Called by a local HUD timer (10 Hz) or on an E press, never Actor Tick.
	for (TActorIterator<AColonialMercenariesCharacter> It(GetWorld()); It; ++It)
	{
		AColonialMercenariesCharacter* Candidate = *It;
		if (!CanReviveCandidate(Candidate))
		{
			continue;
		}
		const float DistanceSq = FVector::DistSquared(
			GetActorLocation(), Candidate->GetActorLocation());
		if (DistanceSq <= ClosestDistanceSq)
		{
			Closest = Candidate;
			ClosestDistanceSq = DistanceSq;
		}
	}
	return Closest;
}

float AColonialMercenariesCharacter::GetReviveProgress() const
{
	if (!IsValid(ActiveReviveTarget) || !GetWorld())
	{
		return 0.0f;
	}

	const AGameStateBase* GameState = GetWorld()->GetGameState();
	const float CurrentTime = GameState
		? GameState->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	return FMath::Clamp((CurrentTime - ReviveStartServerTime)
		/ FMath::Max(ReviveHoldSeconds, 0.2f), 0.0f, 1.0f);
}

void AColonialMercenariesCharacter::StartReviveHold()
{
	if (!IsLocallyControlled() || IsIncapacitated())
	{
		return;
	}
	if (HasAuthority())
	{
		StartReviveHoldOnAuthority();
	}
	else
	{
		ServerStartReviveHold();
	}
}

void AColonialMercenariesCharacter::CancelReviveHold()
{
	if (!IsLocallyControlled())
	{
		return;
	}
	if (HasAuthority())
	{
		CancelReviveHoldOnAuthority();
	}
	else
	{
		ServerCancelReviveHold();
	}
}

void AColonialMercenariesCharacter::ServerStartReviveHold_Implementation()
{
	StartReviveHoldOnAuthority();
}

void AColonialMercenariesCharacter::ServerCancelReviveHold_Implementation()
{
	CancelReviveHoldOnAuthority();
}

void AColonialMercenariesCharacter::StartReviveHoldOnAuthority()
{
	if (!HasAuthority() || IsIncapacitated() || !GetWorld()
		|| IsValid(ActiveReviveTarget))
	{
		return;
	}

	AColonialMercenariesCharacter* Candidate = FindReviveCandidate();
	if (!IsValid(Candidate))
	{
		return;
	}

	ActiveReviveTarget = Candidate;
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	ReviveStartServerTime = GameState
		? GameState->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	ForceNetUpdate();

	GetWorldTimerManager().SetTimer(ReviveHoldTimerHandle, this,
		&AColonialMercenariesCharacter::UpdateReviveHoldOnAuthority,
		0.05f, true);
}

void AColonialMercenariesCharacter::UpdateReviveHoldOnAuthority()
{
	if (!HasAuthority() || !CanReviveCandidate(ActiveReviveTarget))
	{
		CancelReviveHoldOnAuthority();
		return;
	}

	if (GetReviveProgress() < 1.0f)
	{
		return;
	}

	AColonialMercenariesCharacter* RevivedPlayer = ActiveReviveTarget;
	CancelReviveHoldOnAuthority();
	if (IsValid(RevivedPlayer) && IsValid(RevivedPlayer->HealthComponent))
	{
		RevivedPlayer->HealthComponent->RestoreHealth(
			FMath::Max(ReviveHealth, 1.0f));
	}
}

void AColonialMercenariesCharacter::CancelReviveHoldOnAuthority()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(ReviveHoldTimerHandle);
	if (IsValid(ActiveReviveTarget) || ReviveStartServerTime != 0.0f)
	{
		ActiveReviveTarget = nullptr;
		ReviveStartServerTime = 0.0f;
		ForceNetUpdate();
	}
}

void AColonialMercenariesCharacter::BeginAim()
{
	if (IsLocallyControlled() && !IsIncapacitated())
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
	// Ignore stale aim RPCs from a player who has already gone down.
	bIsAiming = bNewAiming && !IsIncapacitated();
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

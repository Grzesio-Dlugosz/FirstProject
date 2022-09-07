// Fill out your copyright notice in the Description page of Project Settings.


#include "Main.h"


#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Enemy.h"		///////////////////////////////////////////////////
#include "FirstSaveGame.h"

// Sets default values
AMain::AMain()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	GetCapsuleComponent()->SetCapsuleSize(27.f, 78.f);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	BaseTurnRate = 65.f;
	BaseLookUpRate = 65.f;

	// dont rotate when the controller rotates. Lets that just affect the camera
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	
	GetCharacterMovement()->bOrientRotationToMovement = true;	// character automatically facing direction of his/her movement
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.f, 0.0f);	//rate with which cha-er is rotating automatically
	GetCharacterMovement()->JumpZVelocity = 650.f;
	GetCharacterMovement()->AirControl = 0.2f;

	MaxHealth = 100.f;
	Health = 15.f;
	MaxStamina = 150.f;
	Stamina = 125.f;
	Coins = 0;

	SneakSpeed = 400.f;
	SprintingSpeed = 700.f;

	MovementStatus = EMovementStatus::EMS_Normal;
	StaminaStatus = EStaminaStatus::ESS_Normal;

	bShiftKeyDown = false;
	bLMBDown = false;

	StaminaDrainRate =  25.f;
	MinSprintStamina = 50.f;

	AttackBasicThrustRate = 1.5f;
	AttackComboRate = 2.f;

	InterpSpeed = 15.f;
	bInterpToEnemy = false;

	bHasCombatTarget = false;

	bIsMoving = false;
}

// Called when the game starts or when spawned
void AMain::BeginPlay()
{
	Super::BeginPlay();

	MainPlayerController = Cast<AMainPlayerController>(GetController());

	FString Map = GetWorld()->GetMapName();
	Map.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	// if(Map != "SunTemple")
	// {
		LoadGameNoSwitch();
		if(MainPlayerController) MainPlayerController->GameModeOnly();
	//}
	
	
}

// Called every frame
void AMain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MovementStatus == EMovementStatus::EMS_Dead) return;
	
	float DeltaStamina = StaminaDrainRate * DeltaTime;

	switch(StaminaStatus)
	{
		case EStaminaStatus::ESS_Normal:
			if (bShiftKeyDown)
			{
				if (bIsMoving)
				{
					Stamina -= DeltaStamina;
					if (Stamina <= MinSprintStamina)
					{
						SetStaminaStatus(EStaminaStatus::ESS_BelowMinimum);
					}
					SetMovementStatus(EMovementStatus::EMS_Sprinting);
				}
				else
				{
					SetMovementStatus(EMovementStatus::EMS_Normal);
				}
			}
			else	// shift key up
			{
				if (Stamina + DeltaStamina >= MaxStamina)
				{
					Stamina = MaxStamina;
				}
				else
				{
					Stamina += DeltaStamina;
				}
				SetMovementStatus(EMovementStatus::EMS_Normal);
			}
			break;
		case EStaminaStatus::ESS_BelowMinimum:
			if (bShiftKeyDown)
			{
				if (bIsMoving)
				{
					if (Stamina - DeltaStamina <= 0.f)
					{
						SetStaminaStatus(EStaminaStatus::ESS_Exhausted);
						Stamina = 0.f;
						SetMovementStatus(EMovementStatus::EMS_Normal);
					}
					else
					{
						Stamina -= DeltaStamina;
						SetMovementStatus(EMovementStatus::EMS_Sprinting);
					}
				}
				else
				{
					SetMovementStatus(EMovementStatus::EMS_Normal);
				}
				
			}
			else	// shift key up
			{
				Stamina += DeltaStamina;
				if (Stamina >= MinSprintStamina)
				{
					SetStaminaStatus(EStaminaStatus::ESS_Normal);
				}
				SetMovementStatus(EMovementStatus::EMS_Normal);
			}
			break;
		case EStaminaStatus::ESS_Exhausted:
			if (bShiftKeyDown)
			{
				Stamina = 0.f;
			}
			else	// shift key up
			{
				SetStaminaStatus(EStaminaStatus::ESS_ExhaustedRecovering);
				Stamina += DeltaTime;
			}
			SetMovementStatus(EMovementStatus::EMS_Normal);
			break;
		case EStaminaStatus::ESS_ExhaustedRecovering:
			if (bShiftKeyDown)
			{
				// no change to stamina
			}
			else	// shift key up
			{
				Stamina += DeltaStamina;
				if (Stamina >= MinSprintStamina)
				{
					SetStaminaStatus(EStaminaStatus::ESS_Normal);
				}
			}
			SetMovementStatus(EMovementStatus::EMS_Normal);
			break;
		default: ;	
	}

	if (bInterpToEnemy && CombatTarget)
	{
		FRotator LookAtYaw = GetLookAtRotationYaw(CombatTarget->GetActorLocation());
		FRotator InterpRotation = FMath::RInterpTo(GetActorRotation(), LookAtYaw, DeltaTime, InterpSpeed);
	
		SetActorRotation(InterpRotation);
	}

	if (CombatTarget)
	{
		CombatTargetLocation = CombatTarget->GetActorLocation();
		if(MainPlayerController)
		{
			MainPlayerController->EnemyLocation = CombatTargetLocation;
		}
	}
}

// Called to bind functionality to input
void AMain::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", EInputEvent::IE_Pressed, this, &AMain::Jump);
	PlayerInputComponent->BindAction("Jump", EInputEvent::IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Sprint", EInputEvent::IE_Pressed, this, &AMain::ShiftKeyDown);
	PlayerInputComponent->BindAction("Sprint", EInputEvent::IE_Released, this, &AMain::ShiftKeyUp);

	PlayerInputComponent->BindAction("LMB", EInputEvent::IE_Pressed, this, &AMain::LMBDown);
	PlayerInputComponent->BindAction("LMB", EInputEvent::IE_Released, this, &AMain::LMBUp);

	PlayerInputComponent->BindAction("ESC", EInputEvent::IE_Pressed, this, &AMain::ESCDown);
	PlayerInputComponent->BindAction("ESC", EInputEvent::IE_Released, this, &AMain::ESCUp);

	PlayerInputComponent->BindAxis("MoveForward", this, &AMain::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMain::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &AMain::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AMain::LookUp);
	PlayerInputComponent->BindAxis("TurnRate", this, &AMain::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMain::LookUpAtRate);
}

bool AMain::CanMove(float Value)
{
	if(MainPlayerController)
	{
		return (		(Value != 0.f) &&
							(!bAttacking) &&
								(MovementStatus != EMovementStatus::EMS_Dead) &&
									(MainPlayerController->bPauseMenuVisible == false)	);
	}
	return false;
}

void AMain::MoveForward(float Value)	// this functions from SetupPlayerInput are called repeatedly (in the tick function maybe?)
{
	bIsMoving = false;
	if ( CanMove(Value) )
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
		bIsMoving = true;
	}

}
void AMain::MoveRight(float Value)
{
	bIsMoving = false;
	if ( CanMove(Value) )
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
		bIsMoving = true;
	}
	
}

void AMain::LookUp(float Value)
{
	if ( CanMove(Value) )
	{
		AddControllerPitchInput(Value);
	}
}

void AMain::Turn(float Value)
{
	if ( CanMove(Value) )
	{
		AddControllerYawInput(Value);
	}
}

void AMain::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}
void AMain::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AMain::LMBDown()
{
	bLMBDown = true;

	if (MovementStatus == EMovementStatus::EMS_Dead) return;

	if(MainPlayerController) if (MainPlayerController->bPauseMenuVisible) return;

	if(ActiveOverlappingItem)
	{
		AWeapon* Weapon = Cast<AWeapon>(ActiveOverlappingItem);
		if (Weapon)
		{
			Weapon->Equip(this);
			SetActiveOverlappingItem(nullptr);
		}
	}else if (EquippedWeapon)
	{
		Attack();
	}
}

void AMain::LMBUp()
{
	bLMBDown = false;
}

void AMain::ESCDown()
{
	bESCDown = true;

	if(MainPlayerController)
	{
		MainPlayerController->TogglePauseMenu();
	}
}

void AMain::ESCUp()
{
	bESCDown = false;
}

void AMain::SetEquippedWeapon(AWeapon* WeaponToSet)
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
	}
	
	EquippedWeapon = WeaponToSet;
}

void AMain::Attack()
{
	if (!bAttacking && MovementStatus != EMovementStatus::EMS_Dead)
	{
		bAttacking = true;
		SetInterpToEnemy(true);
		
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && CombatMontage)
		{
			int32 Selection = FMath::RandRange(0,1);
			switch(Selection)
			{
			case 0:
				AnimInstance->Montage_Play(CombatMontage, AttackBasicThrustRate);
				AnimInstance->Montage_JumpToSection(FName("AttackBasicThrust"), CombatMontage);
				break;
			case 1:
				AnimInstance->Montage_Play(CombatMontage, AttackComboRate);
				AnimInstance->Montage_JumpToSection(FName("AttackCombo"), CombatMontage);
				break;
			default:;
			}
			
		}
	}
}

void AMain::AttackEnd()
{
	bAttacking = false;
	SetInterpToEnemy(false);
	if (bLMBDown)
	{
		Attack();
	}
}

// void AMain::DecrementHealth(float Amount)	// not used anymore ???
// {
// 	if (Health - Amount <= 0.f)
// 	{
// 		Health = 0;
// 		Die();
// 	}else Health -= Amount;
// 	
// }

float AMain::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	//return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (Health - DamageAmount <= 0.f)
	{
		Health = 0;
		Die();
		if (DamageCauser)
		{
			AEnemy* Enemy = Cast<AEnemy>(DamageCauser);
			if(Enemy)
			{
				Enemy->bHasValidTarget = false;
			}
		}
	}else Health -= DamageAmount;

	
	return DamageAmount;
}

void AMain::Die()
{
	if (MovementStatus == EMovementStatus::EMS_Dead) return;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && CombatMontage)
	{
		AnimInstance->Montage_Play(CombatMontage, 1.0f);
		AnimInstance->Montage_JumpToSection(FName("Death"));
	}
	SetMovementStatus(EMovementStatus::EMS_Dead);
}

void AMain::DeathEnd()
{
	GetMesh()->bPauseAnims = true;
	GetMesh()-> bNoSkeletonUpdate = true;
}

void AMain::UpdateCombatTarget()
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, EnemyFilter);
	bool bGotFirstEnemy = false;
	ClearCombatTarget();

	if(OverlappingActors.Num() == 0)
	{
		if(MainPlayerController)
		{
			MainPlayerController->RemoveEnemyHealthBar();
		}
		return;
	}

	AEnemy* ClosestEnemy = nullptr;	
	
		FVector MainLocation = GetActorLocation();
		float MinDistance = 0.f;
		
		for (auto Actor : OverlappingActors)
		{
			AEnemy* Enemy = Cast<AEnemy>(Actor);
			if(Enemy)
			{
				if(bGotFirstEnemy == false)
				{
					ClosestEnemy = Enemy;
					MinDistance = (ClosestEnemy->GetActorLocation() - MainLocation).Size();
					bGotFirstEnemy = true;
				}
				else
				{
					float Distance = (Enemy->GetActorLocation() - MainLocation).Size();
					if(Distance < MinDistance)
					{
						MinDistance = Distance;
						ClosestEnemy = Enemy;
					}
				}
			}
		}

	if(ClosestEnemy)
	{
		if(MainPlayerController) MainPlayerController->DisplayEnemyHealthBar();
		SetCombatTarget(ClosestEnemy);
		bHasCombatTarget = true;
	}
	else
	{
		if(MainPlayerController) MainPlayerController->RemoveEnemyHealthBar();
	}
}

void AMain::ClearCombatTarget()
{
	SetCombatTarget(nullptr);
	bHasCombatTarget = false;
}

void AMain::SwitchLevel(FName LevelName)
{
	UWorld* World = GetWorld();
	if(World)
	{
		FString CurrentLevel = World->GetMapName();

		FName CurrentLevelName(*CurrentLevel);
		if(CurrentLevelName != LevelName)
		{
			UGameplayStatics::OpenLevel(World, LevelName);
		}
	}
}

void AMain::SaveGame()
{
	UFirstSaveGame* SaveGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));

	if(SaveGameInstance)
	{
		SaveGameInstance->CharacterStats.Health = Health;
		SaveGameInstance->CharacterStats.MaxHealth = MaxHealth;
		SaveGameInstance->CharacterStats.Stamina = Stamina;
		SaveGameInstance->CharacterStats.MaxStamina = MaxStamina;
		SaveGameInstance->CharacterStats.Coins = Coins;
		SaveGameInstance->CharacterStats.Location = GetActorLocation();
		SaveGameInstance->CharacterStats.Rotation = GetActorRotation();
		if(EquippedWeapon)
		{
			SaveGameInstance->CharacterStats.WeaponName = EquippedWeapon->Name;
		}
		FString MapName = GetWorld()->GetMapName();
		MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
		SaveGameInstance->CharacterStats.LevelName = MapName;

		UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveGameInstance->PlayerName, SaveGameInstance->UserIndex);
	}
	
}

void AMain::LoadGame(bool SetPosition)
{
	UFirstSaveGame* LoadGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));

	if(LoadGameInstance)
	{
		LoadGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::LoadGameFromSlot(LoadGameInstance->PlayerName, LoadGameInstance->UserIndex));

		if(LoadGameInstance)
		{
			Health = LoadGameInstance->CharacterStats.Health;
			MaxHealth = LoadGameInstance->CharacterStats.MaxHealth;
			Stamina = LoadGameInstance->CharacterStats.Stamina;
			MaxStamina = LoadGameInstance->CharacterStats.MaxStamina;
			Coins = LoadGameInstance->CharacterStats.Coins;

			if(WeaponStorage)
			{
				AItemStorage* Weapons = GetWorld()->SpawnActor<AItemStorage>(WeaponStorage);
				if(Weapons)
				{
					FString WeaponName = LoadGameInstance->CharacterStats.WeaponName;
					
					if(Weapons->WeaponMap.Contains(WeaponName))
					{
						AWeapon* WeaponToEquip = GetWorld()->SpawnActor<AWeapon>(Weapons->WeaponMap[WeaponName]);
						WeaponToEquip->Equip(this);
					}
				}
			}
			

			if(SetPosition)
			{
				SetActorLocation(LoadGameInstance->CharacterStats.Location);
				SetActorRotation(LoadGameInstance->CharacterStats.Rotation);
			}

			SetMovementStatus(EMovementStatus::EMS_Normal);
			GetMesh()->bPauseAnims = false;
			GetMesh()-> bNoSkeletonUpdate = false;

			if(LoadGameInstance->CharacterStats.LevelName != TEXT(""))
			{
				FName LevelName(*LoadGameInstance->CharacterStats.LevelName);
				SwitchLevel(LevelName);
			}
			
		}else UE_LOG(LogTemp, Warning, TEXT("There isn't any game to load!"));
	}else UE_LOG(LogTemp, Warning, TEXT("There isn't any game to load!"));
}

void AMain::LoadGameNoSwitch()
{
	UFirstSaveGame* LoadGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));

	if(LoadGameInstance)
	{
		LoadGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::LoadGameFromSlot(LoadGameInstance->PlayerName, LoadGameInstance->UserIndex));

		if(LoadGameInstance)
		{
			Health = LoadGameInstance->CharacterStats.Health;
			MaxHealth = LoadGameInstance->CharacterStats.MaxHealth;
			Stamina = LoadGameInstance->CharacterStats.Stamina;
			MaxStamina = LoadGameInstance->CharacterStats.MaxStamina;
			Coins = LoadGameInstance->CharacterStats.Coins;

			if(WeaponStorage)
			{
				AItemStorage* Weapons = GetWorld()->SpawnActor<AItemStorage>(WeaponStorage);
				if(Weapons)
				{
					FString WeaponName = LoadGameInstance->CharacterStats.WeaponName;
					
					if(Weapons->WeaponMap.Contains(WeaponName))
					{
						AWeapon* WeaponToEquip = GetWorld()->SpawnActor<AWeapon>(Weapons->WeaponMap[WeaponName]);
						WeaponToEquip->Equip(this);
					}
				}
			}
			
			

			SetMovementStatus(EMovementStatus::EMS_Normal);
			GetMesh()->bPauseAnims = false;
			GetMesh()-> bNoSkeletonUpdate = false;
			
		}else UE_LOG(LogTemp, Warning, TEXT("There isn't any game to load!"));
	}else UE_LOG(LogTemp, Warning, TEXT("There isn't any game to load!"));
}

void AMain::IncrementCoins(int32 Amount)
{
	Coins += Amount;
}

void AMain::IncrementHealth(float Amount)
{
	if ( Health + Amount >= MaxHealth)
	{
		Health = MaxHealth;
	}
	else
	{
		Health += Amount;
	}
}

void AMain::Jump()
{
	if(MainPlayerController) if (MainPlayerController->bPauseMenuVisible) return;
	
	if (MovementStatus != EMovementStatus::EMS_Dead)
	{
		Super::Jump();
	}
	
}

void AMain::SetMovementStatus(EMovementStatus Status)
{
	MovementStatus = Status;
	if (MovementStatus == EMovementStatus::EMS_Sprinting)
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintingSpeed;
	}
	if (MovementStatus == EMovementStatus::EMS_Normal)
	{
		GetCharacterMovement()->MaxWalkSpeed = SneakSpeed;
	}
}

void AMain::SetInterpToEnemy(bool Interp)
{
	bInterpToEnemy = Interp;
}

FRotator AMain::GetLookAtRotationYaw(FVector Target)
{
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Target);
	FRotator LookAtRotationYaw = FRotator(0.f, LookAtRotation.Yaw, 0.f);
	return LookAtRotationYaw;
}

void AMain::ShiftKeyDown()
{
	bShiftKeyDown = true;
}
void AMain::ShiftKeyUp()
{
	bShiftKeyDown = false;
}

void AMain::ShowPickupLocation()
{
	// for (int32 i=0 ; i < PickupLocations.Num() ; i++)
	// {
	// 	UKismetSystemLibrary::DrawDebugSphere(this, PickupLocations[i], 30.f, 8, FLinearColor::Gray, 10.f, 0.5f);
	// }

	for (FVector Location : PickupLocations)
	{
		UKismetSystemLibrary::DrawDebugSphere(this, Location, 30.f, 8, FLinearColor::Gray, 10.f, 0.5f);
	}
	
}
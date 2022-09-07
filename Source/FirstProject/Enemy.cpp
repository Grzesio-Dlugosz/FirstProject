// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/CapsuleComponent.h"


// Sets default values
AEnemy::AEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AgroSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AgroSphere"));
	AgroSphere->SetupAttachment(GetRootComponent());
	AgroSphere->InitSphereRadius(600.f);
	AgroSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	CombatSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CombatSphere"));
	CombatSphere->SetupAttachment(GetRootComponent());
	CombatSphere->InitSphereRadius(75.f);

	CombatSocketCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("CombatCollision"));
	CombatSocketCollision->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("EnemySocket"));

	bOverlappingCombatSphere = false;
	bAttacking = false;

	Health = 75.f;
	MaxHealth = 100.f;
	Damage =10.f;

	AttackMinTime = 1.5f;
	AttackMaxTime = 5.f;

	EnemyMovementStatus = EEnemyMovementStatus::EMS_Idle;

	DeathDelay = 3.f;

	//bHasCombatTarget = false;	// same as in the course, not there

	bHasValidTarget = false;
}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	AIController = Cast<AAIController>(GetController());

	AgroSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::AgroSphereOnOverlapBegin);
	AgroSphere->OnComponentEndOverlap.AddDynamic(this, &AEnemy::AgroSphereOnOverlapEnd);
	CombatSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::CombatSphereOnOverlapBegin);
	CombatSphere->OnComponentEndOverlap.AddDynamic(this, &AEnemy::CombatSphereOnOverlapEnd);

	CombatSocketCollision->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::CombatSocketOnOverlapBegin);
	CombatSocketCollision->OnComponentEndOverlap.AddDynamic(this, &AEnemy::CombatSocketOnOverlapEnd);

	CombatSocketCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CombatSocketCollision->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CombatSocketCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CombatSocketCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AEnemy::AgroSphereOnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && Alive())
	{
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main)
		{
			MoveToTarget(Main);
			//bHasValidTarget = true;
		}
	}
}

void AEnemy::AgroSphereOnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && Alive())
	{
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main)
		{
			bHasValidTarget = false;
			Main->SetHasCombatTarget(false);
			if (Main->CombatTarget == this)
			{
				Main->SetCombatTarget(nullptr);		
			}
			Main->UpdateCombatTarget();
					
			SetEnemyMovementStatus(EEnemyMovementStatus::EMS_Idle);
			if (AIController) AIController->StopMovement();
			//bHasValidTarget = false;
		}
	}
}

void AEnemy::CombatSphereOnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && Alive())
	{
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main)
		{
			bHasValidTarget = true;
			Main->UpdateCombatTarget();
			
			CombatTarget = Main;
			bOverlappingCombatSphere = true;
			Attack();
			// float AttackTime = FMath::FRandRange(AttackMinTime, AttackMaxTime);
			// GetWorldTimerManager().SetTimer(AttackTimer, this, &AEnemy::Attack, AttackTime);
		}
	}
}

void AEnemy::CombatSphereOnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && Alive() && OtherComp)
	{
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main)
		{
			bOverlappingCombatSphere = false;
			MoveToTarget(Main);
			CombatTarget = nullptr;

			if(Main->CombatTarget == this)
			{
				Main->UpdateCombatTarget();
			}
			if(Main->MainPlayerController)
			{
				USkeletalMeshComponent* MainMesh = Cast<USkeletalMeshComponent>(OtherComp);
				if(MainMesh) Main->MainPlayerController->RemoveEnemyHealthBar();
			}
			
			GetWorldTimerManager().ClearTimer(AttackTimer);
		}
	}
}

void AEnemy::MoveToTarget(AMain* Target)
{
	if (Alive())
	{
		SetEnemyMovementStatus(EEnemyMovementStatus::EMS_MoveToTarget);

		if (AIController)
		{
			FAIMoveRequest MoveRequest;
			MoveRequest.SetGoalActor(Target);
			MoveRequest.SetAcceptanceRadius(30.0f);

			FNavPathSharedPtr NavPath;

			AIController->MoveTo(MoveRequest, &NavPath);

			TArray<FNavPathPoint> PathPoints = NavPath->GetPathPoints();
			for (auto Point : PathPoints)
			{
				FVector Location = Point.Location;
				UKismetSystemLibrary::DrawDebugSphere(this, Location, 50.f, 8, FLinearColor::Yellow, 10.f, 2.f);
			}
		}
	}
}

void AEnemy::CombatSocketOnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && Alive())
	{
		AMain* Main = Cast<AMain>(OtherActor);
		{
			if (Main)
			{
				if (Main->MainHitParticles)
				{
					const USkeletalMeshSocket* TipSocket = GetMesh()->GetSocketByName("TipSocket");
					if (TipSocket)
					{
						FVector SocketLocation = TipSocket->GetSocketLocation(GetMesh());
						UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Main->MainHitParticles, SocketLocation, FRotator(0.f), false );
					}
					
				}
				if (Main->MainHitSound) UGameplayStatics::PlaySound2D(this, Main->MainHitSound);
				if (DamageTypeClass)
				{
					UGameplayStatics::ApplyDamage(Main, Damage, AIController, this, DamageTypeClass);
				}
			}
			
		}
	}
}

void AEnemy::CombatSocketOnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	//bOverlappingCombatSphere = false;
	
}

void AEnemy::EnemyActivateCollision()
{
	CombatSocketCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AEnemy::EnemyDeactivateCollision()
{
	CombatSocketCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AEnemy::Attack()
{
	if (Alive() && bHasValidTarget)
	{
		if (AIController)
		{
			AIController->StopMovement();
			SetEnemyMovementStatus(EEnemyMovementStatus::EMS_Attacking);
		}
		if(!bAttacking)
		{
			bAttacking = true;
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			if (AnimInstance)
			{
				AnimInstance->Montage_Play(EnemyCombatMontage, 1.35f);
				AnimInstance->Montage_JumpToSection(FName("Attack"), EnemyCombatMontage);
				
			}
		}
	}
	if (Alive() && ! bHasValidTarget) SetEnemyMovementStatus(EEnemyMovementStatus::EMS_Idle);
}

void AEnemy::AttackEnd()
{
	bAttacking = false;
	if (bOverlappingCombatSphere && Alive())
	{
		float AttackTime = FMath::FRandRange(AttackMinTime, AttackMaxTime);
		GetWorldTimerManager().SetTimer(AttackTimer, this, &AEnemy::Attack, AttackTime);
		
	}
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
	AActor* DamageCauser)
{
	//return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (Health - DamageAmount <= 0.f)
	{
		Health -= DamageAmount;
		Die(DamageCauser);
	}else
	{
		Health -= DamageAmount;
	}

	return DamageAmount;
}

void AEnemy::Die(AActor* Causer)
{
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->Montage_Play(EnemyCombatMontage, 1.35f);
		AnimInstance->Montage_JumpToSection(FName("Death"), EnemyCombatMontage);
	}
	SetEnemyMovementStatus(EEnemyMovementStatus::EMS_Dead);

	CombatSocketCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AgroSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CombatSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	bAttacking = false;

	AMain* Main = Cast<AMain>(Causer);
	if(Main) Main->UpdateCombatTarget();
}

void AEnemy::DeathEnd()
{
	GetMesh()->bPauseAnims = true;
	GetMesh()->bNoSkeletonUpdate = true;

	GetWorldTimerManager().SetTimer(DeathTimer, this, &AEnemy::Disappear, DeathDelay);
}

bool AEnemy::Alive()
{
	return GetEnemyMovementStatus() != EEnemyMovementStatus::EMS_Dead;
}

void AEnemy::Disappear()
{
	Destroy();
}



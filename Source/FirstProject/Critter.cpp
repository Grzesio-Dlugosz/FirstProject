// Fill out your copyright notice in the Description page of Project Settings.


#include "Critter.h"

// Sets default values
ACritter::ACritter()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	MeshComponent->SetupAttachment(GetRootComponent());

	// Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	// Camera ->SetupAttachment(GetRootComponent());
	// Camera->SetRelativeLocation(FVector(-300.f, 0.f, 300.f));
	// Camera->SetRelativeRotation(FRotator(-45.f, 0.f, 0.f));

	CurrentVelocity = FVector(0.f);
	MaxSpeed = 100.f;
}

// Called when the game starts or when spawned
void ACritter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACritter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ACritter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}


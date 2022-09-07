// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "Main.h"
#include "Pickup.generated.h"

/**
 * 
 */
UCLASS()
class FIRSTPROJECT_API APickup : public AItem
{
	GENERATED_BODY()
public: 
	APickup();

	UFUNCTION(BlueprintImplementableEvent, Category = "Pickup")
	void OnPickupBP(AMain* Target);

	// // UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Type")
	// // int32 CoinCount;
	//
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Type")
	// float HealthAddition;
	//
	// UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pickup Type")
	// bool bIsItCoin;
	//
	// UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pickup Type")
	// bool bIsItHealth;

	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult) override;
	
	virtual void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) override;

	
	
};

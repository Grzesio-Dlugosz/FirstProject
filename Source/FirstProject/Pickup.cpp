// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup.h"
#include "Main.h"

APickup::APickup()
{
	//bIsItCoin = true;
	//bIsItHealth = false;

	//HealthAddition = 30.f;
}

void APickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	Super::OnOverlapBegin(OverlappedComponent, OtherActor,OtherComp,  OtherBodyIndex,  bFromSweep,SweepResult );

	if (OtherActor)
	{
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main)
		{
			//if ( bIsItCoin) Main->IncrementCoins();
			//if (bIsItHealth) Main->IncrementHealth(HealthAddition);
			OnPickupBP(Main);
			
			Main->PickupLocations.Add(GetActorLocation());

			Destroy();
		}
	}
}
	
void APickup::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnOverlapEnd(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);
}

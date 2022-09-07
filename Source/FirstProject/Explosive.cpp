// Fill out your copyright notice in the Description page of Project Settings.


#include "Explosive.h"

#include "Enemy.h"
#include "Main.h"
#include "Kismet/GameplayStatics.h"

AExplosive::AExplosive()
{
	Damage = 15.f;
}

void AExplosive::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	Super::OnOverlapBegin(OverlappedComponent, OtherActor,OtherComp,  OtherBodyIndex,  bFromSweep,SweepResult );
	//UE_LOG(LogTemp, Warning, TEXT("begin explosive"));

	if (OtherActor)
	{
		AMain* Main = Cast<AMain>(OtherActor);
		AEnemy* Enemy = Cast<AEnemy>(OtherActor);
		if (Main || Enemy)
		{
			//Main->DecrementHealth(Damage);	// old one
			if (DamageTypeClass)
			{
				AController* MainControllerTMP = Main->GetController();
				UGameplayStatics::ApplyDamage(OtherActor, Damage, nullptr, this, DamageTypeClass);

				Destroy();
			}
		}
	}
}


void AExplosive::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnOverlapEnd(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);
	//UE_LOG(LogTemp, Warning, TEXT("end explosive"));


}

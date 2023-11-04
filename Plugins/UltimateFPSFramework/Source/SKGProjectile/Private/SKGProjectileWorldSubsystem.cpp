// Copyright 2022, Dakota Dawe, All rights reserved


#include "SKGProjectileWorldSubsystem.h"

#include "Engine/WindDirectionalSource.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

USKGProjectileWorldSubsystem::USKGProjectileWorldSubsystem()
{
	
}

void USKGProjectileWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	FTimerHandle TTempHandle;
	GetWorld()->GetTimerManager().SetTimer(TTempHandle, this, &USKGProjectileWorldSubsystem::FindAndSetWindSource, 2.0f, false);
}

void USKGProjectileWorldSubsystem::FindAndSetWindSource()
{
	WindSource = Cast<AWindDirectionalSource>(UGameplayStatics::GetActorOfClass(GetWorld(), AWindDirectionalSource::StaticClass()));
}

// Copyright 2022, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DataTypes/SKGFPSDataTypes.h"
#include "SKGFirearmPartsInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class USKGFirearmPartsInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ULTIMATEFPSFRAMEWORK_API ISKGFirearmPartsInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SKGFPSFramework|FirearmPartsInterface")
	TArray<AActor*> GetCachedParts();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SKGFPSFramework|FirearmPartsInterface")
	FSKGFirearmCachedParts GetCachedComponents();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SKGFPSFramework|FirearmPartsInterface")
	float GetStockLengthOfPull();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SKGFPSFramework|FirearmPartsInterface")
	FVector GetAimStockOffset();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SKGFPSFramework|FirearmPartsInterface")
	FTransform GetMuzzleSocketTransform();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SKGFPSFramework|FirearmPartsInterface")
	ASKGSight* GetCurrentSight();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SKGFPSFramework|FirearmPartsInterface")
	AActor* GetCurrentSightActor();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SKGFPSFramework|FirearmPartsInterface")
	void ActivateCurrentSight(bool bActivate);
};

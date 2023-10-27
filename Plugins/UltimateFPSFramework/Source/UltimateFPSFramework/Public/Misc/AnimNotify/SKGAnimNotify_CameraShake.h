// Copyright 2022, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Components/SKGCharacterComponent.h"
#include "Runtime/Launch/Resources/Version.h"
#include "SKGAnimNotify_CameraShake.generated.h"

/**
 * 
 */
UCLASS()
class ULTIMATEFPSFRAMEWORK_API USKGAnimNotify_CameraShake : public UAnimNotify
{
	GENERATED_BODY()

public:
	USKGAnimNotify_CameraShake();

protected:
	UPROPERTY(EditAnywhere, Category = "AnimNotify")
	TSubclassOf<UCameraShakeBase> CameraShake;
	UPROPERTY(EditAnywhere, Category = "AnimNotify")
	float ShakeScale;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};

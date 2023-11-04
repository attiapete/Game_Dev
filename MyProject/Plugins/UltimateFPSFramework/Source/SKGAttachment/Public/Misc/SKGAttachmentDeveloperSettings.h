// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SKGAttachmentDeveloperSettings.generated.h"

/**
 * 
 */
UCLASS(Config=Game, DefaultConfig, meta = (DisplayName = "SKG Attachment Editor Settings"))
class SKGATTACHMENT_API USKGAttachmentDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	USKGAttachmentDeveloperSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual FName GetCategoryName() const override { return FName("Plugins"); }

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "ComponentTags")
	FName AttachmentMeshTag = "SKGAttachment";
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "ComponentTags")
	FName AttachmentOverlapTag = "SKGOverlap";
};

//Copyright 2022, Dakota Dawe, All rights reserved

#pragma once

#include "Templates/SubclassOf.h"
#include "Engine/EngineTypes.h"
#include "SKGLightLaserDataTypes.generated.h"

UENUM(BlueprintType)
enum class ESKGLightLaserState : uint8
{
	Off		UMETA(DisplayName = "Off"),
	Light	UMETA(DisplayName = "Light"),
	Laser	UMETA(DisplayName = "Laser"),
	Both	UMETA(DisplayName = "Both")
};

UENUM(BlueprintType)
enum class ESKGAllLightLaser : uint8
{
	Off		UMETA(DisplayName = "Off"),
	Light	UMETA(DisplayName = "Light"),
	Laser	UMETA(DisplayName = "Laser"),
	IRLight	UMETA(DisplayName = "IRLight"),
	IRLaser	UMETA(DisplayName = "IRLaser"),
	IRBoth	UMETA(DisplayName = "IRBoth"),
	Both	UMETA(DisplayName = "Both")
};

UENUM(BlueprintType)
enum class ESKGLightMode : uint8
{
	Off		UMETA(DisplayName = "Off"),
	Strobe	UMETA(DisplayName = "Strobe"),
	Steady	UMETA(DisplayName = "Steady")
};

class UMaterialInterface;

USTRUCT(BlueprintType)
struct FSKGLaserColor
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, NotReplicated, Category = "SKGFPSFramework")
	UMaterialInterface* LaserMaterial = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, NotReplicated, Category = "SKGFPSFramework")
	UMaterialInterface* LaserDotMaterial = nullptr;
};

USTRUCT(BlueprintType)
struct FSKGLaserSettings
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, NotReplicated, Category = "SKGFPSFramework")
	TArray<FSKGLaserColor> LaserColors;
	UPROPERTY(BlueprintReadWrite, Category = "SKGFPSFramework")
	uint8 LaserColorIndex = 0;

	uint8 GetNextIndex(bool bDownArray)
	{
		if (bDownArray)
		{
			if (++LaserColorIndex >= LaserColors.Num())
			{
				LaserColorIndex = 0;
			}
		}
		else
		{
			if (LaserColorIndex - 1 < 0)
			{
				LaserColorIndex = LaserColors.Num() - 1;
			}
			else
			{
				--LaserColorIndex;
			}
		}
		return LaserColorIndex;
	}

	FSKGLaserColor GetCurrentColor()
	{
		if (LaserColors.Num())
		{
			if (LaserColorIndex >= LaserColors.Num())
			{
				LaserColorIndex = 0;
			}
			return LaserColors[LaserColorIndex];
		}

		return FSKGLaserColor();
	}

	void SetColorIndex(uint8 Index)
	{
		if (Index >= LaserColors.Num())
		{
			LaserColorIndex = 0;
		}
		LaserColorIndex = Index;
	}
};

USTRUCT(BlueprintType)
struct FSKGLightSettings
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, NotReplicated, Category = "SKGFPSFramework")
	TArray<float> LightIntensityLevels = { 2.0f };
	UPROPERTY(BlueprintReadWrite, Category = "SKGFPSFramework")
	uint8 LightIntensityIndex = 0;

	uint8 GetNextIndex(bool bDownArray)
	{
		if (bDownArray)
		{
			if (++LightIntensityIndex >= LightIntensityLevels.Num())
			{
				LightIntensityIndex = 0;
			}
		}
		else
		{
			if (LightIntensityIndex - 1 < 0)
			{
				LightIntensityIndex = LightIntensityLevels.Num() - 1;
			}
			else
			{
				--LightIntensityIndex;
			}
		}
		return LightIntensityIndex;
	}

	float GetCurrentIntensity()
	{
		if (LightIntensityIndex >= LightIntensityLevels.Num())
		{
			LightIntensityIndex = 0;
		}
		return LightIntensityLevels[LightIntensityIndex];
	}

	void SetIntensityIndex(uint8 Index)
	{
		if (Index >= LightIntensityLevels.Num())
		{
			LightIntensityIndex = 0;
		}
		LightIntensityIndex = Index;
	}
};

USTRUCT(BlueprintType)
struct FSKGLightLaserSettings
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, NotReplicated, Category = "SKGFPSFramework")
	bool bSupportsInfrared = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFPSFramework")
	FSKGLightSettings LightSettings;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFPSFramework")
	FSKGLaserSettings LaserSettings;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFPSFramework", meta = (EditCondition = "bSupportsInfrared", EditConditionHides))
	FSKGLightSettings IRLightSettings;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFPSFramework", meta = (EditCondition = "bSupportsInfrared", EditConditionHides))
	FSKGLaserSettings IRLaserSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, NotReplicated, Category = "SKGFPSFramework")
	uint8 LaserMaterialIndex = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, NotReplicated, Category = "SKGFPSFramework")
	float MaxLaserDistance = 10000.0f;
	// What collision channel to use for the light. For things like glass you will want to set it to ignore
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, NotReplicated, Category = "SKGFPSFramework")
	TEnumAsByte<ECollisionChannel> LaserCollisionChannel = ECC_Visibility;
	// How much to scale the lazer dot by per distance
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, NotReplicated, Category = "SKGFPSFramework")
	float LaserDistanceScaleMultiplier = 45.0f;
};
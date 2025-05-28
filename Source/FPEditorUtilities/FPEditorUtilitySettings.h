// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FPEditorUtilitySettings.generated.h"

USTRUCT(BlueprintType)
struct FDataTableTags
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UDataTable> DataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (FilePathFilter = "ini"))
	FFilePath TagPath;
};

UCLASS(config = EditorPerProjectUserSettings, defaultconfig)
class FPEDITORUTILITIES_API UFPEditorUtilitySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere)
	TArray<FDataTableTags> TablesUsingGameplayTags;

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	FSimpleMulticastDelegate OnTablesChanged;

	static const UFPEditorUtilitySettings& Get()
	{
		return *GetDefault<UFPEditorUtilitySettings>();
	}

	static UFPEditorUtilitySettings& GetMutable()
	{
		return *GetMutableDefault<UFPEditorUtilitySettings>();
	}
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/Skeleton.h"
#include "Blutility/Classes/EditorUtilityObject.h"
#include "Engine/EngineTypes.h"
#include "FPAssetCreation.generated.h"

class UAnimBlueprint;
class UAnimInstance;
class FAssetToolsModule;
/**
 *
 */
UCLASS(Blueprintable)
class FPEDITORUTILITIES_API UFPAssetCreation : public UEditorUtilityObject
{
	GENERATED_BODY()

	UFPAssetCreation();

	UPROPERTY(EditAnywhere, meta = (ContentDir, LongPackageName))
	FDirectoryPath DefaultPath;

	UPROPERTY(EditAnywhere)
	FString CharacterName;

	UPROPERTY(EditAnywhere)
	USkeletalMesh* CharacterSkeleton;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> CharacterClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UAnimInstance> AnimInstanceClass;

	UPROPERTY(EditAnywhere)
	TArray<FString> MontagesToCreate;

	UFUNCTION(BlueprintCallable)
	void CreateCharacter();

	UFUNCTION(BlueprintCallable)
	void TryLoadObj();

private:
	FString DestinationFolder;

	UAnimBlueprint* CreatedAnimBlueprint;

	bool SelectDestinationFolder();

	void CreateMontage(FAssetToolsModule& AssetToolsModule, FString Suffix);
	void CreateCharacter(FAssetToolsModule& AssetToolsModule);
	void CreateAnimBP(FAssetToolsModule& AssetToolsModule);
	void CreateBlendspace(FAssetToolsModule& AssetToolsModule);

	void SaveAsset(UObject* CreatedAsset);

	void EditCharacterBlueprint(UObject* Object);
};
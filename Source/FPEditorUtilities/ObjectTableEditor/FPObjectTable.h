#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include <random>
#include "FPObjectTable.generated.h"

UCLASS(BlueprintType)
class FPEDITORUTILITIES_API UFPObjectTable : public UObject
{
	GENERATED_BODY()

public:
	UFPObjectTable();

	UPROPERTY(EditAnywhere, meta=(AllowAbstract))
	TObjectPtr<UClass> ClassFilter;

	UPROPERTY(EditAnywhere, meta = (RelativeToGameContentDir, LongPackageName))
	FDirectoryPath RootDirectory;

	UPROPERTY(EditAnywhere)
	FString NameTemplate;

	UPROPERTY(EditAnywhere)
	bool bCheckSubfolders;

	UPROPERTY()
	TArray<FString> CheckedProperties;

	UPROPERTY()
	TArray<FString> DetailsViewSections;

	UPROPERTY()
	TArray<FString> CheckedCategories;

	void SetClassFilter(UClass* InClass)
	{
		Modify();
		ClassFilter = InClass;
	}

	void SetRootDirectory(FDirectoryPath Directory)
	{
		Modify();
		RootDirectory = Directory;
	}

	UFUNCTION(CallInEditor)
	void Refresh() { OnChange.Broadcast(); }

	FSimpleMulticastDelegate OnChange;
	FSimpleMulticastDelegate OnClassChanged;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

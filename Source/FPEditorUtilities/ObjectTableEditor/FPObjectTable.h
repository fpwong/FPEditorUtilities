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

	UPROPERTY(EditAnywhere)
	UClass* ClassFilter;

	UPROPERTY(EditAnywhere, meta = (RelativeToGameContentDir, LongPackageName))
	FDirectoryPath RootDirectory;

	UPROPERTY(EditAnywhere)
	FString Suffix;

	UPROPERTY(EditAnywhere)
	bool bCheckSubfolders;

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

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

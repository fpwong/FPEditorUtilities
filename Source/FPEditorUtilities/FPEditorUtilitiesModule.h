#pragma once

#include "CoreMinimal.h"

class FFPObjectTableAssetTypeActions;

class FFPEditorUtilitiesModule final : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void OnPostEngineInit();

	void CheckTagFiles();

	void ReadTableFiles(UDataTable* Table);
	void BindTables();

private:
	TSharedPtr<FFPObjectTableAssetTypeActions> ObjectTableActions;

	FTimerHandle UpdateTimer;

	TArray<TWeakObjectPtr<UDataTable>> BoundTables;
};

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

private:
	TSharedPtr<FFPObjectTableAssetTypeActions> ObjectTableActions;
};

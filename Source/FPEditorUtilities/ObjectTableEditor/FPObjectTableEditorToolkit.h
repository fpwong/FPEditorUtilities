#pragma once

#include "CoreMinimal.h"
#include "FPObjectTable.h"
#include "Toolkits/AssetEditorToolkit.h"

class FFPObjectTableEditorToolkit final : public FAssetEditorToolkit
{
public:
	void InitEditor(const TArray<UObject*>& InObjects);

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;

	virtual FName GetToolkitFName() const override { return "FPObjectTableEditor"; }
	virtual FText GetBaseToolkitName() const override { return INVTEXT("FP Object Table Editor"); }
	virtual FString GetWorldCentricTabPrefix() const override { return "FP Object Table "; }
	virtual FLinearColor GetWorldCentricTabColorScale() const override { return {}; }

private:
	UFPObjectTable* ObjectTable = nullptr;
};

#include "FPObjectTableActions.h"
#include "FPObjectTable.h"
#include "FPObjectTableEditorToolkit.h"

UClass* FFPObjectTableAssetTypeActions::GetSupportedClass() const
{
	return UFPObjectTable::StaticClass();
}

FText FFPObjectTableAssetTypeActions::GetName() const
{
	return INVTEXT("FP Object Table");
}

FColor FFPObjectTableAssetTypeActions::GetTypeColor() const
{
	return FColor::Cyan;
}

uint32 FFPObjectTableAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

void FFPObjectTableAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	MakeShared<FFPObjectTableEditorToolkit>()->InitEditor(InObjects);
}

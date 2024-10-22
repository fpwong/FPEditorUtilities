#include "FPObjectTableEditorToolkit.h"

#include "FPObjectTableEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Modules/ModuleManager.h"

#define MAIN_TAB_NAME "FPObjectTableTab"
#define DETAILS_TAB_NAME "FPObjectTableDetailsTab"

void FFPObjectTableEditorToolkit::InitEditor(const TArray<UObject*>& InObjects)
{
	ObjectTable = Cast<UFPObjectTable>(InObjects[0]);

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("FPObjectEditorLayout")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewSplitter()
				->SetSizeCoefficient(0.6f)
				->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.8f)
					->AddTab(MAIN_TAB_NAME, ETabState::OpenedTab)
				)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.4f)
				->AddTab(DETAILS_TAB_NAME, ETabState::OpenedTab)
			)
		);
	FAssetEditorToolkit::InitAssetEditor(EToolkitMode::Standalone, {}, "FPObjectTableEditor", Layout, true, true, InObjects);
}

void FFPObjectTableEditorToolkit::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(INVTEXT("Object Table Editor"));

	InTabManager->RegisterTabSpawner(MAIN_TAB_NAME, FOnSpawnTab::CreateLambda([this](const FSpawnTabArgs&) {
			return SNew(SDockTab)
				[
					SNew(SFPObjectTableEditor).Settings(ObjectTable)
				];
		}))
		.SetDisplayName(INVTEXT("ObjectEditor"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObjects(TArray<UObject*>{ObjectTable});
	InTabManager->RegisterTabSpawner(DETAILS_TAB_NAME, FOnSpawnTab::CreateLambda([=](const FSpawnTabArgs&) {
			return SNew(SDockTab)
				[
					DetailsView
				];
		}))
		.SetDisplayName(INVTEXT("Details"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
}

void FFPObjectTableEditorToolkit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(MAIN_TAB_NAME);
	InTabManager->UnregisterTabSpawner(DETAILS_TAB_NAME);
}

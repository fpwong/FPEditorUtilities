#include "FPEditorUtilities.h"

#include "GameplayTagsManager.h"
#include "LoadDataURL/FPLoadDataURL_CurveTable.h"
#include "LoadDataURL/FPLoadDataURL_DataTable.h"

#define LOCTEXT_NAMESPACE "FFPEditorUtilitiesModule"

void FFPEditorUtilitiesModule::StartupModule()
{
#if WITH_EDITOR
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FFPEditorUtilitiesModule::OnPostEngineInit);
#endif
}

void FFPEditorUtilitiesModule::ShutdownModule()
{
}

void FFPEditorUtilitiesModule::OnPostEngineInit()
{
	FFPLoadDataURL_CurveTable::Get().Init();
	FFPLoadDataURL_DataTable::Get().Init();

	UToolMenu* HelpMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& Section = HelpMenu->AddSection("Reload Gameplay Tags", INVTEXT("ReloadGameplayTags"));

	FToolMenuOwnerScoped OwnerScoped(this);
	{
		FToolMenuEntry& MenuEntry = Section.AddEntry(FToolMenuEntry::InitMenuEntry(
			"ReloadGameplayTags",
			INVTEXT("Reload Gameplay Tags"),
			INVTEXT("ReloadGameplayTags"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]()
			{
				// UE_LOG(LogTemp, Warning, TEXT("Reloading tags"));

				TArray<FString> OutPaths;
				UGameplayTagsManager::Get().GetTagSourceSearchPaths(OutPaths);

				TArray<const FGameplayTagSource*> AllSources;
				UGameplayTagsManager::Get().FindTagSourcesWithType(EGameplayTagSourceType::DefaultTagList, AllSources);
				UGameplayTagsManager::Get().FindTagSourcesWithType(EGameplayTagSourceType::RestrictedTagList, AllSources);
				UGameplayTagsManager::Get().FindTagSourcesWithType(EGameplayTagSourceType::TagList, AllSources);

				for (const FGameplayTagSource* TagSource : AllSources) 
				{
					GConfig->LoadFile(TagSource->GetConfigFileName());
				}

				UGameplayTagsManager::Get().EditorRefreshGameplayTagTree();
			}))
		));
		MenuEntry.InsertPosition = FToolMenuInsert(NAME_None, EToolMenuInsertType::First);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFPEditorUtilitiesModule, FPEditorUtilities)

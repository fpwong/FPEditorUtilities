#include "FPEditorUtilitiesModule.h"

#include "AssetToolsModule.h"
#include "DataTableEditorUtils.h"
#include "FPEditorUtilitySettings.h"
#include "GameplayTagsEditorModule.h"
#include "GameplayTagsManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "LoadDataURL/FPLoadDataURL_CurveTable.h"
#include "LoadDataURL/FPLoadDataURL_DataTable.h"
#include "ObjectTableEditor/FPObjectTableActions.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "FFPEditorUtilitiesModule"

void FFPEditorUtilitiesModule::StartupModule()
{
#if WITH_EDITOR
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FFPEditorUtilitiesModule::OnPostEngineInit);

	ObjectTableActions = MakeShared<FFPObjectTableAssetTypeActions>();
	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(ObjectTableActions.ToSharedRef());
#endif
}

void FFPEditorUtilitiesModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools") && ObjectTableActions.IsValid())
	{
		FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(ObjectTableActions.ToSharedRef());
	}
}

void FFPEditorUtilitiesModule::OnPostEngineInit()
{
	FFPLoadDataURL_CurveTable::Get().Init();
	FFPLoadDataURL_DataTable::Get().Init();

	UToolMenu* HelpMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& Section = HelpMenu->AddSection("Reload Gameplay Tags", INVTEXT("ReloadGameplayTags"));
	RegisterGameCategory();

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
					
					FText Msg = FText::FromString(FString::Printf(TEXT("Reloaded %s"), *TagSource->GetConfigFileName()));
					FNotificationInfo Notification(Msg);
					Notification.ExpireDuration = 5.0f;
					FSlateNotificationManager::Get().AddNotification(Notification);
				}

				UGameplayTagsManager::Get().EditorRefreshGameplayTagTree();
			}))
		));
		MenuEntry.InsertPosition = FToolMenuInsert(NAME_None, EToolMenuInsertType::First);
	}

	// check tag files changed on a timer
	GEditor->GetTimerManager()->SetTimer(UpdateTimer, FTimerDelegate::CreateRaw(this, &FFPEditorUtilitiesModule::CheckTagFiles), 2.0f, true);

	BindTables();
	UFPEditorUtilitySettings::GetMutable().OnTablesChanged.AddRaw(this, &FFPEditorUtilitiesModule::BindTables);
}

void FFPEditorUtilitiesModule::CheckTagFiles()
{
	TArray<FString> OutPaths;
	UGameplayTagsManager::Get().GetTagSourceSearchPaths(OutPaths);

	TArray<const FGameplayTagSource*> AllSources;
	UGameplayTagsManager::Get().FindTagSourcesWithType(EGameplayTagSourceType::DefaultTagList, AllSources);
	UGameplayTagsManager::Get().FindTagSourcesWithType(EGameplayTagSourceType::RestrictedTagList, AllSources);
	UGameplayTagsManager::Get().FindTagSourcesWithType(EGameplayTagSourceType::TagList, AllSources);

	bool bChanged = false;
	for (const FGameplayTagSource* TagSource : AllSources) 
	{
		FString TagFile = TagSource->GetConfigFileName();

		if (IFileManager::Get().FileExists(*TagFile))
		{
			const FDateTime FoundFileTime = IFileManager::Get().GetTimeStamp(*TagFile);

			FTimespan Delta = FDateTime::UtcNow() - FoundFileTime;

			if (Delta.GetTotalSeconds() <= 3.f)
			{
				// UE_LOG(LogTemp, Warning, TEXT("%s, %s %d %f"), *FoundFileTime.ToString(), *FDateTime::UtcNow().ToString(), Delta.GetSeconds(), Delta.GetTotalSeconds());
				GConfig->LoadFile(TagSource->GetConfigFileName());

				FText Msg = FText::FromString(FString::Printf(TEXT("Reloaded %s"), *TagSource->GetConfigFileName()));
				// UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
				FNotificationInfo Notification(Msg);
				Notification.ExpireDuration = 5.0f;
				FSlateNotificationManager::Get().AddNotification(Notification);
				bChanged = true;
			}
		}
	}

	if (bChanged)
	{
		UGameplayTagsManager::Get().EditorRefreshGameplayTagTree();
	}
}

void FFPEditorUtilitiesModule::ReadTableFiles(UDataTable* Table)
{
	FString TagSource = "";
	for (auto& TablesUsingGameplayTag : UFPEditorUtilitySettings::Get().TablesUsingGameplayTags)
	{
		if (TablesUsingGameplayTag.DataTable.LoadSynchronous() == Table)
		{
			TagSource = TablesUsingGameplayTag.TagPath.FilePath;
			FString Left;
			TablesUsingGameplayTag.TagPath.FilePath.Split("/", &Left, &TagSource, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		}
	}

	if (TagSource.IsEmpty())
	{
		return; 
	}

	for (FName RowName : Table->GetRowNames())
	{
		FString RowStr = RowName.ToString();
		if (RowStr.Contains("RowName"))
		{
			continue;
		}

		if (UGameplayTagsManager::Get().IsDictionaryTag(FName(RowStr)))
		{
			continue;
		}

		UE_LOG(LogTemp, Log, TEXT("Auto added new tag %s"), *RowStr);
		IGameplayTagsEditorModule::Get().AddNewGameplayTagToINI(RowStr, "", FName(TagSource));
	}
}

void FFPEditorUtilitiesModule::BindTables()
{
	for (auto Table : BoundTables)
	{
		if (Table.IsValid())
		{
			Table->OnDataTableChanged().RemoveAll(this);
		}
	}

	BoundTables.Empty();

	// try import tags when a data table is changed
	for (auto& TablesUsingGameplayTag : UFPEditorUtilitySettings::Get().TablesUsingGameplayTags)
	{
		if (auto Table = TablesUsingGameplayTag.DataTable.LoadSynchronous())
		{
			ReadTableFiles(Table);
			Table->OnDataTableChanged().AddRaw(this, &FFPEditorUtilitiesModule::ReadTableFiles, Table);
			BoundTables.Add(Table);
		}
	}
}

void FFPEditorUtilitiesModule::RegisterGameCategory()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FString ProjName = FString(FApp::GetProjectName());
	static const FString Prefix = FString::Printf(TEXT("/Script/%s"), *ProjName);

	for (const auto* const Class : TObjectRange<UClass>())
	{
		if (!Class->GetClassPathName().ToString().StartsWith(Prefix)) 
		{ 
			continue; 
		}

		const TSharedRef<FPropertySection> Section = PropertyModule.FindOrCreateSection(
			Class->GetFName(),
			TEXT("MyProject"),
			INVTEXT("🍩Project")
		);

		Section->AddCategory(TEXT("Default"));
		Section->AddCategory(Class->GetFName());
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFPEditorUtilitiesModule, FPEditorUtilities)

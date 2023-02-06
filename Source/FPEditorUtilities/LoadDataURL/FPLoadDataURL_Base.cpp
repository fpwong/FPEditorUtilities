// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPLoadDataURL_Base.h"

#include "ContentBrowserModule.h"
#include "ObjectEditorUtils.h"
#include "FPGetGoogleSheet.h"
#include "ToolMenus.h"
#include "Interfaces/IMainFrameModule.h"
#include "Misc/LazySingleton.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

static FName NAME_URL_SOURCE("FPURLSource");

void SFPURLEntry::Construct(const FArguments& InArgs)
{
	OnURLEntered = InArgs._OnUrlEntered;

	ChildSlot.Padding(16.0f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(INVTEXT("Enter URL"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(8.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Center)
			[
				SAssignNew(EditableText, SEditableTextBox)
				.MinDesiredWidth(500)
				.Text(FText::FromString(InArgs._DefaultURL))
				.BackgroundColor(FSlateColor(FLinearColor(0.9f, 0.9f, 0.9f)))
			]
		]
		+ SVerticalBox::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Apply")))
				.OnClicked_Lambda([&]
				{
					OnURLEntered.ExecuteIfBound(EditableText->GetText().ToString());
					FSlateApplication::Get().GetActiveTopLevelWindow()->RequestDestroyWindow();
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Cancel")))
				.OnClicked_Lambda([&]
				{
					FSlateApplication::Get().GetActiveTopLevelWindow()->RequestDestroyWindow();
					return FReply::Handled();
				})
			]
		]
	];
}

void FFPLoadDataURL_Base::Init()
{
	SetValidClasses(ValidAssetName, ValidAssetEditorName);
	if (!ValidAssetName.IsValid() || !ValidAssetEditorName.IsValid())
	{
		return;
	}

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBAssetMenuExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
	CBAssetMenuExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedAssets::CreateRaw(this, &FFPLoadDataURL_Base::MakeContextMenuExtender));

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OnAssetOpenedInEditor().AddRaw(this, &FFPLoadDataURL_Base::OnAssetOpenedInEditor);
}

void FFPLoadDataURL_Base::OnAssetOpenedInEditor(UObject* Asset, IAssetEditorInstance* AssetEditor)
{
	if (AssetEditor->GetEditorName() == ValidAssetEditorName)
	{
		if (FAssetEditorToolkit* AssetEditorToolkit = static_cast<FAssetEditorToolkit*>(AssetEditor))
		{
			TSharedRef<FUICommandList> ToolkitCommands = AssetEditorToolkit->GetToolkitCommands();
			TSharedRef<FExtender> ToolbarExtender = MakeShareable(new FExtender);

			TSharedRef<const FExtensionBase> Extension = ToolbarExtender->AddToolBarExtension(
				"Asset",
				EExtensionHook::After,
				ToolkitCommands,
				FToolBarExtensionDelegate::CreateRaw(this, &FFPLoadDataURL_Base::ExtendToolbar, TWeakObjectPtr<UObject>(Asset)));

			AssetEditorToolkit->AddToolbarExtender(ToolbarExtender);
		}
	}
}

void FFPLoadDataURL_Base::ExtendToolbar(FToolBarBuilder& ToolbarBuilder, TWeakObjectPtr<UObject> Object)
{
	ToolbarBuilder.AddToolBarButton(
		FExecuteAction::CreateRaw(this, &FFPLoadDataURL_Base::OpenWindow, Object),
		NAME_None,
		INVTEXT("Import URL"),
		FText::FromString("Import From URL"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.GameSettings")
	);
}

TSharedRef<FExtender> FFPLoadDataURL_Base::MakeContextMenuExtender(const TArray<FAssetData>& AssetDataList)
{
	if (AssetDataList.Num() != 1 || AssetDataList[0].AssetClassPath.GetAssetName() != ValidAssetName)
		return MakeShareable(new FExtender());

	FAssetData AssetData;
	// AssetData.FindTag()

	TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());

	TWeakObjectPtr<UObject> Object = Cast<UObject>(AssetDataList[0].GetAsset());

	MenuExtender->AddMenuExtension(
		"ImportedAssetActions",
		EExtensionHook::Before,
		TSharedPtr<FUICommandList>(),
		FMenuExtensionDelegate::CreateRaw(this, &FFPLoadDataURL_Base::AddMenuEntry, Object));

	return MenuExtender.ToSharedRef();
}

void FFPLoadDataURL_Base::AddMenuEntry(FMenuBuilder& MenuBuilder, TWeakObjectPtr<UObject> Object)
{
	MenuBuilder.AddMenuEntry(
		FText::FromString("Load URL"),
		FText::FromString("Load data from URL"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FFPLoadDataURL_Base::OpenWindow, Object))
	);
}

void FFPLoadDataURL_Base::HandleURLEntered(FString URL, TWeakObjectPtr<UObject> Object)
{
	if (Object.IsValid())
	{
		UObject* Table = Object.Get();
		FString GoogleSheetId = URL;

		if (UPackage* AssetPackage = Object->GetPackage())
		{
			if (UMetaData* TableMetaData = AssetPackage->GetMetaData())
			{
				TableMetaData->SetValue(Table, NAME_URL_SOURCE, *GoogleSheetId);
			}
		}

		ImportFromGoogleSheets(Object, GoogleSheetId);

		// const TSharedPtr<SWindow> ActiveWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
		// ActiveWindow->RequestDestroyWindow();
	}
}

void FFPLoadDataURL_Base::OpenWindow(TWeakObjectPtr<UObject> Object)
{
	TSharedPtr<SEditableTextBox> TestBox;

	FString DefaultURL;
	if (UPackage* AssetPackage = Object->GetPackage())
	{
		if (AssetPackage->HasMetaData())
		{
			if (UMetaData* TableMetaData = AssetPackage->GetMetaData())
			{
				DefaultURL = TableMetaData->GetValue(Object.Get(), NAME_URL_SOURCE);
			}
		}
	}

	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(INVTEXT("Enter URL"))
		.CreateTitleBar(false)
		.Type(EWindowType::Menu)
		.IsPopupWindow(true) // the window automatically closes when user clicks outside of it
		.SizingRule(ESizingRule::Autosized)
		.FocusWhenFirstShown(true)
		.ActivationPolicy(EWindowActivationPolicy::FirstShown)
		[
			SNew(SFPURLEntry)
			.DefaultURL(DefaultURL)
			.OnUrlEntered(FFPOnURLEntered::CreateRaw(this, &FFPLoadDataURL_Base::HandleURLEntered, Object))
		];

	FSlateApplication::Get().AddWindow(Window);
}

void FFPLoadDataURL_Base::ImportFromGoogleSheets(TWeakObjectPtr<UObject> Object, FString GoogleSheetsId)
{
	if (!Object.IsValid())
	{
		return;
	}

	UFPGetGoogleSheets* GetGoogleSheets = NewObject<UFPGetGoogleSheets>(UFPGetGoogleSheets::StaticClass());
	GetGoogleSheets->OnResponseDelegate.BindRaw(this, &FFPLoadDataURL_Base::ReceiveCSV, Object);
	GetGoogleSheets->SendRequest(GoogleSheetsId);

	//https://docs.google.com/spreadsheets/d/1IGLqj-ViOm8OdB9R8kl3Jg3TdvJ6F3-QOSX149eza4w/edit?usp=sharing
	UE_LOG(LogTemp, Warning, TEXT("Importing from google sheets~"));
}

// void FFPLoadDataURL_Base::ReceiveCSV(FString CSV, TWeakObjectPtr<UObject> Object)
// {
// 	// need to override this!
//
// 	if (!Object.IsValid())
// 	{
// 		return;
// 	}
//
// 	// Object->CreateTableFromCSVString(CSV);
// 	//
// 	// UObject* Table = Object.Get();
// 	// FObjectEditorUtils::BroadcastPostChange(Table, FObjectEditorUtils::EObjectChangeInfo::RowList);
// 	// GEditor->RedrawAllViewports();
// }

#undef LOCTEXT_NAMESPACE

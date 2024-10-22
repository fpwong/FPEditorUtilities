// Fill out your copyright notice in the Description page of Project Settings.


#include "FPObjectTableEditor.h"

#include "AssetSelection.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "EditorDialogLibrary.h"
#include "IContentBrowserSingleton.h"
#include "IPropertyTable.h"
#include "ISinglePropertyView.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor/EditorWidgets/Public/SAssetDropTarget.h"
#include "Kismet2/SClassPickerDialog.h"
#include "UObject/PropertyAccessUtil.h"
#include "ClassViewerFilter.h"
#include "ObjectTools.h"
#include "ActorFactories/ActorFactory.h"
#include "Factories/DataAssetFactory.h"

void SFPObjectTableRow::Construct(const FArguments& InArgs, TSharedPtr<FFPObjectData> InReference, const TSharedRef<STableViewBase>& OwnerTable)
{
	Reference = InReference;
	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments(), OwnerTable);
}

TSharedRef<SWidget> SFPObjectTableRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	TSharedRef<SWidget> ColumnWidget = SNullWidget::NullWidget;

	FSinglePropertyParams Params;
	Params.NamePlacement = EPropertyNamePlacement::Type::Hidden;
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	TSharedRef<SHorizontalBox> HBox = SNew(SHorizontalBox);

	if (Reference->Object.IsValid())
	{
		UObject* Obj = Reference->Object.Get();
		if (ColumnName == TEXT("___FPRowName"))
		{
			ColumnWidget = SNew(STextBlock).Text(FText::Format(INVTEXT("{0}"), FText::FromString(GetNameSafe(Obj))));
		}

		// else if (ColumnName == TEXT("Value"))
		// {
		// 	for (TFieldIterator<FProperty> PropertyIt(Obj->GetClass()); PropertyIt; ++PropertyIt)
		// 	{
		// 		TSharedPtr<ISinglePropertyView> Test = EditModule.CreateSingleProperty(Obj, PropertyIt->GetFName(), Params);
		// 		HBox->AddSlot().AutoWidth().AttachWidget(Test.ToSharedRef());
		// 		ColumnWidget = HBox;
		// 	}
		// }

		for (TFieldIterator<FProperty> PropertyIt(Obj->GetClass(), EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
		{
			if (ColumnName == PropertyIt->GetFName())
			{
				ColumnWidget = EditModule.CreateSingleProperty(Obj, PropertyIt->GetFName(), Params).ToSharedRef();
				break;
			}
		}
	}

	return SNew(SBox)
			// .HeightOverride(FUsdStageEditorStyle::Get()->GetFloat("UsdStageEditor.ListItemHeight"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				// .Padding(UsdReferencesListConstants::RowPadding)
				.AutoWidth()
				[
					ColumnWidget
				]
			];
}

// void SMyObjectTable::Construct(const FArguments& InArgs)
// {
// }

void SFPObjectTableListView::Construct(const FArguments& InArgs)
{
	SAssignNew(HeaderRowWidget, SHeaderRow);

	SListView::Construct
	(
		SListView::FArguments()
		.ListItemsSource(&Rows)
		.OnGenerateRow(this, &SFPObjectTableListView::OnGenerateRow)
		.HeaderRow(HeaderRowWidget)
	);
}

void SFPObjectTableListView::Refresh(UFPObjectTable* TableSettings)
{
	if (!TableSettings->ClassFilter)
	{
		return;
	}

	HeaderRowWidget->ClearColumns();
	HeaderRowWidget->AddColumn(
		SHeaderRow::Column(FName(TEXT("___FPRowName")))
		.DefaultLabel(INVTEXT("RowName"))
		.FillWidth(100.f));


	for (TFieldIterator<FProperty> PropertyIt(TableSettings->ClassFilter, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
	{
		UE_LOG(LogTemp, Warning, TEXT("Made column %s"), *PropertyIt->GetName());
		SHeaderRow::FColumn::FArguments ColumnArgs;
		ColumnArgs.ColumnId(PropertyIt->GetFName());
		ColumnArgs.DefaultLabel(FText::FromString(PropertyIt->GetName()));
		ColumnArgs.FillWidth(100.f);
		HeaderRowWidget->AddColumn(ColumnArgs);
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	TArray<FAssetData> AssetDataList;
	auto RelativePath = TableSettings->RootDirectory.Path;
	FPaths::MakePathRelativeTo(RelativePath, *FPaths::ProjectDir());
	AssetRegistryModule.Get().GetAssetsByPath(FName(*TableSettings->RootDirectory.Path), AssetDataList);

	UE_LOG(LogTemp, Warning, TEXT("Searched %s %s"), *RelativePath, *TableSettings->RootDirectory.Path);
	if (AssetDataList.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Found no matching assets empty!"));
	}

	Rows.Empty();
	for (FAssetData DataList : AssetDataList)
	{
		TSharedPtr<FFPObjectData> NewRow = MakeShared<FFPObjectData>();
		NewRow->Object = DataList.GetAsset();

		if (!NewRow->Object.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load obj? %s"), *DataList.AssetName.ToString());
			continue;
		}

		if (!NewRow->Object->IsA(TableSettings->ClassFilter))
		{
			continue;
		}

		if (NewRow->Object.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Add row %s"), *GetNameSafe(NewRow->Object.Get()));
			Rows.Add(NewRow);
		}
	}

	Rows.StableSort([](TSharedPtr<FFPObjectData> A, TSharedPtr<FFPObjectData> B) {
		return GetNameSafe(A->Object.Get()) < GetNameSafe(B->Object.Get());
	});
	// SetItemsSource(&Rows);
	RequestListRefresh();
}

TArray<UObject*> SFPObjectTableListView::GetSelectedObjects()
{
	TArray<UObject*> OutObjs;
	for (TSharedPtr<FFPObjectData> Row : SelectedItems)
	{
		if (Row->Object.IsValid())
		{
			OutObjs.Add(Row->Object.Get());
		}
	}
	return OutObjs;
}

TSharedRef<ITableRow> SFPObjectTableListView::OnGenerateRow(TSharedPtr<FFPObjectData> InDisplayNode, const TSharedRef<STableViewBase>& OwnerTable)
{
	UE_LOG(LogTemp, Warning, TEXT("Generate row %s"), *GetNameSafe(InDisplayNode->Object.Get()));
	return SNew(SFPObjectTableRow, InDisplayNode, OwnerTable);
}

void SFPObjectTableEditor::Construct(const FArguments& InArgs)
{
	TableSettings = InArgs._Settings;

	TableSettings->OnChange.AddRaw(this, &SFPObjectTableEditor::RefreshTable);

	// FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	// FSinglePropertyParams Params;
	// TSharedPtr<ISinglePropertyView> Test = EditModule.CreateSingleProperty(nullptr, FName(""), Params);
	// Test.ToSharedRef();

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	FPathPickerConfig PathPickerConfig;
	PathPickerConfig.OnPathSelected = FOnPathSelected::CreateRaw(this, &SFPObjectTableEditor::OnPathSet);

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	NewPropertyTable = PropertyEditorModule.CreatePropertyTable();
	NewPropertyTable->SetIsUserAllowedToChangeRoot(false);
	NewPropertyTable->SetSelectionMode(ESelectionMode::SingleToggle);
	NewPropertyTable->SetSelectionUnit(EPropertyTableSelectionUnit::Cell);
	NewPropertyTable->SetShowObjectName(true);

	// class FAssetClassParentFilter : public IClassViewerFilter
	// {
	// public:
	// 	/** All children of these classes will be included unless filtered out by another setting. */
	// 	TSet< const UClass* > AllowedChildrenOfClasses;
	//
	// 	/** Disallowed class flags. */
	// 	EClassFlags DisallowedClassFlags;
	//
	// 	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override;
	//
	// 	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override;
	// };

	FDetailsViewArgs ViewArgs;
	ViewArgs.bAllowSearch = true;
	ViewArgs.bHideSelectionTip = false;
	ViewArgs.NameAreaSettings = FDetailsViewArgs::ObjectsUseNameArea;
	// ViewArgs.NotifyHook = this;

	DetailsView = PropertyEditorModule.CreateDetailView(ViewArgs);

	ObjectTable = SNew(SFPObjectTableListView);
	ObjectTable->MyOnSelectionChanged().BindRaw(this, &SFPObjectTableEditor::OnSelectionChanged);
	ObjectTable->Refresh(TableSettings);

	ChildSlot
	[
		// SNew(STextBlock, "Test")
		// SNew(SButton).OnClicked_Lambda([this]()
		// {
		// 	UObject* OutObj = nullptr;
		// 	return FReply::Handled();
		// })
		// [
		// 	SNew(STextBlock).Text(INVTEXT("Test"))
		// ]

		SNew(SSplitter).Orientation(Orient_Horizontal)
		+ SSplitter::Slot()
		.Value(0.8f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).OnClicked_Lambda([this]() {
						UE_LOG(LogTemp, Warning, TEXT("Chosen class %s"), *TableSettings->ClassFilter->GetName());

						// static FName AssetToolsModuleName = FName("AssetTools");
						FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>(FName("AssetTools"));

						TArray<UFactory*> Factories = AssetToolsModule.Get().GetNewAssetFactories();
						UFactory* FoundFactory = nullptr;
						for (UFactory* Factory : Factories)
						{
							if (Factory->DoesSupportClass(TableSettings->ClassFilter))
							{
								FoundFactory = Factory;
								break;
							}
						}

						if (TableSettings->ClassFilter->IsChildOf(UDataAsset::StaticClass()))
						{
							FoundFactory = NewObject<UDataAssetFactory>();
						}

						if (FoundFactory)
						{
							FString DefaultName = TableSettings->ClassFilter->GetName();
							DefaultName.RemoveFromEnd(TEXT("_C"));

							FString PackageName;
							FString UniqueName;
							AssetToolsModule.Get().CreateUniqueAssetName(TableSettings->RootDirectory.Path / DefaultName, "", PackageName, UniqueName);
							UE_LOG(LogTemp, Warning, TEXT("Checking %s"), *(TableSettings->RootDirectory.Path / TableSettings->GetClass()->GetName()));
							UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(UniqueName, TableSettings->RootDirectory.Path, TableSettings->ClassFilter, FoundFactory);
							RefreshTable();
						}
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(INVTEXT("Create New"))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).OnClicked_Lambda([this]() {
						if (ObjectTable.IsValid())
						{
							if (ObjectTools::DeleteObjects(ObjectTable->GetSelectedObjects()))
							{
								RefreshTable();
							}
						}
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(INVTEXT("Delete"))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).OnClicked_Lambda([this]() {
						UE_LOG(LogTemp, Warning, TEXT("TODO"));
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(INVTEXT("Duplicate"))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).OnClicked_Lambda([this]() {
						UE_LOG(LogTemp, Warning, TEXT("TODO"));
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(INVTEXT("Bulk Rename"))
					]
				]
			]
			// +SVerticalBox::Slot()
			// [
			// 	PropertyEditorModule.CreatePropertyTableWidget(TextureAnalyzerTable.ToSharedRef())
			// ]
			+ SVerticalBox::Slot()
			[
				SNew(SScrollBox).Orientation(Orient_Horizontal)
				+ SScrollBox::Slot()
				[
					ObjectTable.ToSharedRef()
				]
				// SAssignNew(Rows, SVerticalBox)
			]
		]
		+ SSplitter::Slot().Value(0.2f)
		[
			DetailsView.ToSharedRef()
		]

		// SNew(SAssetDropTarget)
		// .OnAreAssetsAcceptableForDropWithReason_Lambda([&](TArrayView<FAssetData> InAssets, FText& OutReason) -> bool
		// {
		// 	return true;
		// 	// return OnShouldFilterAsset.IsBound() && OnShouldFilterAsset.Execute(InAssets[0]);
		// })
		// .OnAssetsDropped_Lambda([&](const FDragDropEvent& Event, TArrayView<FAssetData> InAssets) -> void
		// {
		// 	// AssetComboButton->SetIsOpen(false);
		// 	// OnSetObject.ExecuteIfBound(InAssets[0]);
		// 	UE_LOG(LogTemp, Warning, TEXT("Selected this thing %s"), *GetNameSafe(InAssets[0].GetAsset()));
		//
		// 	UEditorDialogLibrary::ShowObjectDetailsView(INVTEXT("Show thing"), InAssets[0].GetAsset());
		//
		// })
		// [
		// 	SNew(STextBlock).Text(INVTEXT("What this"))
		// ]
		// [
		// 	SAssignNew(ValueContentBox, SHorizontalBox)
		// ]
	];
}

SFPObjectTableEditor::~SFPObjectTableEditor()
{
	if (TableSettings)
	{
		TableSettings->OnChange.RemoveAll(this);
	}
}

TSharedRef<SDockTab> SFPObjectTableEditor::CreateTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab).TabRole(ETabRole::NomadTab)
		[
			SNew(SFPObjectTableEditor)
		];
}

void SFPObjectTableEditor::OnPathSet(const FString& Path)
{
	// TableSettings->SetRootDirectory(FDirectoryPath(Path));

	if (false)
	{
		// FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		//
		// FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		//
		// TArray<FAssetData> AssetDataList;
		// AssetRegistryModule.Get().GetAssetsByPath(FName(*Path), AssetDataList);
		//
		// FSinglePropertyParams Params;
		// FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		//
		// TArray<UObject*> Objects;
		// Rows->ClearChildren();
		//
		// SNew(SListView<TSharedPtr<FMyObjectData>>).OnSelectionChanged(this, &SMyObjectTableEditor::OnSelectionChanged);
		//
		// // SFPObjectTableEditor::FOnSelectionChanged
		// ObjectTable = SNew(SMyObjectTable, Path, TableSettings->ClassFilter);
		// ObjectTable->MyOnSelectionChanged().BindRaw(this, &SMyObjectTableEditor::OnSelectionChanged);
		// Rows->AddSlot().AttachWidget(ObjectTable.ToSharedRef());
	}

	// for (FAssetData DataList : AssetDataList)
	// {
	// 	TSharedRef<SHorizontalBox> HBox = SNew(SHorizontalBox);
	//
	// 	if (UObject* Asset = DataList.GetAsset())
	// 	{
	// 		for (TFieldIterator<FProperty> PropertyIt(Asset->GetClass()); PropertyIt; ++PropertyIt)
	// 		{
	// 			TSharedPtr<ISinglePropertyView> Test = EditModule.CreateSingleProperty(Asset, PropertyIt->GetFName(), Params);
	//
	// 			// HBox->AddSlot().AutoWidth().AttachWidget(SNew(STextBlock).Text(FText::FromString(*DataList.AssetName.ToString())));
	// 			HBox->AddSlot().AutoWidth().AttachWidget(Test.ToSharedRef());
	// 		}
	//
	// 		Objects.Add(Asset);
	// 	}
	//
	// 	Rows->AddSlot().AutoHeight().AttachWidget(HBox);
	// }
	//
	// TextureAnalyzerTable->SetObjects(Objects);
}

void SFPObjectTableEditor::OnSelectionChanged(TSharedPtr<FFPObjectData> ObjectData, ESelectInfo::Type SelectInfo)
{
	if (ObjectTable.IsValid() && DetailsView.IsValid())
	{
		DetailsView.Get()->SetObjects(ObjectTable->GetSelectedObjects());
	}
}

void SFPObjectTableEditor::RefreshTable()
{
	UE_LOG(LogTemp, Warning, TEXT("Refresh table?"));
	ObjectTable->Refresh(TableSettings);
}

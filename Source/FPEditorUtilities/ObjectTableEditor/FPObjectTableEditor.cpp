// Fill out your copyright notice in the Description page of Project Settings.


#include "FPObjectTableEditor.h"

#include "AssetToolsModule.h"
#include "AssetViewUtils.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "IPropertyTable.h"
#include "ISinglePropertyView.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ObjectTools.h"
#include "AssetRegistry/AssetRegistryHelpers.h"
#include "Editor/PropertyEditor/Private/PropertyNode.h"
#include "Factories/BlueprintFactory.h"
#include "Factories/DataAssetFactory.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Filters/SAssetFilterBar.h"
#include "GeometryCollection/Facades/CollectionPositionTargetFacade.h"

UObject* FFPObjectData::GetObj()
{
	UObject* Obj = AssetData.GetAsset();

	if (UBlueprint* BP = Cast<UBlueprint>(Obj))
	{
		if (BP->GeneratedClass)
		{
			return BP->GeneratedClass->GetDefaultObject();
		}
	}

	return Obj;
}

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

	if (Reference->GetObj())
	{
		UObject* Obj = Reference->GetObj();
		if (ColumnName == TEXT("___FPRowName"))
		{
			ColumnWidget = SAssignNew(InlineEditableText, SInlineEditableTextBlock)
				.IsReadOnly(false)
				.Text(this, &SFPObjectTableRow::GetObjectName)
				.OnTextCommitted(this, &SFPObjectTableRow::HandleRename);
		}

		if (auto PropWidget = EditModule.CreateSingleProperty(Obj, ColumnName, Params))
		{
			ColumnWidget = PropWidget.ToSharedRef();
		}
	}

	return SNew(SBox).Padding(2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				ColumnWidget
			]
		];
}

void SFPObjectTableRow::HandleRename(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::Type::OnEnter)
	{
		auto Pkg = Reference->AssetData.GetAsset();
		
		if (Pkg)
		{
			FString NewName = Text.ToString();
			FString Path = FPaths::GetPath(Pkg->GetPathName());

			UE_LOG(LogTemp, Warning, TEXT("Rename %s to %s"), *Path, *NewName);

			FAssetRenameData RenameData(Pkg, Path, NewName);
			IAssetTools::Get().RenameAssets({RenameData});
		}
	}
}

FText SFPObjectTableRow::GetObjectName() const
{
	return FText::FromName(Reference->AssetData.AssetName);
}

FReply SFPObjectTableRow::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	if (InlineEditableText->IsHovered())
	{
		InlineEditableText->EnterEditingMode();
	}
	return FReply::Handled();
}

void SFPObjectTableListView::Construct(const FArguments& InArgs, UFPObjectTable* InTableSettings)
{
	ObjectTable = InTableSettings;

	SAssignNew(HeaderRowWidget, SHeaderRow);

	FCoreUObjectDelegates::OnObjectRenamed.AddRaw(this, &SFPObjectTableListView::HandleObjRenamed);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

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
	AssetDataList.Empty();
	if (!TableSettings->ClassFilter)
	{
		return;
	}

	HeaderRowWidget->ClearColumns();
	HeaderRowWidget->AddColumn(
		SHeaderRow::Column(FName(TEXT("___FPRowName")))
		.DefaultLabel(INVTEXT("Object Name"))
		.FillWidth(100.f));


	for (TFieldIterator<FProperty> PropertyIt(TableSettings->ClassFilter, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		if (!TableSettings->CheckedProperties.Contains(PropertyIt->GetName()))
		{
			continue;
		}

		UE_LOG(LogTemp, Warning, TEXT("Made column %s"), *PropertyIt->GetName());
		SHeaderRow::FColumn::FArguments ColumnArgs;
		ColumnArgs.ColumnId(PropertyIt->GetFName());
		ColumnArgs.DefaultLabel(FText::FromString(PropertyIt->GetName()));
		ColumnArgs.FillWidth(100.f);
		HeaderRowWidget->AddColumn(ColumnArgs);
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	FString RelativePath = TableSettings->RootDirectory.Path;
	FPaths::MakePathRelativeTo(RelativePath, *FPaths::ProjectDir());
	// AssetRegistryModule.Get().GetAssetsByPath(FName(*TableSettings->RootDirectory.Path), AssetDataList, TableSettings->bCheckSubfolders);

	FARFilter Filter;
	if (!TableSettings->RootDirectory.Path.IsEmpty())
	{
		Filter.bRecursivePaths = true;
		Filter.PackagePaths.Add(FName(TableSettings->RootDirectory.Path));
	}

	Filter.ClassPaths.Add(TableSettings->ClassFilter->GetClassPathName());
	Filter.bRecursiveClasses = true;

	bool bIsBlueprint = UBlueprint::GetBlueprintFromClass(TableSettings->ClassFilter) != nullptr;
	bool bIsDataAsset = TableSettings->ClassFilter->IsChildOf(UDataAsset::StaticClass());

	if (bIsBlueprint && !bIsDataAsset)
	{
		UAssetRegistryHelpers::GetBlueprintAssets(Filter, AssetDataList);
	}
	else
	{
		AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);
	}

	UE_LOG(LogTemp, Warning, TEXT("Searched %s %s"), *RelativePath, *TableSettings->RootDirectory.Path);
	if (AssetDataList.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Found no matching assets empty!"));
	}

	Rows.Empty();
	for (FAssetData DataList : AssetDataList)
	{
		TSharedPtr<FFPObjectData> NewRow = MakeShared<FFPObjectData>();
		NewRow->AssetData = DataList;

		if (NewRow->AssetData.IsValid())
		{
			Rows.Add(NewRow);
		}
	}

	Rows.StableSort([](TSharedPtr<FFPObjectData> A, TSharedPtr<FFPObjectData> B)
	{
		return A->AssetData.AssetName.ToString() < B->AssetData.AssetName.ToString();
	});

	RequestListRefresh();
}

TArray<UObject*> SFPObjectTableListView::GetSelectedObjects()
{
	TArray<UObject*> OutObjs;
	for (TSharedPtr<FFPObjectData> Row : SelectedItems)
	{
		if (Row->GetObj())
		{
			OutObjs.Add(Row->GetObj());
		}
	}
	return OutObjs;
}

TArray<FAssetData> SFPObjectTableListView::GetSelectedAssets()
{
	TArray<FAssetData> OutObjs;
	for (TSharedPtr<FFPObjectData> Row : SelectedItems)
	{
		OutObjs.Add(Row->AssetData);
	}
	return OutObjs;
}

TSharedRef<ITableRow> SFPObjectTableListView::OnGenerateRow(TSharedPtr<FFPObjectData> InDisplayNode, const TSharedRef<STableViewBase>& OwnerTable)
{
	// UE_LOG(LogTemp, Warning, TEXT("Generate row %s"), *GetNameSafe(InDisplayNode->GetObj()));
	return SNew(SFPObjectTableRow, InDisplayNode, OwnerTable);
}

void SFPObjectTableListView::HandleObjRenamed(UObject* Object, UObject* OldOuter, FName OldName)
{
	for (FAssetData& Asset : AssetDataList)
	{
		if (Asset.AssetName == OldName)
		{
			Refresh(ObjectTable);
		}
	}
}

void SFPToggleButtons::Construct(const FArguments& InArgs)
{
	SetUseAllottedSize(true);
	SetInnerSlotPadding(FVector2D(4.0f, 4.0f));
	CheckedProperties = InArgs._CheckedProperties;
	OnPropertiesChanged = InArgs._OnPropertiesChanged;
	SetProperties(InArgs._Properties);
}

void SFPToggleButtons::OnPropertyCheckedChanged(ECheckBoxState CheckBoxState, FString PropertyName)
{
	if (CheckBoxState == ECheckBoxState::Checked)
	{
		if (AllProperties.Num() == CheckedProperties.Num())
		{
			CheckedProperties.Empty();
			CheckedProperties.Add(PropertyName);
		}
		else
		{
			CheckedProperties.Add(PropertyName);
		}
	}
	else
	{
		CheckedProperties.Remove(PropertyName);
	}

	OnPropertiesChanged.ExecuteIfBound(CheckedProperties);
}

ECheckBoxState SFPToggleButtons::IsPropertyChecked(FString String) const
{
	if (AllProperties.Num() == CheckedProperties.Num())
	{
		return ECheckBoxState::Unchecked;
	}

	return CheckedProperties.Contains(String)
		? ECheckBoxState::Checked
		: ECheckBoxState::Unchecked;
}

void SFPToggleButtons::OnAllChecked(ECheckBoxState CheckBoxState)
{
	if (CheckBoxState == ECheckBoxState::Checked)
	{
		CheckedProperties = AllProperties;
	}
	else
	{
		CheckedProperties.Empty();
	}

	OnPropertiesChanged.ExecuteIfBound(CheckedProperties);
}

ECheckBoxState SFPToggleButtons::IsAllChecked() const
{
	return AllProperties.Num() == CheckedProperties.Num()
		? ECheckBoxState::Checked
		: ECheckBoxState::Unchecked;
}

void SFPToggleButtons::SetProperties(const TArray<FString>& InProperties)
{
	AllProperties = InProperties;

	ClearChildren();

	// make the all button
	AddSlot().AttachWidget(SNew(SBox)
	.Padding(FMargin(0.0f))
	[
		SNew(SCheckBox)
		.Style(FAppStyle::Get(), "DetailsView.SectionButton")
		.OnCheckStateChanged(this, &SFPToggleButtons::OnAllChecked)
		.IsChecked(this, &SFPToggleButtons::IsAllChecked)
		[
			SNew(STextBlock)
			.TextStyle(FAppStyle::Get(), "SmallText")
			.Text(INVTEXT("All"))
		]
	]);

	for (const FString& Property : InProperties)
	{
		AddSlot().AttachWidget(SNew(SBox)
		.Padding(FMargin(0.0f))
		[
			SNew(SCheckBox)
			.Style(FAppStyle::Get(), "DetailsView.SectionButton")
			.OnCheckStateChanged(this, &SFPToggleButtons::OnPropertyCheckedChanged, Property)
			.IsChecked(this, &SFPToggleButtons::IsPropertyChecked, Property)
			[
				SNew(STextBlock)
				.TextStyle(FAppStyle::Get(), "SmallText")
				.Text(FText::FromString(Property))
			]
		]);
	}
}

void SFPObjectTableEditor::OnTableColumnsChanged(const TArray<FString>& Properties)
{
	TableSettings->CheckedProperties = Properties;
	ObjectTable->Refresh(TableSettings);
}

void SFPObjectTableEditor::OnDetailsViewSectionsChanged(const TArray<FString>& Properties)
{
	TableSettings->DetailsViewSections = Properties;
	DetailsView->ForceRefresh();
}

void SFPObjectTableEditor::Construct(const FArguments& InArgs)
{
	TableSettings = InArgs._Settings;

	TableSettings->OnChange.AddRaw(this, &SFPObjectTableEditor::RefreshTable);
	TableSettings->OnClassChanged.AddRaw(this, &SFPObjectTableEditor::UpdateButtons);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().OnAssetRemoved().AddRaw(this, &SFPObjectTableEditor::HandleAssetRemoved);
	AssetRegistryModule.Get().OnAssetAdded().AddRaw(this, &SFPObjectTableEditor::HandleAssetAdded);
	AssetRegistryModule.Get().OnAssetRenamed().AddRaw(this, &SFPObjectTableEditor::HandleAssetRenamed);

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	FPathPickerConfig PathPickerConfig;
	PathPickerConfig.OnPathSelected = FOnPathSelected::CreateRaw(this, &SFPObjectTableEditor::OnPathSet);

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	NewPropertyTable = PropertyEditorModule.CreatePropertyTable();
	NewPropertyTable->SetIsUserAllowedToChangeRoot(false);
	NewPropertyTable->SetSelectionMode(ESelectionMode::SingleToggle);
	NewPropertyTable->SetSelectionUnit(EPropertyTableSelectionUnit::Cell);
	NewPropertyTable->SetShowObjectName(true);

	FDetailsViewArgs ViewArgs;
	ViewArgs.bAllowSearch = true;
	ViewArgs.bHideSelectionTip = false;
	ViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	ViewArgs.bShowSectionSelector = false;
	ViewArgs.ViewIdentifier = "FPObjectTable";

	ViewArgs.ShouldForceHideProperty = FDetailsViewArgs::FShouldForceHideProperty::CreateLambda([&](const TSharedRef<FPropertyNode>& Property)
	{
		if (!Property->GetProperty())
		{
			return false;
		}

		if (const FString* CategoryNamesMetaData = Property->GetProperty()->FindMetaData("Category"))
		{
			TArray<FString> CategoryNames;
			CategoryNamesMetaData->ParseIntoArray(CategoryNames, TEXT(","), true);

			bool bFound = false;
			for (int i = 0; i < CategoryNames.Num(); i++)
			{
				if (!TableSettings->DetailsViewSections.Contains(*CategoryNames[i]))
				{
					bFound = true;
					break;
				}
			}

			if (!bFound)
			{
				return false;
			}
		}

		return true;
	});

	DetailsView = PropertyEditorModule.CreateDetailView(ViewArgs);

	ObjectTable = SNew(SFPObjectTableListView, TableSettings);
	ObjectTable->MyOnSelectionChanged().BindRaw(this, &SFPObjectTableEditor::OnSelectionChanged);
	ObjectTable->Refresh(TableSettings);

	TSet<FString> ClassProperties;
	TSet<FString> AllProperties;

	if (TableSettings->ClassFilter)
	{
		for (TFieldIterator<FProperty> It(TableSettings->ClassFilter.Get()); It; ++It)
		{
			FProperty* Property = *It;
			UE_LOG(LogTemp, Warning, TEXT("TEST %s"), *Property->GetName());

			AllProperties.Add(Property->GetName());

			if (const FString* CategoryNamesMetaData = Property->FindMetaData("Category"))
			{
				TArray<FString> CategoryNames;
				CategoryNamesMetaData->ParseIntoArray(CategoryNames, TEXT(","), true);
				for (int i = 0; i < CategoryNames.Num(); i++)
				{
					// UE_LOG(LogTemp, Warning, TEXT("\tCATEGORY %s"), *CategoryNames[i]);
					ClassProperties.Add(CategoryNames[i]);
				}
			}
		}
	}

	SAssignNew(DetailViewButtons, SFPToggleButtons)
	.Properties(ClassProperties.Array())
	.CheckedProperties(TableSettings->DetailsViewSections)
	.OnPropertiesChanged(this, &SFPObjectTableEditor::OnDetailsViewSectionsChanged);

	SAssignNew(PropertyButtons, SFPToggleButtons)
	.Properties(AllProperties.Array())
	.OnPropertiesChanged(this, &SFPObjectTableEditor::OnTableColumnsChanged);

	ChildSlot
	[
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
					SNew(SButton).OnClicked_Lambda([this]()
					{
						RefreshTable();
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(INVTEXT("Refresh"))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.OnClicked(this, &SFPObjectTableEditor::HandleNewClassClicked)
					// .OnClicked_Lambda([this]() {
					// 	UE_LOG(LogTemp, Warning, TEXT("Chosen class %s"), *TableSettings->ClassFilter->GetName());
					//
					// 	// static FName AssetToolsModuleName = FName("AssetTools");
					// 	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>(FName("AssetTools"));
					//
					// 	TArray<UFactory*> Factories = AssetToolsModule.Get().GetNewAssetFactories();
					// 	UFactory* FoundFactory = nullptr;
					// 	for (UFactory* Factory : Factories)
					// 	{
					// 		if (Factory->DoesSupportClass(TableSettings->ClassFilter))
					// 		{
					// 			FoundFactory = Factory;
					// 			break;
					// 		}
					// 	}
					//
					// 	if (TableSettings->ClassFilter->IsChildOf(UDataAsset::StaticClass()))
					// 	{
					// 		FoundFactory = NewObject<UDataAssetFactory>();
					// 	}
					//
					// 	if (FoundFactory)
					// 	{
					// 		FString DefaultName = TableSettings->ClassFilter->GetName();
					// 		DefaultName.RemoveFromEnd(TEXT("_C"));
					//
					// 		if (!TableSettings->NameTemplate.IsEmpty())
					// 		{
					// 			DefaultName = TableSettings->NameTemplate; 
					// 		}
					//
					// 		FString PackageName;
					// 		FString UniqueName;
					// 		AssetToolsModule.Get().CreateUniqueAssetName(TableSettings->RootDirectory.Path / DefaultName, "", PackageName, UniqueName);
					// 		UE_LOG(LogTemp, Warning, TEXT("Checking %s"), *(TableSettings->RootDirectory.Path / TableSettings->GetClass()->GetName()));
					// 		UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(UniqueName, TableSettings->RootDirectory.Path, TableSettings->ClassFilter, FoundFactory);
					// 		RefreshTable();
					// 	}
					// 	return FReply::Handled();
					// })
					[
						SNew(STextBlock).Text(INVTEXT("Create New"))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.OnClicked(this, &SFPObjectTableEditor::HandleDeleteClicked)
					// .OnClicked_Lambda([this]() {
					// 	if (ObjectTable.IsValid())
					// 	{
					// 		if (ObjectTools::DeleteObjects(ObjectTable->GetSelectedObjects()))
					// 		{
					// 			RefreshTable();
					// 		}
					// 	}
					// 	return FReply::Handled();
					// })
					[
						SNew(STextBlock).Text(INVTEXT("Delete"))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).OnClicked(this, &SFPObjectTableEditor::HandleDuplicateClicked)
					// .OnClicked_Lambda([this]() {
					// 	UE_LOG(LogTemp, Warning, TEXT("TODO"));
					// 	return FReply::Handled();
					// })
					[
						SNew(STextBlock).Text(INVTEXT("Duplicate"))
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				PropertyButtons.ToSharedRef()
			]
			+ SVerticalBox::Slot().Padding(12)
			[
				SNew(SScrollBox).Orientation(Orient_Horizontal)
				+ SScrollBox::Slot().FillSize(1.0)
				[
					ObjectTable.ToSharedRef()
				]
			]
		]
		+ SSplitter::Slot().Value(0.2f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				DetailViewButtons.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				DetailsView.ToSharedRef()
			]
		]
	];
}

SFPObjectTableEditor::~SFPObjectTableEditor()
{
	if (TableSettings)
	{
		TableSettings->OnChange.RemoveAll(this);
	}

	FCoreUObjectDelegates::OnObjectRenamed.RemoveAll(this);
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
		DetailsView.Get()->SetObjects(ObjectTable->GetSelectedObjects(), true);
	}
}

FReply SFPObjectTableEditor::HandleNewClassClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Create new class %s"), *TableSettings->ClassFilter->GetName());

	// static FName AssetToolsModuleName = FName("AssetTools");
	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>(FName("AssetTools"));

	TArray<UFactory*> Factories = AssetToolsModule.Get().GetNewAssetFactories();
	UFactory* FoundFactory = nullptr;

	UClass* ClassToUse = TableSettings->ClassFilter;

	if (auto BP = UBlueprint::GetBlueprintFromClass(TableSettings->ClassFilter))
	{
		UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
		Factory->ParentClass = BP->GeneratedClass;

		FoundFactory = Factory;
		ClassToUse = Factory->ParentClass;
	}
	else if (TableSettings->ClassFilter->IsChildOf(UDataAsset::StaticClass()))
	{
		auto Factory = NewObject<UDataAssetFactory>();
		Factory->DataAssetClass = TableSettings->ClassFilter;
		FoundFactory = Factory;
	}

	if (!FoundFactory)
	{
		for (UFactory* Factory : Factories)
		{
			if (Factory->DoesSupportClass(TableSettings->ClassFilter))
			{
				FoundFactory = Factory;
				break;
			}
		}
	}

	UObject* CreatedAsset = nullptr;
	if (FoundFactory)
	{
		FString DefaultName = TableSettings->ClassFilter->GetName();
		DefaultName.RemoveFromEnd(TEXT("_C"));

		if (!TableSettings->NameTemplate.IsEmpty())
		{
			DefaultName = TableSettings->NameTemplate;
		}

		FString PackageName;
		FString UniqueName;
		AssetToolsModule.Get().CreateUniqueAssetName(TableSettings->RootDirectory.Path / DefaultName, "", PackageName, UniqueName);
		UE_LOG(LogTemp, Warning, TEXT("Checking %s"), *(TableSettings->RootDirectory.Path / TableSettings->GetClass()->GetName()));
		CreatedAsset = AssetToolsModule.Get().CreateAsset(UniqueName, TableSettings->RootDirectory.Path, FoundFactory->SupportedClass, FoundFactory);
		RefreshTable();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to find a valid factory"))
	}

	if (!CreatedAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create asset"))
	}

	return FReply::Handled();
}

FReply SFPObjectTableEditor::HandleDeleteClicked()
{
	if (ObjectTable.IsValid())
	{
		if (ObjectTools::DeleteObjects(ObjectTable->GetSelectedObjects()))
		{
			RefreshTable();
		}
	}
	return FReply::Handled();
}

FReply SFPObjectTableEditor::HandleDuplicateClicked()
{

	// TArray<UObject*> SelectedObjs = ObjectTable->GetSelectedObjects();
	// if (SelectedObjs.Num() > 0)
	// {
	// 	// FString DefaultName = TableSettings->ClassFilter->GetName();
	// 	// DefaultName.RemoveFromEnd(TEXT("_C"));
	// 	//
	// 	// if (!TableSettings->NameTemplate.IsEmpty())
	// 	// {
	// 	// 	DefaultName = TableSettings->NameTemplate;
	// 	// }
	//
	// 	// FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>(FName("AssetTools"));
	// 	// FString PackageName;
	// 	// FString UniqueName;
	// 	// AssetToolsModule.Get().CreateUniqueAssetName(TableSettings->RootDirectory.Path / DefaultName, "", PackageName, UniqueName);
	// 	// // UE_LOG(LogTemp, Warning, TEXT("Checking %s"), *(TableSettings->RootDirectory.Path / TableSettings->GetClass()->GetName()));
	// 	// AssetViewUtils::CopyAssets(SelectedObjs, TableSettings->RootDirectory.Path);
	//
	// 	for (UObject* Obj : SelectedObjs)
	// 	{
	//
	// 		UE_LOG(LogTemp, Warning, TEXT("Try dup asset %s"), *Obj->GetName())
	// 		IAssetTools::Get().DuplicateAsset(Obj->GetName(), TableSettings->RootDirectory.Path, Obj);
	// 	}
	// }

	for (TSharedPtr<FFPObjectData> Item : ObjectTable->GetSelectedItems())
	{
		UE_LOG(LogTemp, Warning, TEXT("Dupe %s"), *Item->AssetData.AssetName.ToString());

		// FString PackageName;
		// FString UniqueName;
		// AssetToolsModule.Get().CreateUniqueAssetName(TableSettings->RootDirectory.Path / DefaultName, "", PackageName, UniqueName);
		// IAssetTools::Get().DuplicateAsset(Item->AssetData.AssetName.ToString(), TableSettings->RootDirectory.Path, Item->AssetData.GetAsset());
		AssetViewUtils::CopyAssets({Item->AssetData.GetAsset()}, TableSettings->RootDirectory.Path);
		RefreshTable();
	}

	return FReply::Handled();
}

void SFPObjectTableEditor::HandleAssetRenamed(const FAssetData& Asset, const FString& NewName)
{
	if (ObjectTable->AssetDataList.Contains(Asset))
	{
		RefreshTable();
	}
}

void SFPObjectTableEditor::HandleAssetAdded(const FAssetData& Asset)
{
	FARFilter Filter;
	if (!TableSettings->RootDirectory.Path.IsEmpty())
	{
		Filter.bRecursivePaths = true;
		Filter.PackagePaths.Add(FName(TableSettings->RootDirectory.Path));
	}

	Filter.ClassPaths.Add(TableSettings->ClassFilter->GetClassPathName());
	Filter.bRecursiveClasses = true;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FARCompiledFilter CompiledFilter;
	AssetRegistryModule.Get().CompileFilter(Filter, CompiledFilter);
	if (AssetRegistryModule.Get().IsAssetIncludedByFilter(Asset, CompiledFilter))
	{
		RefreshTable();
	}
}

void SFPObjectTableEditor::HandleAssetRemoved(const FAssetData& Asset)
{
	if (ObjectTable->AssetDataList.Contains(Asset))
	{
		RefreshTable();
	}
}

void SFPObjectTableEditor::RefreshTable()
{
	ObjectTable->Refresh(TableSettings);
}

void SFPObjectTableEditor::UpdateButtons()
{
	if (!TableSettings->ClassFilter)
	{
		return;
	}

	TSet<FString> Categories;
	TSet<FString> Properties;

	for (TFieldIterator<FProperty> It(TableSettings->ClassFilter.Get()); It; ++It)
	{
		FProperty* Property = *It;
		// UE_LOG(LogTemp, Warning, TEXT("TEST %s"), *Property->GetName());

		Properties.Add(Property->GetName());

		if (const FString* CategoryNamesMetaData = Property->FindMetaData("Category"))
		{
			TArray<FString> CategoryNames;
			CategoryNamesMetaData->ParseIntoArray(CategoryNames, TEXT(","), true);
			for (int i = 0; i < CategoryNames.Num(); i++)
			{
				UE_LOG(LogTemp, Warning, TEXT("\tCATEGORY %s"), *CategoryNames[i]);
				Categories.Add(CategoryNames[i]);
			}
		}
	}

	TableSettings->CheckedProperties.Empty();
	TableSettings->CheckedCategories = Categories.Array();

	if (DetailViewButtons)
	{
		DetailViewButtons->SetProperties(Categories.Array());
		DetailViewButtons->CheckedProperties = Categories.Array();
	}

	if (PropertyButtons)
	{
		PropertyButtons->SetProperties(Properties.Array());
	}
}

FReply SFPObjectTableEditor::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	TArray<TSharedPtr<FFPObjectData>> SelectedItems = ObjectTable->GetSelectedItems();
	if (SelectedItems.Num() == 1)
	{
		TSharedPtr<FFPObjectData> SelectedItem = SelectedItems[0];
		if (SelectedItem->GetObj())
		{
			if (InKeyEvent.GetKey() == EKeys::F2)
			{
				TSharedPtr<SFPObjectTableRow> Row = StaticCastSharedPtr<SFPObjectTableRow>(ObjectTable->WidgetFromItem(SelectedItem));
				Row->InlineEditableText->EnterEditingMode();
				return FReply::Handled();
			}

			// TODO for some reason the asset doesn't get selected when doing this, maybe move this to a mouse hotkey?
			if (InKeyEvent.GetKey() == EKeys::B && FSlateApplication::Get().GetModifierKeys().IsControlDown())
			{
				// Highlight the asset in content browser
				const TArray<UObject*>& Assets = {SelectedItem->GetObj()};
				const FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
				ContentBrowserModule.Get().SyncBrowserToAssets(Assets, false, true);
				return FReply::Handled();
			}
		}
	}

	return FReply::Unhandled();
}

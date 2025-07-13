// Fill out your copyright notice in the Description page of Project Settings.


#include "FPObjectTableEditor.h"

#include "AssetToolsModule.h"
#include "AssetViewUtils.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "IPropertyTable.h"
#include "ISinglePropertyView.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetRegistryHelpers.h"
#include "Editor/PropertyEditor/Private/PropertyNode.h"
#include "Factories/BlueprintFactory.h"
#include "Factories/DataAssetFactory.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Filters/SAssetFilterBar.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/TransactionObjectEvent.h"
#include "Widgets/Notifications/SNotificationList.h"

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

void SFPPropertyValueContainer::Construct(const FArguments& InArgs, UObject* InObj, FProperty* InProperty)
{
	Obj = InObj;
	Prop = InProperty;

	ChildSlot
	[
		InArgs._Content.Widget
	];
}

FReply SFPPropertyValueContainer::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetModifierKeys().IsShiftDown())
	{
		if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			// paste
			FString PasteValue;
			FPlatformApplicationMisc::ClipboardPaste(PasteValue);

			const FScopedTransaction Transaction(INVTEXT("Paste Property (Object Table)"));
			Obj->Modify();

			Obj->PreEditChange(Prop);
			if (FBlueprintEditorUtils::PropertyValueFromString(Prop, PasteValue, reinterpret_cast<uint8*>(Obj)))
			{
				FPropertyChangedEvent Event(Prop);
				Obj->PostEditChangeProperty(Event);

				FNotificationInfo Notification(FText::FromString(FString::Printf(TEXT("Pasted to property %s"), *PasteValue)));
				FSlateNotificationManager::Get().AddNotification(Notification);
			}
			else
			{
				FNotificationInfo Notification(FText::FromString(FString::Printf(TEXT("Unable to paste %s"), *PasteValue)));
				FSlateNotificationManager::Get().AddNotification(Notification);
			}
		}
		else if (MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
		{
			// copy
			FString PropVal;
			if (FBlueprintEditorUtils::PropertyValueToString(Prop, reinterpret_cast<const uint8*>(Obj), PropVal))
			{
				FPlatformApplicationMisc::ClipboardCopy(*PropVal);

				FNotificationInfo Notification(FText::FromString(FString::Printf(TEXT("Copied to clipboard %s"), *PropVal)));
				FSlateNotificationManager::Get().AddNotification(Notification);
			}
		}
	}

	return FReply::Unhandled();
}

void SFPPropertyText::Construct(const FArguments& InArgs, UObject* InObj, FProperty* InProperty)
{
	Obj = InObj;
	Prop = InProperty;
	FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &SFPPropertyText::HandleObjectPropertyChanged);
	FCoreUObjectDelegates::OnObjectTransacted.AddRaw(this, &SFPPropertyText::HandleObjectTransacted);

	ChildSlot
	[
		SAssignNew(EditableTextBlock, SInlineEditableTextBlock).OnTextCommitted(this, &SFPPropertyText::HandleTextCommitted)
	];

	SyncPropertyText();
}

SFPPropertyText::~SFPPropertyText()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
	FCoreUObjectDelegates::OnObjectTransacted.AddRaw(this, &SFPPropertyText::HandleObjectTransacted);
}

void SFPPropertyText::SyncPropertyText()
{
	FString PropVal;
	if (FBlueprintEditorUtils::PropertyValueToString(Prop, reinterpret_cast<const uint8*>(Obj), PropVal))
	{
		EditableTextBlock->SetText(FText::FromString(PropVal));
	}
	else
	{
		EditableTextBlock->SetText(INVTEXT(""));
	}
}

void SFPPropertyText::HandleTextCommitted(const FText& Text, ETextCommit::Type Arg)
{
	if (Arg == ETextCommit::Type::OnEnter)
	{
		const FString TransactionMsg = FString::Printf(TEXT("Edit %s"), *Prop->GetName());
		const FScopedTransaction Transaction(FText::FromString(TransactionMsg));

		Obj->Modify();

		Obj->PreEditChange(Prop);

		if (FBlueprintEditorUtils::PropertyValueFromString(Prop, Text.ToString(), reinterpret_cast<uint8*>(Obj)))
		{
			FPropertyChangedEvent Event(Prop);
			Obj->PostEditChangeProperty(Event);
		}
	}
}

void SFPPropertyText::HandleObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& ChangeEvent)
{
	if (Obj == Object && ChangeEvent.Property == Prop)
	{
		SyncPropertyText();
	}
}

void SFPPropertyText::HandleObjectTransacted(UObject* Object, const FTransactionObjectEvent& TransactionObjectEvent)
{
	if (Obj == Object && TransactionObjectEvent.GetChangedProperties().Contains(Prop->GetName()))
	{
		SyncPropertyText();
	}
}

FReply SFPPropertyText::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	if (EditableTextBlock->IsHovered())
	{
		EditableTextBlock->EnterEditingMode();
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SFPObjectTableRow::HandleObjPropertyChange(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent)
{
	if (Object != Reference->GetObj())
	{
		return;
	}

	PropertyChangedEvent.Property->GetName();
}

void SFPObjectTableRow::Construct(const FArguments& InArgs, TSharedPtr<FFPObjectData> InReference, const TSharedRef<STableViewBase>& OwnerTable)
{
	Reference = InReference;
	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments(), OwnerTable);
}

TSharedRef<SWidget> SFPObjectTableRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	TSharedPtr<SWidget> ColumnWidget = SNullWidget::NullWidget;

	FSinglePropertyParams Params;
	Params.NamePlacement = EPropertyNamePlacement::Type::Hidden;
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	TSharedRef<SHorizontalBox> HBox = SNew(SHorizontalBox);

	if (ColumnName == TEXT("___FPRowName"))
	{
		ColumnWidget = SAssignNew(InlineEditableText, SInlineEditableTextBlock)
			.IsReadOnly(false)
			.Text(this, &SFPObjectTableRow::GetObjectName)
			.OnTextCommitted(this, &SFPObjectTableRow::HandleRename);
	}
	else
	{
		if (UObject* Obj = Reference->GetObj())
		{
			if (FProperty* Property = FindFProperty<FProperty>(Obj->GetClass(), ColumnName))
			{
				bool bDoesPropertyHaveSupportedClass =
					!Property->IsA(FMapProperty::StaticClass()) &&
					!Property->IsA(FArrayProperty::StaticClass()) &&
					!Property->IsA(FSetProperty::StaticClass());

				const FStructProperty* StructProp = CastField<const FStructProperty>(Property);
				if (StructProp && StructProp->Struct)
				{
					FName StructName = StructProp->Struct->GetFName();
					bDoesPropertyHaveSupportedClass = StructName == NAME_Rotator || 
								 StructName == NAME_Color ||  
								 StructName == NAME_LinearColor || 
								 StructName == NAME_Vector ||
								 StructName == NAME_Quat ||
								 StructName == NAME_Vector4 ||
								 StructName == NAME_Vector2D ||
								 StructName == NAME_IntPoint;

					FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
					if (PropertyEditorModule.IsCustomizedStruct(StructProp->Struct, FCustomPropertyTypeLayoutMap()))
					{
						bDoesPropertyHaveSupportedClass = true;
					}
				}

				if (bDoesPropertyHaveSupportedClass)
				{
					if (TSharedPtr<class ISinglePropertyView> PropWidget = EditModule.CreateSingleProperty(Obj, ColumnName, Params))
					{
						if (PropWidget->HasValidProperty())
						{
							ColumnWidget = SNew(SFPPropertyValueContainer, Obj, Property)
							[
								PropWidget.ToSharedRef()
							];
						}
					}
				}
				else if (Property->ContainsInstancedObjectProperty())
				{
					ColumnWidget = SNew(STextBlock).Text(INVTEXT("Instanced Property")).ColorAndOpacity(FLinearColor::Red);
				}

				if (ColumnWidget == SNullWidget::NullWidget)
				{
					ColumnWidget = SNew(SFPPropertyValueContainer, Obj, Property)
					[
						SNew(SFPPropertyText, Obj, Property)
					];
				}
			}
		}
	}

	return SNew(SBox).Padding(12.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				ColumnWidget.ToSharedRef()
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
			// UE_LOG(LogTemp, Warning, TEXT("Rename %s to %s"), *Path, *NewName);

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
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SFPObjectTableListView::Construct(const FArguments& InArgs, UFPObjectTable* InTableSettings)
{
	ObjectTable = InTableSettings;

	SAssignNew(HeaderRowWidget, SHeaderRow).ResizeMode(ESplitterResizeMode::Fill);

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
	Rows.Empty();
	if (!TableSettings->ClassFilter)
	{
		return;
	}

	HeaderRowWidget->ClearColumns();
	HeaderRowWidget->AddColumn(
		SHeaderRow::Column(FName(TEXT("___FPRowName")))
		.DefaultLabel(INVTEXT("Object Name"))
		.ManualWidth(300.f));


	for (TFieldIterator<FProperty> PropertyIt(TableSettings->ClassFilter, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		if (!TableSettings->CheckedProperties.Contains(PropertyIt->GetName()))
		{
			continue;
		}

		PropertyIt->HasAnyPropertyFlags(CPF_AdvancedDisplay);

		// How to save / load the column widths?
		SHeaderRow::FColumn::FArguments ColumnArgs;
		ColumnArgs.ColumnId(PropertyIt->GetFName());
		ColumnArgs.DefaultLabel(FText::FromString(PropertyIt->GetName()));
		ColumnArgs.ManualWidth(200.0f);
		ColumnArgs.OverflowPolicy(ETextOverflowPolicy::MultilineEllipsis);
		ColumnArgs.HeaderContentPadding(FMargin(8.f, 2.f));
		HeaderRowWidget->AddColumn(ColumnArgs);
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	FString RelativePath = TableSettings->RootDirectory.Path;
	FPaths::MakePathRelativeTo(RelativePath, *FPaths::ProjectDir());

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

	// UE_LOG(LogTemp, Warning, TEXT("%s BP %d DA %d "), *GetNameSafe(TableSettings->ClassFilter), bIsBlueprint, bIsDataAsset);

	// TODO figure out how to disable children which are BP and DA, cause these are likely not what you want to view in the table
	// E.g. UColorAsset -> B_ColorAsset -> DA_AssetBlue (if we want to view all UColorAsset, we don't want to see B_ColorAsset)

	TArray<FAssetData> AssetDataList;
	if (bIsBlueprint && !bIsDataAsset)
	{
		UAssetRegistryHelpers::GetBlueprintAssets(Filter, AssetDataList);
	}
	else
	{
		AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);
	}

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

void SFPObjectTableListView::AddAsset(const FAssetData& AssetData)
{
	// don't add if it's already in the table
	for (int i = Rows.Num() - 1; i >= 0; --i)
	{
		if (Rows[i]->AssetData == AssetData)
		{
			return;
		}
	}

	TSharedPtr<FFPObjectData> NewRow = MakeShared<FFPObjectData>();
	NewRow->AssetData = AssetData;
	Rows.Add(NewRow);

	Rows.StableSort([](TSharedPtr<FFPObjectData> A, TSharedPtr<FFPObjectData> B)
	{
		return A->AssetData.AssetName.ToString() < B->AssetData.AssetName.ToString();
	});

	// UE_LOG(LogTemp, Warning, TEXT("Added %s"), *AssetData.AssetName.ToString());
	RequestListRefresh();
}

void SFPObjectTableListView::RemoveAsset(const FAssetData& AssetData)
{
	for (int i = Rows.Num() - 1; i >= 0; --i)
	{
		if (Rows[i]->AssetData == AssetData)
		{
			// UE_LOG(LogTemp, Warning, TEXT("Removed %s"), *AssetData.AssetName.ToString());
			Rows.RemoveAt(i);
			return;
		}
	}

	RequestListRefresh();
}

void SFPObjectTableListView::RenameAsset(const FAssetData& Asset, const FString& OldPath)
{
	// UE_LOG(LogTemp, Warning, TEXT("Renamed %s PREV %s"), *Asset.AssetName.ToString(), *OldPath);
	for (int i = Rows.Num() - 1; i >= 0; --i)
	{
		if (Rows[i]->AssetData.GetSoftObjectPath().ToString() == OldPath)
		{
			Rows[i]->AssetData = Asset;
			return;
		}
	}

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
	TableSettings->OnClassChanged.AddRaw(this, &SFPObjectTableEditor::HandleClassChanged);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().OnAssetRemoved().AddRaw(this, &SFPObjectTableEditor::HandleAssetRemoved);
	AssetRegistryModule.Get().OnAssetAdded().AddRaw(this, &SFPObjectTableEditor::HandleAssetAdded);
	AssetRegistryModule.Get().OnAssetRenamed().AddRaw(this, &SFPObjectTableEditor::HandleAssetRenamed);

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	NewPropertyTable = PropertyEditorModule.CreatePropertyTable();
	NewPropertyTable->SetIsUserAllowedToChangeRoot(false);
	NewPropertyTable->SetSelectionMode(ESelectionMode::SingleToggle);
	NewPropertyTable->SetSelectionUnit(EPropertyTableSelectionUnit::Cell);
	NewPropertyTable->SetShowObjectName(true);

	FDetailsViewArgs ViewArgs;
	ViewArgs.bAllowSearch = true;
	ViewArgs.bHideSelectionTip = false;
	ViewArgs.NameAreaSettings = FDetailsViewArgs::ObjectsUseNameArea;
	ViewArgs.bShowSectionSelector = false;
	ViewArgs.ViewIdentifier = "FPObjectTable";
	ViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Show;
	ViewArgs.ShouldForceHideProperty = FDetailsViewArgs::FShouldForceHideProperty::CreateRaw(this, &SFPObjectTableEditor::ShouldHideProperty);

	DetailsView = PropertyEditorModule.CreateDetailView(ViewArgs);

	ObjectTable = SNew(SFPObjectTableListView, TableSettings);
	ObjectTable->MyOnSelectionChanged().BindRaw(this, &SFPObjectTableEditor::OnSelectionChanged);
	ObjectTable->Refresh(TableSettings);

	SAssignNew(DetailViewButtons, SFPToggleButtons).OnPropertiesChanged(this, &SFPObjectTableEditor::OnDetailsViewSectionsChanged);
	SAssignNew(PropertyButtons, SFPToggleButtons).OnPropertiesChanged(this, &SFPObjectTableEditor::OnTableColumnsChanged);

	UpdateButtons();

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
					[
						SNew(STextBlock).Text(INVTEXT("Create New"))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.OnClicked(this, &SFPObjectTableEditor::HandleDeleteClicked)
					[
						SNew(STextBlock).Text(INVTEXT("Delete"))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).OnClicked(this, &SFPObjectTableEditor::HandleDuplicateClicked)
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
		TableSettings->OnClassChanged.RemoveAll(this);
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

void SFPObjectTableEditor::OnSelectionChanged(TSharedPtr<FFPObjectData> ObjectData, ESelectInfo::Type SelectInfo)
{
	if (ObjectTable.IsValid() && DetailsView.IsValid())
	{
		DetailsView.Get()->SetObjects(ObjectTable->GetSelectedObjects(), true);
	}
}

FReply SFPObjectTableEditor::HandleNewClassClicked()
{
	// UE_LOG(LogTemp, Warning, TEXT("Create new class %s"), *TableSettings->ClassFilter->GetName());
	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>(FName("AssetTools"));

	TArray<UFactory*> Factories = AssetToolsModule.Get().GetNewAssetFactories();
	UFactory* FoundFactory = nullptr;


	if (TableSettings->ClassFilter->IsChildOf(UDataAsset::StaticClass()))
	{
		auto Factory = NewObject<UDataAssetFactory>();
		Factory->DataAssetClass = TableSettings->ClassFilter;
		FoundFactory = Factory;
	}
	else if (auto BP = UBlueprint::GetBlueprintFromClass(TableSettings->ClassFilter))
	{
		UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
		Factory->ParentClass = BP->GeneratedClass;

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
		CreatedAsset = AssetToolsModule.Get().CreateAsset(UniqueName, TableSettings->RootDirectory.Path, FoundFactory->SupportedClass, FoundFactory);
	}

	if (!CreatedAsset)
	{
		FSlateNotificationManager::Get().AddNotification(INVTEXT("Failed to create asset"));
	}

	return FReply::Handled();
}

FReply SFPObjectTableEditor::HandleDeleteClicked()
{
	if (ObjectTable.IsValid())
	{
		AssetViewUtils::DeleteAssets(ObjectTable->GetSelectedObjects());
	}
	return FReply::Handled();
}

FReply SFPObjectTableEditor::HandleDuplicateClicked()
{
	for (TSharedPtr<FFPObjectData> Item : ObjectTable->GetSelectedItems())
	{
		AssetViewUtils::CopyAssets({Item->AssetData.GetAsset()}, TableSettings->RootDirectory.Path);
		RefreshTable();
	}

	return FReply::Handled();
}

void SFPObjectTableEditor::HandleAssetRenamed(const FAssetData& Asset, const FString& OldPath)
{
	ObjectTable->RenameAsset(Asset, OldPath);
}

void SFPObjectTableEditor::HandleAssetAdded(const FAssetData& Asset)
{
	IAssetRegistry& AssetRegistry = IAssetRegistry::GetChecked();

	FARFilter Filter;
	if (!TableSettings->RootDirectory.Path.IsEmpty())
	{
		Filter.bRecursivePaths = true;
		Filter.PackagePaths.Add(FName(TableSettings->RootDirectory.Path));
	}

	Filter.ClassPaths.Add(TableSettings->ClassFilter->GetClassPathName());
	Filter.bRecursiveClasses = true;

	bool bShouldAdd = false;

	// bool bIsBlueprint = UBlueprint::GetBlueprintFromClass(TableSettings->ClassFilter) != nullptr;
	// bool bIsDataAsset = TableSettings->ClassFilter->IsChildOf(UDataAsset::StaticClass());
	// if (bIsBlueprint && !bIsDataAsset)
	{
		// Expand list of classes to include derived classes
		TArray<FTopLevelAssetPath> BlueprintParentClassPathRoots = MoveTemp(Filter.ClassPaths);
		TSet<FTopLevelAssetPath> BlueprintParentClassPaths;
		if (Filter.bRecursiveClasses)
		{
			AssetRegistry.GetDerivedClassNames(
				BlueprintParentClassPathRoots,
				TSet<FTopLevelAssetPath>(),
				BlueprintParentClassPaths);
		}
		else
		{
			BlueprintParentClassPaths.Append(BlueprintParentClassPathRoots);
		}

		// Search for all blueprints and then check BlueprintParentClassPaths in the results
		Filter.ClassPaths.Reset(1);
		Filter.ClassPaths.Add(FTopLevelAssetPath(FName(TEXT("/Script/Engine")), FName(TEXT("BlueprintCore"))));
		Filter.bRecursiveClasses = true;

		// Verify blueprint class
		if (BlueprintParentClassPaths.IsEmpty() || UAssetRegistryHelpers::IsAssetDataBlueprintOfClassSet(Asset, BlueprintParentClassPaths))
		{
			bShouldAdd = true;
		}
	}
	// else
	// {
	// 	FARCompiledFilter CompiledFilter;
	// 	AssetRegistry.CompileFilter(Filter, CompiledFilter);
	//
	// 	if (AssetRegistry.IsAssetIncludedByFilter(Asset, CompiledFilter))
	// 	{
	// 		bShouldAdd = true;
	// 	}
	// }

	if (bShouldAdd)
	{
		ObjectTable->AddAsset(Asset);
	}
}

void SFPObjectTableEditor::HandleAssetRemoved(const FAssetData& Asset)
{
	ObjectTable->RemoveAsset(Asset);
}

void SFPObjectTableEditor::RefreshTable()
{
	ObjectTable->Refresh(TableSettings);
	UpdateButtons();
}

void SFPObjectTableEditor::HandleClassChanged()
{
	// clear the table properties
	TableSettings->CheckedProperties.Empty();

	// enable all details view section
	TSet<FString> PropCategories;
	for (TFieldIterator<FProperty> It(TableSettings->ClassFilter.Get()); It; ++It)
	{
		FProperty* Property = *It;
		if (auto CategoryName = ObjectTableUtils::GetPropertyCategory(Property))
		{
			PropCategories.Add(*CategoryName);
		}
	}

	TableSettings->DetailsViewSections = PropCategories.Array();

	UpdateButtons();
}

void SFPObjectTableEditor::UpdateButtons()
{
	if (!TableSettings->ClassFilter)
	{
		return;
	}

	TSet<FString> PropNames;
	TSet<FString> PropCategories;

	for (TFieldIterator<FProperty> It(TableSettings->ClassFilter.Get()); It; ++It)
	{
		FProperty* Property = *It;
		// UE_LOG(LogTemp, Warning, TEXT("%s | %s "), *Property->GetName(), *Property->GetClass()->GetName())

		// see SObjectMixerEditorList::AddUniquePropertyColumnInfo
		const bool bIsPropertyBlueprintEditable = (Property->GetPropertyFlags() & CPF_Edit) != 0 && Property->HasAllFlags(RF_Public);
		if (bIsPropertyBlueprintEditable)
		{
			PropNames.Add(Property->GetName());
			// UE_LOG(LogTemp, Warning, TEXT("Property %s | %s"), *Property->GetName(), *Property->GetClass()->GetName());
		}

		if (auto CategoryName = ObjectTableUtils::GetPropertyCategory(Property))
		{
			PropCategories.Add(*CategoryName);
		}
	}

	if (DetailViewButtons)
	{
		DetailViewButtons->SetProperties(PropCategories.Array());
		DetailViewButtons->CheckedProperties = TableSettings->DetailsViewSections;
	}

	if (PropertyButtons)
	{
		PropertyButtons->SetProperties(PropNames.Array());
		PropertyButtons->CheckedProperties = TableSettings->CheckedProperties;
	}
}

bool SFPObjectTableEditor::ShouldHideProperty(const TSharedRef<FPropertyNode>& Property)
{
	// if (!TableSettings->DetailsViewSections.Num())
	// {
	// 	return false;
	// }

	if (!Property->GetProperty())
	{
		return false;
	}

	if (Property->GetParentNode() && !Property->GetParentNode()->AsCategoryNode())
	{
		return false;
	}

	if (auto Category = ObjectTableUtils::GetPropertyCategory(Property->GetProperty()))
	{
		return !TableSettings->DetailsViewSections.Contains(*Category);
	}

	return true;
}

TOptional<FString> ObjectTableUtils::GetPropertyCategory(FProperty* Property)
{
	if (const FString* CategoryNamesMetaData = Property->FindMetaData("Category"))
	{
		TArray<FString> CategoryNames;
		CategoryNamesMetaData->ParseIntoArray(CategoryNames, TEXT(","), true);
		if (CategoryNames.Num())
		{
			return CategoryNames[0];
		}
	}

	return TOptional<FString>();
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

			// TODO sometimes the asset doesn't get selected when doing this, maybe move this to a mouse hotkey?
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

	if (InKeyEvent.GetKey() == EKeys::Delete)
	{
		AssetViewUtils::DeleteAssets(ObjectTable->GetSelectedObjects());
	}

	return FReply::Unhandled();
}

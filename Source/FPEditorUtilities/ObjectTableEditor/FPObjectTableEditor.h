﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTable.h"
#include "FPObjectTable.h"
#include "Filters/SBasicFilterBar.h"
#include "Widgets/Layout/SWrapBox.h"

class SWrapBox;

class FFPObjectData : public TSharedFromThis<FFPObjectData>
{
public:
	FAssetData AssetData;

	UObject* GetObj();
};

class SFPPropertyValueContainer : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFPPropertyValueContainer)
	{
	}
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UObject* InObj, FProperty* InProperty);
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	UObject* Obj = nullptr;
	FProperty* Prop = nullptr;
};

class SFPPropertyText : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFPPropertyText)
	{
	}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UObject* Obj, FProperty* Property);
	virtual ~SFPPropertyText() override;

	void SyncPropertyText();

	void HandleTextCommitted(const FText& Text, ETextCommit::Type Arg);

	void HandleObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& ChangeEvent);
	void HandleObjectTransacted(UObject* Object, const FTransactionObjectEvent& TransactionObjectEvent);
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;

	TSharedPtr<SInlineEditableTextBlock> EditableTextBlock;
	UObject* Obj = nullptr;
	FProperty* Prop = nullptr;
};

class SFPObjectTableRow : public SMultiColumnTableRow<TSharedPtr<FFPObjectData>>
{
public:
	SLATE_BEGIN_ARGS(SFPObjectTableRow)
		{
		}

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, TSharedPtr<FFPObjectData> InReference, const TSharedRef<STableViewBase>& OwnerTable);

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

	void HandleRename(const FText& Text, ETextCommit::Type CommitMethod);

	FText GetObjectName() const;

	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;

	void HandleObjPropertyChange(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);

	TSharedPtr<SInlineEditableTextBlock> InlineEditableText;
	TSharedPtr<FFPObjectData> Reference;
};

class SFPObjectTableListView : public SListView<TSharedPtr<FFPObjectData>>
{
	SLATE_BEGIN_ARGS(SFPObjectTableListView)
		{
		}
	
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, UFPObjectTable* InTableSettings);

	void Refresh(UFPObjectTable* TableSettings);
	void AddAsset(const FAssetData& AssetData);
	void RemoveAsset(const FAssetData& AssetData);
	void RenameAsset(const FAssetData& Asset, const FString& NewName);

	TArray<UObject*> GetSelectedObjects();
	TArray<FAssetData> GetSelectedAssets();

	FOnSelectionChanged& MyOnSelectionChanged() { return OnSelectionChanged; }

protected:
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FFPObjectData> InDisplayNode, const TSharedRef<STableViewBase>& OwnerTable);

private:
	TSharedPtr<SHeaderRow> HeaderRowWidget;
	TArray<TSharedPtr<FFPObjectData>> Rows;

	UFPObjectTable* ObjectTable = nullptr;
};


class SFPToggleButtons : public SWrapBox
{
	DECLARE_DELEGATE_OneParam(FFPOnPropertiesChanged, const TArray<FString>&);

	SLATE_BEGIN_ARGS(SFPToggleButtons)
	{
	}
		SLATE_ARGUMENT(TArray<FString>, Properties)
		SLATE_ARGUMENT(TArray<FString>, CheckedProperties)
		SLATE_ARGUMENT(bool, InvertSelection)
		SLATE_EVENT(FFPOnPropertiesChanged, OnPropertiesChanged)
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

	void SetProperties(const TArray<FString>& InProperties);

	void OnPropertyCheckedChanged(ECheckBoxState CheckBoxState, FString PropertyName);
	ECheckBoxState IsPropertyChecked(FString String) const;

	void OnAllChecked(ECheckBoxState CheckBoxState);
	ECheckBoxState IsAllChecked() const;

	FFPOnPropertiesChanged OnPropertiesChanged;

	TArray<FString> AllProperties;
	TArray<FString> CheckedProperties;

	bool bInvertSelection = false;
};


class SFPObjectTableEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFPObjectTableEditor)
		{
		}

		SLATE_ARGUMENT(UFPObjectTable*, Settings)
	SLATE_END_ARGS()

	void OnTableColumnsChanged(const TArray<FString>& Properties);
	void OnDetailsViewSectionsChanged(const TArray<FString>& Properties);
	void Construct(const FArguments& InArgs);

	virtual ~SFPObjectTableEditor() override;

	static TSharedRef<SDockTab> CreateTab(const FSpawnTabArgs& Args);

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	TSharedPtr<IPropertyTable> NewPropertyTable;
	TSharedPtr<SFPObjectTableListView> ObjectTable;
	TSharedPtr<IDetailsView> DetailsView;

	TSharedPtr<SFPToggleButtons> DetailViewButtons;
	TSharedPtr<SFPToggleButtons> PropertyButtons;

	UFPObjectTable* TableSettings;

	TSharedPtr<SBasicFilterBar<FText>> FilterBar;

	void OnSelectionChanged(TSharedPtr<FFPObjectData> ObjectData, ESelectInfo::Type SelectInfo);

	FReply HandleNewClassClicked();
	FReply HandleDeleteClicked();
	FReply HandleDuplicateClicked();

	void HandleAssetRenamed(const FAssetData& Asset, const FString& OldPath);
	void HandleAssetAdded(const FAssetData& Asset);
	void HandleAssetRemoved(const FAssetData& Asset);

	void RefreshTable();

	void HandleClassChanged();
	void UpdateButtons();

	bool ShouldHideProperty(const TSharedRef<FPropertyNode>& Property);
};

namespace ObjectTableUtils
{
	static TOptional<FString> GetPropertyCategory(FProperty* Property);
}

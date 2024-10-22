// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTable.h"
#include "FPObjectTable.h"

class FFPObjectData : public TSharedFromThis<FFPObjectData>
{
public:
	TWeakObjectPtr<UObject> Object;
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

private:
	TSharedPtr<FFPObjectData> Reference;
};

class SFPObjectTableListView : public SListView<TSharedPtr<FFPObjectData>>
{
	SLATE_BEGIN_ARGS(SFPObjectTableListView)
		{
		}

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

	void Refresh(UFPObjectTable* TableSettings);

	TArray<UObject*> GetSelectedObjects();

	FOnSelectionChanged& MyOnSelectionChanged() { return OnSelectionChanged; }

protected:
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FFPObjectData> InDisplayNode, const TSharedRef<STableViewBase>& OwnerTable);

private:
	TSharedPtr<SHeaderRow> HeaderRowWidget;
	TArray<TSharedPtr<FFPObjectData>> Rows;
};


class SFPObjectTableEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFPObjectTableEditor)
		{
		}

		SLATE_ARGUMENT(UFPObjectTable*, Settings)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual ~SFPObjectTableEditor() override;

	static TSharedRef<SDockTab> CreateTab(const FSpawnTabArgs& Args);

	TSharedPtr<SVerticalBox> Rows;
	void OnPathSet(const FString& Path);

	TSharedPtr<IPropertyTable> NewPropertyTable;
	TSharedPtr<SFPObjectTableListView> ObjectTable;
	TSharedPtr<IDetailsView> DetailsView;

	UFPObjectTable* TableSettings;

	void OnSelectionChanged(TSharedPtr<FFPObjectData> ObjectData, ESelectInfo::Type SelectInfo);

	void RefreshTable();
};

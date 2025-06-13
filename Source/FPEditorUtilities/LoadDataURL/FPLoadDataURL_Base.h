// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Toolkits/IToolkitHost.h"

DECLARE_DELEGATE_OneParam(FFPOnURLEntered, FString);

struct SFPURLEntry : SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFPURLEntry) {}
		SLATE_ARGUMENT(FString, DefaultURL)
		SLATE_ARGUMENT(FFPOnURLEntered, OnUrlEntered)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FFPOnURLEntered OnURLEntered;
	TSharedPtr<SEditableTextBox> EditableText;
};

class FFPLoadDataURL_Base
{
public:
	void Init();
	void ImportFromGoogleSheets(TWeakObjectPtr<UObject> Object, FString GoogleSheetsId);

protected:
	~FFPLoadDataURL_Base() = default;

	virtual void ReceiveResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, TWeakObjectPtr<UObject> Object);
	virtual void ReceiveCSV(FString String, TWeakObjectPtr<UObject> Object) = 0;
	virtual void SetValidClasses(FName& OutAssetClassName, FName& OutAssetEditorName) = 0;

private:
	FName ValidAssetName;
	FName ValidAssetEditorName;
	TSharedPtr<SNotificationItem> OngoingNotif;

	// make toolbar button
	void OnAssetOpenedInEditor(UObject* Asset, IAssetEditorInstance* AssetEditor);
	void ExtendToolbar(FToolBarBuilder& ToolbarBuilder, TWeakObjectPtr<UObject> Object);

	// make asset context menu item  
	TSharedRef<FExtender> MakeContextMenuExtender(const TArray<FAssetData>& AssetDataList);
	void AddMenuEntry(FMenuBuilder& MenuBuilder, TWeakObjectPtr<UObject> Object);

	void HandleURLEntered(FString URL, TWeakObjectPtr<UObject> Object);

	TSharedRef<SWidget> MakeURLEntry(TWeakObjectPtr<UObject> Object);

	void OpenWindow(TWeakObjectPtr<UObject> Object);
};

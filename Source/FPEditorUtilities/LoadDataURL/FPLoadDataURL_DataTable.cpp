#include "FPLoadDataURL_DataTable.h"

#include "DataTableEditorUtils.h"
#include "Misc/LazySingleton.h"

FFPLoadDataURL_DataTable& FFPLoadDataURL_DataTable::Get()
{
	return TLazySingleton<FFPLoadDataURL_DataTable>::Get();
}

void FFPLoadDataURL_DataTable::TearDown()
{
	return TLazySingleton<FFPLoadDataURL_DataTable>::TearDown();
}

void FFPLoadDataURL_DataTable::ReceiveCSV(FString CSV, TWeakObjectPtr<UObject> Object)
{
	if (!Object.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to import datatable: nullptr"));
		return;
	}

	UDataTable* DataTable = Cast<UDataTable>(Object.Get());
	if (!DataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed import data table: cast failed"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Received CSV"));
	DataTable->CreateTableFromCSVString(CSV);
	FDataTableEditorUtils::BroadcastPostChange(DataTable, FDataTableEditorUtils::EDataTableChangeInfo::RowList);
	GEditor->RedrawAllViewports();
}

void FFPLoadDataURL_DataTable::SetValidClasses(FName& OutAssetClassName, FName& OutAssetEditorName)
{
	OutAssetClassName = UDataTable::StaticClass()->GetFName();
	OutAssetEditorName = FName("DataTableEditor");
}

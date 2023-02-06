#include "FPLoadDataURL_CurveTable.h"

#include "CurveTableEditorUtils.h"
#include "Misc/LazySingleton.h"

FFPLoadDataURL_CurveTable& FFPLoadDataURL_CurveTable::Get()
{
	return TLazySingleton<FFPLoadDataURL_CurveTable>::Get();
}

void FFPLoadDataURL_CurveTable::TearDown()
{
	return TLazySingleton<FFPLoadDataURL_CurveTable>::TearDown();
}

void FFPLoadDataURL_CurveTable::ReceiveCSV(FString CSV, TWeakObjectPtr<UObject> Object)
{
	if (!Object.IsValid())
	{
		return;
	}

	UCurveTable* CurveTable = Cast<UCurveTable>(Object.Get());
	if (!CurveTable)
	{
		return;
	}

	CurveTable->CreateTableFromCSVString(CSV);
	FCurveTableEditorUtils::BroadcastPostChange(CurveTable, FCurveTableEditorUtils::ECurveTableChangeInfo::RowList);
	GEditor->RedrawAllViewports();
	UE_LOG(LogTemp, Warning, TEXT("Did stuff"));
}

void FFPLoadDataURL_CurveTable::SetValidClasses(FName& OutAssetClassName, FName& OutAssetEditorName)
{
	OutAssetClassName = UCurveTable::StaticClass()->GetFName();
	OutAssetEditorName = FName("CurveTableEditor");
}

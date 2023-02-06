#pragma once

#include "FPLoadDataURL_Base.h"

class FFPLoadDataURL_DataTable final : public FFPLoadDataURL_Base
{
public:
	virtual ~FFPLoadDataURL_DataTable() = default;
	static FFPLoadDataURL_DataTable& Get();
	static void TearDown();

protected:
	virtual void ReceiveCSV(FString CSV, TWeakObjectPtr<UObject> Object) override;
	virtual void SetValidClasses(FName& OutAssetClassName, FName& OutAssetEditorName) override;
};

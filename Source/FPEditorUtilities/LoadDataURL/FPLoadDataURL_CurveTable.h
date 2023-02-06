#pragma once

#include "FPLoadDataURL_Base.h"

class FFPLoadDataURL_CurveTable final : public FFPLoadDataURL_Base
{
public:
	virtual ~FFPLoadDataURL_CurveTable() = default;
	static FFPLoadDataURL_CurveTable& Get();
	static void TearDown();

protected:
	virtual void ReceiveCSV(FString CSV, TWeakObjectPtr<UObject> Object) override;
	virtual void SetValidClasses(FName& OutAssetClassName, FName& OutAssetEditorName) override;
};

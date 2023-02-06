#include "FPEditorUtilities.h"

#include "LoadDataURL/FPLoadDataURL_CurveTable.h"
#include "LoadDataURL/FPLoadDataURL_DataTable.h"

#define LOCTEXT_NAMESPACE "FFPEditorUtilitiesModule"

void FFPEditorUtilitiesModule::StartupModule()
{
#if WITH_EDITOR
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FFPEditorUtilitiesModule::OnPostEngineInit);
#endif
}

void FFPEditorUtilitiesModule::ShutdownModule()
{
}

void FFPEditorUtilitiesModule::OnPostEngineInit()
{
	FFPLoadDataURL_CurveTable::Get().Init();
	FFPLoadDataURL_DataTable::Get().Init();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFPEditorUtilitiesModule, FPEditorUtilities)

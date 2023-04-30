// See https://github.com/riperjack/ue4_googledoc2datatable

#include "FPGetGoogleSheet.h"

#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

const FString UFPGetGoogleSheets::ApiBaseUrl = "https://docs.google.com/spreadsheets/d/";

TSharedRef<IHttpRequest> UFPGetGoogleSheets::RequestWithRoute(FString Subroute)
{
	Http = &FHttpModule::Get();
	TSharedRef<IHttpRequest> Request = Http->CreateRequest();
	// Request->SetURL(UFPGetGoogleSheets::ApiBaseUrl + Subroute);
	Request->SetURL(Subroute);
	SetRequestHeaders(Request);
	return Request;
}

void UFPGetGoogleSheets::SetRequestHeaders(TSharedRef<IHttpRequest>& Request)
{
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("text/csv"));
	Request->SetHeader(TEXT("Accepts"), TEXT("text/csv"));
}

TSharedRef<IHttpRequest> UFPGetGoogleSheets::GetRequest(FString Subroute)
{
	TSharedRef<IHttpRequest> Request = RequestWithRoute(Subroute);
	Request->SetVerb("GET");
	return Request;
}

TSharedRef<IHttpRequest> UFPGetGoogleSheets::PostRequest(FString Subroute, FString ContentJsonString)
{
	TSharedRef<IHttpRequest> Request = RequestWithRoute(Subroute);
	Request->SetVerb("POST");
	Request->SetContentAsString(ContentJsonString);
	return Request;
}

bool UFPGetGoogleSheets::ResponseIsValid(FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid()) return false;
	if (EHttpResponseCodes::IsOk(Response->GetResponseCode())) return true;
	else
	{
		FSlateNotificationManager::Get().AddNotification(FNotificationInfo(INVTEXT("Failed to load URL")));
		UE_LOG(LogTemp, Error, TEXT("Http Response returned error code: %d"), Response->GetResponseCode());
		return false;
	}
}

void UFPGetGoogleSheets::ProcessResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!ResponseIsValid(Response, bWasSuccessful)) return;

	OnResponseDelegate.ExecuteIfBound(Response->GetContentAsString());
}

void UFPGetGoogleSheets::SendRequest(FString DocId)
{
	// TSharedRef<IHttpRequest> Request = GetRequest(FString::Printf(TEXT("%s/export?format=csv"), *DocId));
	TSharedRef<IHttpRequest> Request = GetRequest(*DocId);
	Request->OnProcessRequestComplete().BindUObject(this, &UFPGetGoogleSheets::ProcessResponse);
	Request->ProcessRequest();
}

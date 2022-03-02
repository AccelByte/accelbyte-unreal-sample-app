#include "AccelByteSampleBlueprints.h"

#include "Misc/DateTime.h"
#include "Misc/ConfigCacheIni.h"

#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlinePurchaseInterface.h"

#include "Core/AccelByteRegistry.h"
#include "Api/AccelByteUserApi.h"
#include "Api/AccelByteEntitlementApi.h"
#include "Api/AccelByteItemApi.h"

#define STEAM_LOGIN_DELAY 2 // seconds

DECLARE_LOG_CATEGORY_EXTERN(LogAccelByteSampleApp, Log, All);
DEFINE_LOG_CATEGORY(LogAccelByteSampleApp);

#define DEFAULT_SUBSYSTEM_NAME TEXT("DefaultPlatformService")
#define NATIVE_SUBSYSTEM_NAME TEXT("NativePlatformService")

static FString DefaultSubsystemName;
static FString NativeSubsystemName;

UAccelByteLoginNativePlatform::UAccelByteLoginNativePlatform(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, WorldContextObject(nullptr)
{
}

UAccelByteLoginNativePlatform* UAccelByteLoginNativePlatform::LoginWithNativePlatform
	( UObject* WorldContextObject
	, APlayerController* InPlayerController )
{
	UAccelByteLoginNativePlatform* Proxy = NewObject<UAccelByteLoginNativePlatform>();
	Proxy->PlayerControllerWeakPtr = InPlayerController;
	Proxy->WorldContextObject = WorldContextObject;
	return Proxy;
}

void UAccelByteLoginNativePlatform::Activate()
{
	APlayerController* MyPlayerController = PlayerControllerWeakPtr.Get();
	if (!MyPlayerController)
	{
		UE_LOG(LogAccelByteSampleApp, Warning, TEXT("A player controller must be provided in order to do login with native platform."));
		OnFailure.Broadcast(MyPlayerController, static_cast<int32>(AccelByte::ErrorCodes::UnknownError), TEXT("player-controller-not-provided"));
		return;
	}
	
	const IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get(FName(NativeSubsystemName));
	if (OnlineSubsystem == nullptr)
	{
		UE_LOG(LogAccelByteSampleApp, Warning, TEXT("Cannot login with no online subsystem set!"));
		OnFailure.Broadcast(MyPlayerController, static_cast<int32>(AccelByte::ErrorCodes::UnknownError), TEXT("login-failed-native-subsystem-null"));
		return;
	}

	const IOnlineIdentityPtr OnlineIdentity = OnlineSubsystem->GetIdentityInterface();
	if (!OnlineIdentity.IsValid())
	{
		UE_LOG(LogAccelByteSampleApp, Warning, TEXT("Could not retrieve identity interface from native subsystem."));
		OnFailure.Broadcast(MyPlayerController, static_cast<int32>(AccelByte::ErrorCodes::UnknownError), TEXT("login-failed-native-identity-null"));
		return;
	}

	const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(MyPlayerController->Player);
	if (LocalPlayer == nullptr)
	{
		UE_LOG(LogAccelByteSampleApp, Warning, TEXT("Can only login with native platform for local players"));
		OnFailure.Broadcast(MyPlayerController, static_cast<int32>(AccelByte::ErrorCodes::UnknownError), TEXT("local-player-not-found"));
		return;
	}

	const ELoginStatus::Type LoginStatus = OnlineIdentity->GetLoginStatus(LocalPlayer->GetControllerId());
	if (LoginStatus == ELoginStatus::LoggedIn)
	{
		OnSuccess.Broadcast(MyPlayerController, 0, TEXT(""));
		return;
	}
	
	const FOnLoginCompleteDelegate NativeLoginComplete = FOnLoginCompleteDelegate::CreateUObject(this, &UAccelByteLoginNativePlatform::OnLoginNativePlatformCompleted);
	OnlineIdentity->AddOnLoginCompleteDelegate_Handle(LocalPlayer->GetControllerId(), NativeLoginComplete);
	const bool bWaitForDelegate = OnlineIdentity->Login(LocalPlayer->GetControllerId(), FOnlineAccountCredentials());

	if (!bWaitForDelegate)
	{
		UE_LOG(LogAccelByteSampleApp, Display, TEXT("The online subsystem couldn't login"));
		OnFailure.Broadcast(MyPlayerController, static_cast<int32>(AccelByte::ErrorCodes::UnknownError), TEXT(""));
	}

	UE_LOG(LogAccelByteSampleApp, Display, TEXT("Sending login request to native subsystem!"));
}

void UAccelByteLoginNativePlatform::OnLoginNativePlatformCompleted
	( int32 NativeLocalUserNum
	, bool bWasNativeLoginSuccessful
	, FUniqueNetId const& NativeUserId
	, FString const& NativeError )
{
	// Update the cached unique ID for the local player and the player state.
	APlayerController* MyPlayerController = PlayerControllerWeakPtr.Get();

	if (MyPlayerController != nullptr)
	{
		ULocalPlayer* LocalPlayer = MyPlayerController->GetLocalPlayer();
		if (LocalPlayer != nullptr)
		{
			LocalPlayer->SetCachedUniqueNetId(MakeShareable(&NativeUserId));
		}

		if (MyPlayerController->PlayerState != nullptr)
		{
			MyPlayerController->PlayerState->SetUniqueId(MakeShareable(&NativeUserId));
		}
	}

	const IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get(FName(NativeSubsystemName));
	if (OnlineSubsystem == nullptr)
	{
		UE_LOG(LogAccelByteSampleApp, Warning, TEXT("Cannot login with native subsystem as none was set!"));
		OnFailure.Broadcast(MyPlayerController, static_cast<int32>(AccelByte::ErrorCodes::UnknownError), TEXT("login-failed-native-subsystem-null"));
		return;
	}

	const IOnlineIdentityPtr OnlineIdentity = OnlineSubsystem->GetIdentityInterface();
	if (!OnlineIdentity.IsValid())
	{
		UE_LOG(LogAccelByteSampleApp, Warning, TEXT("Could not retrieve identity interface from native subsystem."));
		OnFailure.Broadcast(MyPlayerController, static_cast<int32>(AccelByte::ErrorCodes::UnknownError), TEXT("login-failed-native-identity-null"));
		return;
	}
	
	// Clear the delegate for our login as it will be invalid once this task ends
	OnlineIdentity->ClearOnLoginCompleteDelegates(NativeLocalUserNum, this);

	// Set the login type for this request to be the login type corresponding to the native subsystem
	const EAccelBytePlatformType PlatformType = UAccelByteBluePrintsSample::GetPlatformTypeFromSubsystem(OnlineSubsystem->GetSubsystemName().ToString());

	if (bWasNativeLoginSuccessful)
	{
		if (PlatformType == EAccelBytePlatformType::Steam)
		{
			int64 TriggerTime = FDateTime::UtcNow().ToUnixTimestamp() + STEAM_LOGIN_DELAY;
			FTicker::GetCoreTicker().AddTicker(
				FTickerDelegate::CreateLambda([TriggerTime, MyPlayerController, this](float Time)
				{
					if (TriggerTime > FDateTime::UtcNow().ToUnixTimestamp())
					{
						return true;
					}
					OnSuccess.Broadcast(MyPlayerController, 0, TEXT(""));

					return false;
				}),
				0.2f);
		}
		else
		{
			OnSuccess.Broadcast(MyPlayerController, 0, TEXT(""));
		}
	}
	else
	{
		OnFailure.Broadcast(MyPlayerController, static_cast<int32>(AccelByte::ErrorCodes::UnknownError), NativeError);
	}
}

UAccelByteLogin::UAccelByteLogin(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, WorldContextObject(nullptr)
{
}

UAccelByteLogin* UAccelByteLogin::LoginWithAccelByte
	( UObject* WorldContextObject
	, APlayerController* InPlayerController )
{
	UAccelByteLogin* Proxy = NewObject<UAccelByteLogin>();
	Proxy->PlayerControllerWeakPtr = InPlayerController;
	Proxy->WorldContextObject = WorldContextObject;
	return Proxy;
}

void UAccelByteLogin::Activate()
{
	APlayerController* MyPlayerController = PlayerControllerWeakPtr.Get();
	if (!MyPlayerController)
	{
		UE_LOG(LogAccelByteSampleApp, Warning, TEXT("A player controller must be provided in order to do login with nataccelbyte."));
		OnFailure.Broadcast(MyPlayerController, static_cast<int32>(AccelByte::ErrorCodes::UnknownError), TEXT("player-controller-not-provided"));
		return;
	}
	
	const IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get(FName(NativeSubsystemName));
	if (OnlineSubsystem == nullptr)
	{
		UE_LOG(LogAccelByteSampleApp, Warning, TEXT("Cannot login with no online subsystem set!"));
		OnFailure.Broadcast(MyPlayerController, static_cast<int32>(AccelByte::ErrorCodes::UnknownError), TEXT("login-failed-native-subsystem-null"));
		return;
	}

	const IOnlineIdentityPtr OnlineIdentity = OnlineSubsystem->GetIdentityInterface();
	if (!OnlineIdentity.IsValid())
	{
		UE_LOG(LogAccelByteSampleApp, Warning, TEXT("Could not retrieve identity interface from native subsystem."));
		OnFailure.Broadcast(MyPlayerController, static_cast<int32>(AccelByte::ErrorCodes::UnknownError), TEXT("login-failed-native-identity-null"));
		return;
	}

	const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(MyPlayerController->Player);
	if (LocalPlayer == nullptr)
	{
		UE_LOG(LogAccelByteSampleApp, Warning, TEXT("Can only login with native platform for local players"));
		OnFailure.Broadcast(MyPlayerController, static_cast<int32>(AccelByte::ErrorCodes::UnknownError), TEXT("local-player-not-found"));
		return;
	}

	// Set the login type for this request to be the login type corresponding to the native subsystem
	const EAccelBytePlatformType PlatformType = UAccelByteBluePrintsSample::GetPlatformTypeFromSubsystem(OnlineSubsystem->GetSubsystemName().ToString());
	const FString PlatformToken = OnlineIdentity->GetAuthToken(LocalPlayer->GetControllerId());
	
	FSimpleDelegate OnLoginSuccessDelegate = FSimpleDelegate::CreateUObject(this, &UAccelByteLogin::OnLoginAccelByteCompleted);
	AccelByte::FErrorHandler OnLoginErrorDelegate = AccelByte::FErrorHandler::CreateUObject(this, &UAccelByteLogin::OnLoginAccelByteFailed);
	FRegistry::User.LoginWithOtherPlatform(PlatformType, PlatformToken, OnLoginSuccessDelegate, OnLoginErrorDelegate);

	UE_LOG(LogAccelByteSampleApp, Display, TEXT("Sending login request to AccelByte service!"));
}

void UAccelByteLogin::OnLoginAccelByteCompleted()
{
	APlayerController* MyPlayerController = PlayerControllerWeakPtr.Get();
	
	UE_LOG(LogAccelByteSampleApp, Display, TEXT("Successfully Login to AccelByte service!"));
	OnSuccess.Broadcast(MyPlayerController, 0, TEXT(""));
}

void UAccelByteLogin::OnLoginAccelByteFailed(int32 ErrorCode, FString const& ErrorMessage)
{
	APlayerController* MyPlayerController = PlayerControllerWeakPtr.Get();
	
	UE_LOG(LogAccelByteSampleApp, Warning, TEXT("Failed Login to AccelByte service! code: %d - message: %s"), ErrorCode, *ErrorMessage);
	OnFailure.Broadcast(MyPlayerController, ErrorCode, ErrorMessage);
}

void UAccelByteBluePrintsSample::LoadConfig()
{
	static FString ConfigSection(TEXT("OnlineSubsystem"));
	if (!GConfig->GetString(*ConfigSection, DEFAULT_SUBSYSTEM_NAME, DefaultSubsystemName, GEngineIni))
	{
		UE_LOG(LogAccelByteSampleApp, Warning, TEXT("Missing DefaultPlatformService = in [%s] of DefaultEngine.ini"), *ConfigSection);
	}
	else
	{
		UE_LOG(LogAccelByteSampleApp, Display, TEXT("Retrieved DefaultPlatformService = %s in [%s] of DefaultEngine.ini"), *DefaultSubsystemName, *ConfigSection);
	}

	if (!GConfig->GetString(*ConfigSection, NATIVE_SUBSYSTEM_NAME, NativeSubsystemName, GEngineIni))
	{
		UE_LOG(LogAccelByteSampleApp, Warning, TEXT("Missing NativePlatformService = in [%s] of DefaultEngine.ini"), *ConfigSection);
	}
	else
	{
		UE_LOG(LogAccelByteSampleApp, Display, TEXT("Retrieved NativePlatformService = %s in [%s] of DefaultEngine.ini"), *NativeSubsystemName, *ConfigSection);
	}
}
EAccelBytePlatformType UAccelByteBluePrintsSample::GetPlatformTypeFromSubsystem(FString const& SubsystemName)
{
	if (SubsystemName.Equals(TEXT("GDK"), ESearchCase::IgnoreCase) || SubsystemName.Equals(TEXT("Live"), ESearchCase::IgnoreCase))
	{
		return EAccelBytePlatformType::Live;
	}
	else if (SubsystemName.Equals(TEXT("PS4"), ESearchCase::IgnoreCase))
	{
		return EAccelBytePlatformType::PS4CrossGen;
	}
	else if (SubsystemName.Equals(TEXT("PS5"), ESearchCase::IgnoreCase))
	{
		return EAccelBytePlatformType::PS5;
	}
	else if (SubsystemName.Equals(TEXT("STEAM"), ESearchCase::IgnoreCase))
	{
		return EAccelBytePlatformType::Steam;
	}
	else if (SubsystemName.Equals(TEXT("GOOGLEPLAY"), ESearchCase::IgnoreCase) || SubsystemName.Equals(TEXT("GOOGLE"), ESearchCase::IgnoreCase))
	{
		return EAccelBytePlatformType::Google;
	}
	else if (SubsystemName.Equals(TEXT("IOS"), ESearchCase::IgnoreCase) || SubsystemName.Equals(TEXT("APPLE"), ESearchCase::IgnoreCase))
	{
		return EAccelBytePlatformType::Apple;
	}
	
	return EAccelBytePlatformType::Device;
}

EAccelBytePlatformType UAccelByteBluePrintsSample::GetNativePlatformType()
{
	return GetPlatformTypeFromSubsystem(NativeSubsystemName);
}

void UAccelByteBluePrintsSample::GetItemBySku
	( FString const& Sku
	, FDAccelByteModelsItemInfo const& OnSuccess
	, FDErrorHandler const& OnError )
{
	const THandler<FAccelByteModelsItemInfo> OnGetItemBySkuSuccessDelegate = THandler<FAccelByteModelsItemInfo>::CreateLambda([OnSuccess](const FAccelByteModelsItemInfo& Respose)
	{
		UE_LOG(LogAccelByteSampleApp, Display, TEXT("AccelByte get item by SKU succeed!"));
		OnSuccess.ExecuteIfBound(Respose);
	});

	AccelByte::FErrorHandler OnSynErrorDelegate = AccelByte::FErrorHandler::CreateLambda([OnError]
		( int32 ErrorCode
		, FString const& ErrorMessage )
		{
			UE_LOG(LogAccelByteSampleApp, Warning, TEXT("AccelByte Get Item by SKU failed! code: %d - message: %s"), ErrorCode, *ErrorMessage);
			OnError.ExecuteIfBound(ErrorCode, ErrorMessage);
		});
	
	FRegistry::Item.GetItemBySku(Sku, "", "", OnGetItemBySkuSuccessDelegate, OnSynErrorDelegate);
}

void UAccelByteBluePrintsSample::FinalizePurchase
	( APlayerController* InPlayerController
	, FString const& ReceiptId
	, FDHandler const& OnSuccess
	, FDErrorHandler const& OnError )
{
	if (!InPlayerController)
	{
		UE_LOG(LogAccelByteSampleApp, Warning, TEXT("A player controller must be provided in order to finalize purchase."));
		OnError.ExecuteIfBound(static_cast<int32>(AccelByte::ErrorCodes::UnknownError), TEXT("player-controller-not-provided"));
		return;
	}

	const IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get(FName(DefaultSubsystemName));
	if (OnlineSubsystem == nullptr)
	{
		UE_LOG(LogAccelByteSampleApp, Warning, TEXT("Cannot finalize purchase with no online subsystem set!"));
		OnError.ExecuteIfBound(static_cast<int32>(AccelByte::ErrorCodes::UnknownError), TEXT("login-failed-native-subsystem-null"));
		return;
	}

	const IOnlineIdentityPtr OnlineIdentity = OnlineSubsystem->GetIdentityInterface();
	if (!OnlineIdentity.IsValid())
	{
		UE_LOG(LogAccelByteSampleApp, Warning, TEXT("Could not retrieve identity interface from the subsystem."));
		OnError.ExecuteIfBound(static_cast<int32>(AccelByte::ErrorCodes::UnknownError), TEXT("login-failed-native-identity-null"));
		return;
	}
	
	const IOnlinePurchasePtr OnlinePurchase = OnlineSubsystem->GetPurchaseInterface();
	if (!OnlinePurchase.IsValid())
	{
		UE_LOG(LogAccelByteSampleApp, Warning, TEXT("Could not retrieve purchase interface from the subsystem."));
		OnError.ExecuteIfBound(static_cast<int32>(AccelByte::ErrorCodes::UnknownError), TEXT("login-failed-native-purchase-null"));
		return;
	}

	const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(InPlayerController->Player);
	if (LocalPlayer == nullptr)
	{
		UE_LOG(LogAccelByteSampleApp, Warning, TEXT("Need local player to finalize purchase"));
		OnError.ExecuteIfBound(static_cast<int32>(AccelByte::ErrorCodes::UnknownError), TEXT("local-player-not-found"));
		return;
	}

	const auto UserIdPtr = OnlineIdentity->GetUniquePlayerId(LocalPlayer->GetControllerId());
	OnlinePurchase->FinalizePurchase(*UserIdPtr.Get(), ReceiptId);

	OnSuccess.ExecuteIfBound();
	
	UE_LOG(LogAccelByteSampleApp, Display, TEXT("IAP Purchase finalized!"));
}

void UAccelByteBluePrintsSample::SyncPurchaseGooglePlay
	( APlayerController* InPlayerController
	, FAccelByteModelsPlatformSyncMobileGoogle const& SyncRequest
	, FDHandler const& OnSuccess
	, FDErrorHandler const& OnError)
{
	const THandler<FAccelByteModelsPlatformSyncMobileGoogleResponse> OnSyncSuccessDelegate = THandler<FAccelByteModelsPlatformSyncMobileGoogleResponse>::CreateLambda(
		[&OnSuccess, &OnError, InPlayerController, ReceiptId = SyncRequest.OrderId](FAccelByteModelsPlatformSyncMobileGoogleResponse const& Response)
		{
			UE_LOG(LogAccelByteSampleApp, Display, TEXT("AccelByte sync purchase Google succeeded!"));
			if (Response.NeedConsume)
			{
				FinalizePurchase(InPlayerController, ReceiptId, OnSuccess, OnError);
			}
			else
			{
				OnSuccess.ExecuteIfBound();
			}
		});

	AccelByte::FErrorHandler OnSynErrorDelegate = AccelByte::FErrorHandler::CreateLambda([&OnError]
		( int32 ErrorCode
		, FString const& ErrorMessage )
		{
			UE_LOG(LogAccelByteSampleApp, Warning, TEXT("AccelByte sync purchase Google failed! code: %d - message: %s"), ErrorCode, *ErrorMessage);
			OnError.ExecuteIfBound(ErrorCode, ErrorMessage);
		});
	
	FRegistry::Entitlement.SyncMobilePlatformPurchaseGooglePlay(SyncRequest, OnSyncSuccessDelegate, OnSynErrorDelegate);
}

void UAccelByteBluePrintsSample::SyncPurchaseApple
	( APlayerController* InPlayerController
	,  FAccelByteModelsPlatformSyncMobileApple const& SyncRequest
	, FDHandler const& OnSuccess
	, FDErrorHandler const& OnError )
{
	FSimpleDelegate OnSyncPurchaseSuccessDelegate = FSimpleDelegate::CreateLambda([&OnSuccess]()
		{
			UE_LOG(LogAccelByteSampleApp, Display, TEXT("AccelByte sync purchase Apple succeeded!"));
			OnSuccess.ExecuteIfBound();
		});

	AccelByte::FErrorHandler OnSyncPurchaseErrorDelegate = AccelByte::FErrorHandler::CreateLambda([&OnError]
		( int32 ErrorCode
		, FString const& ErrorMessage)
		{
			UE_LOG(LogAccelByteSampleApp, Warning, TEXT("AccelByte sync purchase Apple failed! code: %d - message: %s"), ErrorCode, *ErrorMessage);
			OnError.ExecuteIfBound(ErrorCode, ErrorMessage);
		});

	UE_LOG(LogAccelByteSampleApp, Warning, TEXT("AccelByte sync request ProductId: %s - TransactionId: %s - ReceiptData: %s"), *SyncRequest.ProductId, *SyncRequest.TransactionId, *SyncRequest.ReceiptData);
	FRegistry::Entitlement.SyncMobilePlatformPurchaseApple(SyncRequest, OnSyncPurchaseSuccessDelegate, OnSyncPurchaseErrorDelegate);
}

FAccelByteModelsPlatformSyncMobileGoogle UAccelByteBluePrintsSample::ParseReceiptString(const FString& ReceiptData)
{
	FAccelByteModelsPlatformSyncMobileGoogle ReceiptStruct;

	//deserialize to pick receiptData encoded string
	FString OutReceiptEncStr;
	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ReceiptData);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
	{
		OutReceiptEncStr = JsonParsed->GetStringField("receiptData");
	}

	//decode to json string
	FString ReceiptDecodedStr;
	FBase64::Decode(OutReceiptEncStr, ReceiptDecodedStr);

	//deserialize to pick receiptData json string and assign data to FAccelByteModelsPlatformSyncMobileGoogle
	JsonParsed = nullptr;
	JsonReader = TJsonReaderFactory<TCHAR>::Create(ReceiptDecodedStr);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
	{
		ReceiptStruct.OrderId = JsonParsed->GetStringField("orderId");
		ReceiptStruct.PackageName = JsonParsed->GetStringField("packageName");
		ReceiptStruct.ProductId = JsonParsed->GetStringField("productId");
		ReceiptStruct.PurchaseTime = FCString::Atoi64(*JsonParsed->GetStringField("purchaseTime"));
		ReceiptStruct.PurchaseToken = JsonParsed->GetStringField("purchaseToken");
		ReceiptStruct.AutoAck = false;
		ReceiptStruct.Language = "en";
		ReceiptStruct.Region = "US";
		
		return ReceiptStruct;
	}
	
	return ReceiptStruct;		
}

FString UAccelByteBluePrintsSample::ParseReceiptToStringDisplay(const FString& ReceiptData)
{
	FString OutReceiptEncStr;
	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ReceiptData);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
	{
		OutReceiptEncStr = JsonParsed->GetStringField("receiptData");
	}

	//decode to json string
	FString ReceiptDecodedStr;
	FBase64::Decode(OutReceiptEncStr, ReceiptDecodedStr);

	return ReceiptDecodedStr;
}
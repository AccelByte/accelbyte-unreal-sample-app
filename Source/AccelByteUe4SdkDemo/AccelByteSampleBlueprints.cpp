#include "AccelByteSampleBlueprints.h"

#include "Misc/DateTime.h"
#include "Misc/ConfigCacheIni.h"

#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Core/AccelByteRegistry.h"
#include "Api/AccelByteUserApi.h"

#define STEAM_LOGIN_DELAY 2 // seconds

DECLARE_LOG_CATEGORY_EXTERN(LogAccelByteSampleApp, Log, All);
DEFINE_LOG_CATEGORY(LogAccelByteSampleApp);

#define DEFAULT_SUBSYSTEM_NAME TEXT("DefaultPlatformService")
#define NATIVE_SUBSYSTEM_NAME TEXT("NativePlatformService")

static FString DefaultSubsystemName;
static FString NativeSubsystemName;

static EAccelBytePlatformType GetAccelBytePlatformType(FName const& SubsystemName)
{
	const FString SubsystemStr = SubsystemName.ToString();
	if (SubsystemStr.Equals(TEXT("GDK"), ESearchCase::IgnoreCase) || SubsystemStr.Equals(TEXT("Live"), ESearchCase::IgnoreCase))
	{
		return EAccelBytePlatformType::Live;
	}
	else if (SubsystemStr.Equals(TEXT("PS4"), ESearchCase::IgnoreCase))
	{
		return EAccelBytePlatformType::PS4CrossGen;
	}
	else if (SubsystemStr.Equals(TEXT("PS5"), ESearchCase::IgnoreCase))
	{
		return EAccelBytePlatformType::PS5;
	}
	else if (SubsystemStr.Equals(TEXT("STEAM"), ESearchCase::IgnoreCase))
	{
		return EAccelBytePlatformType::Steam;
	}
	else if (SubsystemStr.Equals(TEXT("GOOGLEPLAY"), ESearchCase::IgnoreCase) || SubsystemStr.Equals(TEXT("GOOGLE"), ESearchCase::IgnoreCase))
	{
		return EAccelBytePlatformType::Google;
	}
	else if (SubsystemStr.Equals(TEXT("IOS"), ESearchCase::IgnoreCase) || SubsystemStr.Equals(TEXT("APPLE"), ESearchCase::IgnoreCase))
	{
		return EAccelBytePlatformType::Apple;
	}
	
	return EAccelBytePlatformType::Device;
}

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
	const EAccelBytePlatformType PlatformType = GetAccelBytePlatformType(OnlineSubsystem->GetSubsystemName());

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
	const EAccelBytePlatformType PlatformType = GetAccelBytePlatformType(OnlineSubsystem->GetSubsystemName());
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

void UAccelByteBluePrintsSample::FinalizePurchase
	( FDHandler const& OnSuccess
	, FDErrorHandler const& OnError )
{
	
}

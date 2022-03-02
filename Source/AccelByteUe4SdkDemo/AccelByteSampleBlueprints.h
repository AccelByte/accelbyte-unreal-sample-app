// Copyright (c) 2022 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Core/AccelByteError.h"
#include "Models/AccelByteEcommerceModels.h"
#include "AccelByteSampleBlueprints.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAccelByteLoginResult, APlayerController*, PlayerController, int32, ErrorCode, FString const&, ErrorMessage);
DECLARE_DYNAMIC_DELEGATE_OneParam(FDAccelByteModelsPlatformSyncMobileGoogleResponse, FAccelByteModelsPlatformSyncMobileGoogleResponse, Response);
DECLARE_DYNAMIC_DELEGATE_OneParam(FDAccelByteModelsItemInfo, FAccelByteModelsItemInfo, Response);

UCLASS(MinimalAPI)
class UAccelByteLoginNativePlatform : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	UAccelByteLoginNativePlatform(const FObjectInitializer& ObjectInitializer);

	// Called when there is a successful query
	UPROPERTY(BlueprintAssignable)
	FAccelByteLoginResult OnSuccess;
	
	// Called when there is an unsuccessful query
	UPROPERTY(BlueprintAssignable)
	FAccelByteLoginResult OnFailure;

	// Login with native platform online subsystem
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext="WorldContextObject"), Category = "AccelByte | SampleApp")
	static UAccelByteLoginNativePlatform* LoginWithNativePlatform(UObject* WorldContextObject, APlayerController* InPlayerController);

	// UBlueprintAsyncActionBase interface
	virtual void Activate() override;
	// End of UBlueprintAsyncActionBase interface

private:
	// Internal callback when the login UI closes, calls out to the public success/failure callbacks
	void OnLoginNativePlatformCompleted(int32 NativeLocalUserNum, bool bWasNativeLoginSuccessful, FUniqueNetId const& NativeUserId, FString const& NativeError);

	// The player controller triggering things
	TWeakObjectPtr<APlayerController> PlayerControllerWeakPtr;

	// The world context object in which this call is taking place
	UObject* WorldContextObject;
};

UCLASS(MinimalAPI)
class UAccelByteLogin : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	UAccelByteLogin(const FObjectInitializer& ObjectInitializer);

	// Called when there is a successful query
	UPROPERTY(BlueprintAssignable)
	FAccelByteLoginResult OnSuccess;
	
	// Called when there is an unsuccessful query
	UPROPERTY(BlueprintAssignable)
	FAccelByteLoginResult OnFailure;

	// Login to AccelByte service
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext="WorldContextObject"), Category = "AccelByte | SampleApp")
	static UAccelByteLogin* LoginWithAccelByte(UObject* WorldContextObject, APlayerController* InPlayerController);

	// UBlueprintAsyncActionBase interface
	virtual void Activate() override;
	// End of UBlueprintAsyncActionBase interface

private:
	// Internal callback when the login UI closes, calls out to the public success/failure callbacks
	void OnLoginAccelByteCompleted();

	void OnLoginAccelByteFailed(int32 ErrorCode, FString const& ErrorMessage);

	// The player controller triggering things
	TWeakObjectPtr<APlayerController> PlayerControllerWeakPtr;

	// The world context object in which this call is taking place
	UObject* WorldContextObject;
};

UCLASS(Blueprintable, BlueprintType)
class UAccelByteBluePrintsSample : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "AccelByte | SampleApp | Utilities")
	static void LoadConfig();
	
	UFUNCTION(BlueprintCallable, Category = "AccelByte | SampleApp | Utilities")
	static EAccelBytePlatformType GetPlatformTypeFromSubsystem(FString const& SubsystemName);

	UFUNCTION(BlueprintCallable, Category = "AccelByte | SampleApp | Utilities")
	static EAccelBytePlatformType GetNativePlatformType();
	
	UFUNCTION(BlueprintCallable, Category = "AccelByte | SampleApp | IAP")
	static void GetItemBySku(FString const& Sku, FDAccelByteModelsItemInfo const& OnSuccess, FDErrorHandler const& OnError);

	UFUNCTION(BlueprintCallable, Category = "AccelByte | SampleApp | IAP")
	static void FinalizePurchase(APlayerController* InPlayerController, FString const& ReceiptId, FDHandler const& OnSuccess, FDErrorHandler const& OnError);

	UFUNCTION(BlueprintCallable, Category = "AccelByte | SampleApp | IAP")
	static void SyncPurchaseGooglePlay(APlayerController* InPlayerController, FAccelByteModelsPlatformSyncMobileGoogle const& SyncRequest, FDHandler const& OnSuccess, FDErrorHandler const& OnError);
	
	UFUNCTION(BlueprintCallable, Category = "AccelByte | SampleApp | IAP")
	static void SyncPurchaseApple(APlayerController* InPlayerController, FAccelByteModelsPlatformSyncMobileApple const& SyncRequest, FDHandler const& OnSuccess, FDErrorHandler const& OnError);

	UFUNCTION(BlueprintCallable, Category = "AccelByte | SampleApp | IAP")
	static FAccelByteModelsPlatformSyncMobileGoogle ParseReceiptString(FString const& ReceiptData);

	UFUNCTION(BlueprintCallable, Category = "AccelByte | SampleApp | IAP")
	static FString ParseReceiptToStringDisplay(FString const& ReceiptData);
};
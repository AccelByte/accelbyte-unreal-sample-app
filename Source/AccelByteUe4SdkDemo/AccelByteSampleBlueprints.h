// Copyright (c) 2022 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Core/AccelByteError.h"
#include "AccelByteSampleBlueprints.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAccelByteLoginResult, APlayerController*, PlayerController, int32, ErrorCode, FString const&, ErrorMessage);

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
	static UAccelByteLoginNativePlatform* LoginWithNativePlatform(UObject* WorldContextObject, class APlayerController* InPlayerController);

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
	static UAccelByteLogin* LoginWithAccelByte(UObject* WorldContextObject, class APlayerController* InPlayerController);

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
	UFUNCTION(BlueprintCallable, Category = "AccelByte | SampleApp | Load Config")
	static void LoadConfig();
	
	UFUNCTION(BlueprintCallable, Category = "AccelByte | SampleApp | IAP")
	static void FinalizePurchase(FDHandler const& OnSuccess, FDErrorHandler const& OnError);
};

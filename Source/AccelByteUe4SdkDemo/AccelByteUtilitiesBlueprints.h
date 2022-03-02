// Copyright (c) 2022 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Core/AccelByteError.h"
#include "AccelByteUtilitiesBlueprints.generated.h"

UCLASS(Blueprintable, BlueprintType)
class UAccelByteUtilitiesBlueprints : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category = "AccelByte | Utilities ")
	static FString ConvertToString(TArray<uint8> const& Bytes);

	UFUNCTION(BlueprintCallable, Category = "AccelByte | Utilities ")
	static TArray<uint8> ConvertToBytes(FString const& String);
};

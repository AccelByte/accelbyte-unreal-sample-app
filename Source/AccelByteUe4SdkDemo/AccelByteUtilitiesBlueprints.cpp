#include "AccelByteUtilitiesBlueprints.h"

FString UAccelByteUtilitiesBlueprints::ConvertToString(TArray<uint8> const& Bytes)
{
	uint32 Size = Bytes.Num();
	FString Data = BytesToString(Bytes.GetData(), Size);
	return Data;
}

TArray<uint8> UAccelByteUtilitiesBlueprints::ConvertToBytes(FString const& String)
{
	int32 Length = String.Len();
	uint8* Output = new uint8[Length];
	TArray<uint8> Return;
	Return.AddUninitialized(Length);
	StringToBytes(String, Return.GetData(), Length);
	return Return;
}
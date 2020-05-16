// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UEOscElement.generated.h"

UENUM()
enum class EUEOSCElementType : uint8
{
	OET_Int32 UMETA(DisplayName = "Int32"),
	OET_Float UMETA(DisplayName = "Float"),
	OET_String UMETA(DisplayName = "String"),
	OET_Blob UMETA(DisplayName = "Blob"),
	OET_Bool UMETA(DisplayName = "Bool"),
	OET_Nil UMETA(DisplayName = "Nil")
};

/**
 * 
 */
USTRUCT(BlueprintType)
struct UEOSC_API FUEOSCElement
{
	GENERATED_USTRUCT_BODY()

public:
	EUEOSCElementType Type;
	
	int32 IntValue = 0;
	uint64 LongValue = 0;
	float FloatValue = 0;
	FName StringValue;
	TArray<uint8> BlobValue;
	bool BoolValue = false;
};


/**
 *
 */
USTRUCT(BlueprintType)
struct UEOSC_API FUEOSCMessage
{
	GENERATED_USTRUCT_BODY()

public:
	FString Address;
	TArray<FUEOSCElement> Elements;
};

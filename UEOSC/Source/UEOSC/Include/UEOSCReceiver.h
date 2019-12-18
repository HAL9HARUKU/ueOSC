// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UdpSocketReceiver.h"
#include "UEOSCElement.h"
#include "UEOSCReceiver.generated.h"

class FSocket;
struct FIPv4Endpoint;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FUEOSCReceiveEventDelegate, const FName&, Address, const TArray<FUEOSCElement>&, Data, const FString&, SenderIp);

/**
 * 
 */
UCLASS()
class UEOSC_API UUEOSCReceiver : public UObject
{
    GENERATED_BODY()

private:
	TSharedPtr<FSocket> Socket;
	TSharedPtr<FUdpSocketReceiver> Receiver;
	
public:
	bool Connect(int32 InPort);
	void Disconnect();

private:
	void OnReceived(const FArrayReaderPtr& InData, const FIPv4Endpoint& InIp);
private:
	static uint32 ReadOSCInt32(int32& OutValue, const uint8* InData, int32 Count);
	static uint32 ReadOSCFloat32(float& OutValue, const uint8* InData, int32 Count);
	static uint32 ReadOSCString(FString& OutValue, const uint8* InData, int32 Count);
	static uint32 ReadOSCString(FName& OutValue, const uint8* InData, int32 Count);
	static uint32 ReadOSCBlob(TArray<uint8>& OutValue, const uint8* InData, int32 Count);
	
public:
	FUEOSCReceiveEventDelegate OSCReceiveEventDelegate;
};

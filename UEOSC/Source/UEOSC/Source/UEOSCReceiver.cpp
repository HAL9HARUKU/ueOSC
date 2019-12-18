// Fill out your copyright notice in the Description page of Project Settings.

#include "../Include/UEOSCReceiver.h"
#include "../Include/UEOSCElement.h"
#include "UdpSocketBuilder.h"
#include "Containers/StringConv.h"

bool UUEOSCReceiver::Connect(int32 InPort)
{
	if (this->Socket.IsValid() || this->Receiver.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] Socket or Receiver is Already Running"), *FString(__FUNCTION__));
		return false;
	}
	TSharedPtr<FSocket> NewSocket = MakeShareable(FUdpSocketBuilder(TEXT("UDP Socket")).BoundToPort(InPort).Build());
	if (!NewSocket.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] Socket Error"), *FString(__FUNCTION__));
		return false;
	}

	TSharedPtr<FUdpSocketReceiver> NewReceiver = MakeShareable(new FUdpSocketReceiver(NewSocket.Get(), FTimespan(0, 0, 1), TEXT("UDP Receiver")));
	if (!NewReceiver.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("[%s] Receiver Error"), *FString(__FUNCTION__));
		return false;
	}

	this->Socket = NewSocket;
	this->Receiver = NewReceiver;
	
	this->Receiver->OnDataReceived().BindUObject(this, &UUEOSCReceiver::OnReceived);
	this->Receiver->Start();

	return true;
}

void UUEOSCReceiver::Disconnect()
{
	if (this->Receiver.IsValid()) {
		this->Receiver->Exit();
		this->Receiver.Reset();
	}

	if (this->Socket.IsValid()) {
		this->Socket->Close();
		this->Socket.Reset();
	}
}

void UUEOSCReceiver::OnReceived(const FArrayReaderPtr& InData, const FIPv4Endpoint& InIp)
{
	if (!InData.IsValid())
	{
		return;
	}
	
	const uint8* Begin = InData->GetData();
	const uint8* End = Begin + InData->Num();
	const uint8* Data = Begin;
	int32 Length = InData->Num();
	int32 Count = 0;

	TArray<uint8> Buffer;
	Buffer.SetNum(Length);
	FMemory::Memcpy(Buffer.GetData(), Data, Length);

	// Address
	FString Address;
	Count = ReadOSCString(Address, Data, (int32)(End - Data));
	if (Count == 0)
	{
	}
	Data += Count;

	// Sematics
	FString Sematics;
	Count = ReadOSCString(Sematics, Data, (int32)(End - Data));
	if (Count == 0)
	{
	}
	Data += Count;

	TArray<FUEOSCElement> Elements;

	// Message
	for (int32 Index = 1; Index < Sematics.Len(); ++Index)
	{
		TCHAR S = Sematics[Index];
		if (S == TEXT('i'))
		{
			FUEOSCElement NewElement;
			NewElement.Type = EUEOSCElementType::OET_Int32;
			
			Count = ReadOSCInt32(NewElement.IntValue, Data, (int32)(End - Data));
			if (Count == 0)
			{
				break;
			}
			Data += Count;
			
			Elements.Emplace(NewElement);
		}
		else if (S == TEXT('f'))
		{
			FUEOSCElement NewElement;
			NewElement.Type = EUEOSCElementType::OET_Float;
			
			Count = ReadOSCFloat32(NewElement.FloatValue, Data, (int32)(End - Data));
			if (Count == 0)
			{
				break;
			}
			Data += Count;

			Elements.Emplace(NewElement);
		}
		else if (S == TEXT('s'))
		{
			FUEOSCElement NewElement;
			NewElement.Type = EUEOSCElementType::OET_String;
			
			Count = ReadOSCString(NewElement.StringValue, Data, (int32)(End - Data));
			if (Count == 0)
			{
				break;
			}
			Data += Count;

			Elements.Emplace(NewElement);
		}
		else if (S == TEXT('b'))
		{
			FUEOSCElement NewElement;
			NewElement.Type = EUEOSCElementType::OET_Blob;
			
			Count = ReadOSCBlob(NewElement.BlobValue, Data, (int32)(End - Data));
			if (Count == 0)
			{
				break;
			}
			Data += Count;

			Elements.Emplace(NewElement);
		}
		else if (S == TEXT('T'))
		{
			FUEOSCElement NewElement;
			NewElement.Type = EUEOSCElementType::OET_Bool;
			NewElement.BoolValue = true;

			Elements.Emplace(NewElement);
		}
		else if (S == TEXT('F'))
		{
			FUEOSCElement NewElement;
			NewElement.Type = EUEOSCElementType::OET_Bool;
			NewElement.BoolValue = false;

			Elements.Emplace(NewElement);
		}
		else if (S == TEXT('N'))
		{
			FUEOSCElement NewElement;
			NewElement.Type = EUEOSCElementType::OET_Nil;

			Elements.Emplace(NewElement);
		}
		else
		{
		}
	}

	// Broadcast
	this->OSCReceiveEventDelegate.Broadcast(FName(*Address), Elements, InIp.ToString());
}

uint32 UUEOSCReceiver::ReadOSCInt32(int32& OutValue, const uint8* InData, int32 Count)
{
	uint8 Data[4];
	Data[0] = InData[3];
	Data[1] = InData[2];
	Data[2] = InData[1];
	Data[3] = InData[0];
	FMemory::Memcpy(&OutValue, Data, 4);
	return 4;
}

uint32 UUEOSCReceiver::ReadOSCFloat32(float& OutValue, const uint8* InData, int32 Count)
{
	uint8 Data[4];
	Data[0] = InData[3];
	Data[1] = InData[2];
	Data[2] = InData[1];
	Data[3] = InData[0];
	FMemory::Memcpy(&OutValue, Data, 4);
	return 4;
}

uint32 UUEOSCReceiver::ReadOSCString(FString& OutValue, const uint8* InData, int32 Count)
{
	const uint8* Begin = InData;
	while (*InData != 0 && Count > 0)
	{
		++InData;
		--Count;
	}
	const uint8* End = InData;

	uint32 Length = End - Begin;
	if (Length == 0)
	{
		return 0;
	}
	const uint32 PaddedLength = (Length + 4) & 0xFFFFFFFC;
	OutValue = ANSI_TO_TCHAR((ANSICHAR*)Begin);
	return PaddedLength;
}

uint32 UUEOSCReceiver::ReadOSCString(FName& OutValue, const uint8* InData, int32 Count)
{
	FString StringValue;
	uint32 ReadSize = ReadOSCString(StringValue, InData, Count);
	OutValue = FName(*StringValue);
	return ReadSize;
}

uint32 UUEOSCReceiver::ReadOSCBlob(TArray<uint8>& OutValue, const uint8* InData, int32 Count)
{
	// Size
	int32 BlobSize;
	uint8 Data[4];
	Data[0] = InData[3];
	Data[1] = InData[2];
	Data[2] = InData[1];
	Data[3] = InData[0];
	InData += 4;
	FMemory::Memcpy(&BlobSize, Data, 4);

	// Contents
	OutValue.SetNum(BlobSize);
	FMemory::Memcpy(OutValue.GetData(), InData, BlobSize);
	const uint32 PaddedLength = (BlobSize + 3) & 0xFFFFFFFC;
	return 4 + PaddedLength;
}

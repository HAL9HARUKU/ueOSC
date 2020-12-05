// Fill out your copyright notice in the Description page of Project Settings.

#include "../Include/UEOSCReceiver.h"
#include "../Include/UEOSCElement.h"
#include "Common/UdpSocketBuilder.h"
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
	
	// Read
	TArray<FUEOSCMessage> Messages;
	ReadOSC(Messages, InData->GetData(), InData->Num());

	// Broadcast
	const FString Ip = InIp.ToString();
	for (const auto& Message : Messages)
	{
		this->OSCReceiveEventDelegate.Broadcast(FName(*Message.Address), Message.Elements, Ip);
	}
}

void UUEOSCReceiver::ReadOSC(TArray<FUEOSCMessage>& OutValue, const uint8* InData, int32 Count)
{
	int32 DataSize = Count;
	const uint8* Data = InData;
	
	int32 ReadSize = 0;
	
	// Address
	FString Address;
	ReadSize = ReadOSCString(Address, Data, DataSize);
	if (ReadSize == 0)
	{
		return;
	}
	Data += ReadSize;
	DataSize -= ReadSize;

	if (Address == TEXT("#bundle"))
	{
		// Bundle

		ReadSize = ReadOSCBundle(OutValue, Data, DataSize);
		if (ReadSize == 0)
		{
			return;
		}
		Data += ReadSize;
		DataSize -= ReadSize;
	}
	else
	{
		// Message
		FUEOSCMessage Message;
		
		Message.Address = Address;
		
		ReadSize = ReadOSCMessage(Message, Data, DataSize);
		if (ReadSize == 0)
		{
			return;
		}
		Data += ReadSize;
		DataSize -= ReadSize;

		OutValue.Emplace(Message);
	}
}

uint32 UUEOSCReceiver::ReadOSCBundle(TArray<FUEOSCMessage>& OutValue, const uint8* InData, int32 Count)
{
	int32 DataSize = Count;
	const uint8* Data = InData;
	
	int32 ReadSize = 0;
	
	// Time
	uint64 Time = 0;
	ReadSize = ReadOSCUint64(Time, Data, DataSize);
	if (ReadSize == 0)
	{
		return Count - DataSize;
	}
	Data += ReadSize;
	DataSize -= ReadSize;

	while (DataSize > 0)
	{
		// Content Size
		int32 ContentSize = 0;
		ReadSize = ReadOSCInt32(ContentSize, Data, DataSize);
		if (ReadSize == 0)
		{
			return Count - DataSize;
		}
		Data += ReadSize;
		DataSize -= ReadSize;

		if (ContentSize == (ContentSize & ~0x3))
		{
			// Message
			ReadOSC(OutValue, Data, ContentSize);
		}
		
		Data += ContentSize;
		DataSize -= ContentSize;
	}

	return Count - DataSize;
}

uint32 UUEOSCReceiver::ReadOSCMessage(FUEOSCMessage& OutValue, const uint8* InData, int32 Count)
{
	int32 DataSize = Count;
	const uint8* Data = InData;
	
	int32 ReadSize = 0;

	// Semantics
	FString Semantics;
	ReadSize = ReadOSCString(Semantics, Data, DataSize);
	if (ReadSize == 0)
	{
		return Count - DataSize;
	}
	Data += ReadSize;
	DataSize -= ReadSize;

	// Message
	for (int32 Index = 1; Index < Semantics.Len(); ++Index)
	{
		TCHAR Semantic = Semantics[Index];
		if (Semantic == TEXT('i'))
		{
			FUEOSCElement NewElement;
			NewElement.Type = EUEOSCElementType::OET_Int32;

			ReadSize = ReadOSCInt32(NewElement.IntValue, Data, DataSize);
			if (ReadSize == 0)
			{
				break;
			}
			Data += ReadSize;
			DataSize -= ReadSize;

			OutValue.Elements.Emplace(NewElement);
		}
		else if (Semantic == TEXT('f'))
		{
			FUEOSCElement NewElement;
			NewElement.Type = EUEOSCElementType::OET_Float;

			ReadSize = ReadOSCFloat32(NewElement.FloatValue, Data, DataSize);
			if (ReadSize == 0)
			{
				break;
			}
			Data += ReadSize;
			DataSize -= ReadSize;

			OutValue.Elements.Emplace(NewElement);
		}
		else if (Semantic == TEXT('s'))
		{
			FUEOSCElement NewElement;
			NewElement.Type = EUEOSCElementType::OET_String;

			ReadSize = ReadOSCString(NewElement.StringValue, Data, DataSize);
			if (ReadSize == 0)
			{
				break;
			}
			Data += ReadSize;
			DataSize -= ReadSize;

			OutValue.Elements.Emplace(NewElement);
		}
		else if (Semantic == TEXT('b'))
		{
			FUEOSCElement NewElement;
			NewElement.Type = EUEOSCElementType::OET_Blob;

			ReadSize = ReadOSCBlob(NewElement.BlobValue, Data, DataSize);
			if (ReadSize == 0)
			{
				break;
			}
			Data += ReadSize;
			DataSize -= ReadSize;

			OutValue.Elements.Emplace(NewElement);
		}
		else if (Semantic == TEXT('T'))
		{
			FUEOSCElement NewElement;
			NewElement.Type = EUEOSCElementType::OET_Bool;
			NewElement.BoolValue = true;

			OutValue.Elements.Emplace(NewElement);
		}
		else if (Semantic == TEXT('F'))
		{
			FUEOSCElement NewElement;
			NewElement.Type = EUEOSCElementType::OET_Bool;
			NewElement.BoolValue = false;

			OutValue.Elements.Emplace(NewElement);
		}
		else if (Semantic == TEXT('N'))
		{
			FUEOSCElement NewElement;
			NewElement.Type = EUEOSCElementType::OET_Nil;

			OutValue.Elements.Emplace(NewElement);
		}
		else
		{
		}
	}

	return Count - DataSize;
}

uint32 UUEOSCReceiver::ReadOSCInt32(int32& OutValue, const uint8* InData, int32 Count)
{
	if (Count < 4)
	{
		return 0;
	}
	uint8 Data[4];
	Data[0] = InData[3];
	Data[1] = InData[2];
	Data[2] = InData[1];
	Data[3] = InData[0];
	FMemory::Memcpy(&OutValue, Data, 4);
	return 4;
}

uint32 UUEOSCReceiver::ReadOSCInt64(int64& OutValue, const uint8* InData, int32 Count)
{
	if (Count < 8)
	{
		return 0;
	}
	uint8 Data[8];
	Data[0] = InData[7];
	Data[1] = InData[6];
	Data[2] = InData[5];
	Data[3] = InData[4];
	Data[4] = InData[3];
	Data[5] = InData[2];
	Data[6] = InData[1];
	Data[7] = InData[0];
	FMemory::Memcpy(&OutValue, Data, 8);
	return 8;
}

uint32 UUEOSCReceiver::ReadOSCUint64(uint64& OutValue, const uint8* InData, int32 Count)
{
	if (Count < 8)
	{
		return 0;
	}
	uint8 Data[8];
	Data[0] = InData[7];
	Data[1] = InData[6];
	Data[2] = InData[5];
	Data[3] = InData[4];
	Data[4] = InData[3];
	Data[5] = InData[2];
	Data[6] = InData[1];
	Data[7] = InData[0];
	FMemory::Memcpy(&OutValue, Data, 8);
	return 8;
}

uint32 UUEOSCReceiver::ReadOSCFloat32(float& OutValue, const uint8* InData, int32 Count)
{
	if (Count < 4)
	{
		return 0;
	}
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
	if (Count < 4)
	{
		return 0;
	}
	// Size
	int32 BlobSize;
	uint8 Data[4];
	Data[0] = InData[3];
	Data[1] = InData[2];
	Data[2] = InData[1];
	Data[3] = InData[0];
	InData += 4;
	Count -= 4;
	FMemory::Memcpy(&BlobSize, Data, 4);

	if (Count < BlobSize)
	{
		return 0;
	}
	// Contents
	OutValue.SetNum(BlobSize);
	FMemory::Memcpy(OutValue.GetData(), InData, BlobSize);
	const uint32 PaddedLength = (BlobSize + 3) & 0xFFFFFFFC;
	return 4 + PaddedLength;
}

// Copyright (c) 2025-2026 POWDER-RANGER. All Rights Reserved.

#include "Core/N1PredictionBuffer.h"

UN1PredictionBuffer::UN1PredictionBuffer()
{
	Buffer.Reserve(MaxFrames);
}

void UN1PredictionBuffer::Initialize(int32 MaxHistoryFrames, float InTickRate)
{
	MaxFrames = MaxHistoryFrames;
	TickRate = InTickRate;
	Buffer.Reset();
	Buffer.Reserve(MaxFrames);
	CurrentOldestTick = 0;
	CurrentNewestTick = 0;

	UE_LOG(LogN1Netcode, Log, TEXT("PredictionBuffer initialized: %d frames @ %.0f Hz (%.1f ms history)"),
		MaxFrames, TickRate, (MaxFrames / TickRate) * 1000.0f);
}

void UN1PredictionBuffer::StorePredictedState(int32 Tick, const FN1EntityState& State)
{
	FBufferEntry* Entry = nullptr;
	for (auto& E : Buffer)
	{
		if (E.Tick == Tick)
		{
			Entry = &E;
			break;
		}
	}

	if (!Entry)
	{
		if (Buffer.Num() >= MaxFrames)
		{
			Buffer.RemoveAt(0);
		}
		FBufferEntry NewEntry;
		NewEntry.Tick = Tick;
		NewEntry.PredictedState = State;
		NewEntry.bHasState = true;
		Buffer.Add(NewEntry);
	}
	else
	{
		Entry->PredictedState = State;
		Entry->bHasState = true;
	}

	CurrentNewestTick = FMath::Max(CurrentNewestTick, Tick);
	if (CurrentOldestTick == 0 || Tick < CurrentOldestTick)
	{
		CurrentOldestTick = Tick;
	}
}

void UN1PredictionBuffer::StoreInputFrame(int32 Tick, const FN1InputFrame& Input)
{
	FBufferEntry* Entry = nullptr;
	for (auto& E : Buffer)
	{
		if (E.Tick == Tick)
		{
			Entry = &E;
			break;
		}
	}

	if (!Entry)
	{
		if (Buffer.Num() >= MaxFrames)
		{
			Buffer.RemoveAt(0);
		}
		FBufferEntry NewEntry;
		NewEntry.Tick = Tick;
		NewEntry.Input = Input;
		NewEntry.bHasInput = true;
		Buffer.Add(NewEntry);
	}
	else
	{
		Entry->Input = Input;
		Entry->bHasInput = true;
	}
}

bool UN1PredictionBuffer::GetPredictedState(int32 Tick, FN1EntityState& OutState) const
{
	for (const auto& E : Buffer)
	{
		if (E.Tick == Tick && E.bHasState)
		{
			OutState = E.PredictedState;
			return true;
		}
	}
	return false;
}

bool UN1PredictionBuffer::GetInputFrame(int32 Tick, FN1InputFrame& OutInput) const
{
	for (const auto& E : Buffer)
	{
		if (E.Tick == Tick && E.bHasInput)
		{
			OutInput = E.Input;
			return true;
		}
	}
	return false;
}

int32 UN1PredictionBuffer::GetOldestTick() const
{
	return CurrentOldestTick;
}

int32 UN1PredictionBuffer::GetNewestTick() const
{
	return CurrentNewestTick;
}

void UN1PredictionBuffer::DiscardOlderThan(int32 Tick)
{
	Buffer.RemoveAll([Tick](const FBufferEntry& E) { return E.Tick < Tick; });
	if (Buffer.Num() > 0)
	{
		CurrentOldestTick = Buffer[0].Tick;
	}
}

bool UN1PredictionBuffer::HasContiguousInputs(int32 StartTick, int32 EndTick) const
{
	for (int32 Tick = StartTick; Tick <= EndTick; ++Tick)
	{
		bool Found = false;
		for (const auto& E : Buffer)
		{
			if (E.Tick == Tick && E.bHasInput)
			{
				Found = true;
				break;
			}
		}
		if (!Found) return false;
	}
	return true;
}

TArray<FN1EntityState> UN1PredictionBuffer::GetStateRange(int32 StartTick, int32 EndTick) const
{
	TArray<FN1EntityState> Result;
	for (int32 Tick = StartTick; Tick <= EndTick; ++Tick)
	{
		FN1EntityState State;
		if (GetPredictedState(Tick, State))
		{
			Result.Add(State);
		}
	}
	return Result;
}

TArray<FN1InputFrame> UN1PredictionBuffer::GetInputRange(int32 StartTick, int32 EndTick) const
{
	TArray<FN1InputFrame> Result;
	for (int32 Tick = StartTick; Tick <= EndTick; ++Tick)
	{
		FN1InputFrame Input;
		if (GetInputFrame(Tick, Input))
		{
			Result.Add(Input);
		}
	}
	return Result;
}

int32 UN1PredictionBuffer::GetMemoryFootprint() const
{
	return Buffer.Num() * sizeof(FBufferEntry);
}

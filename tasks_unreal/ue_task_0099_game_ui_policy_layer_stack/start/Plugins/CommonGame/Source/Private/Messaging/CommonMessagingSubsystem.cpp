// Copyright Epic Games, Inc. All Rights Reserved.

#include "Messaging/CommonMessagingSubsystem.h"
#include "Messaging/CommonGameDialog.h"
#include "UObject/UObjectHash.h"

void UCommonMessagingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonMessagingSubsystem::Deinitialize()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


bool UCommonMessagingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UCommonMessagingSubsystem::ShowConfirmation(UCommonGameDialogDescriptor* DialogDescriptor, FCommonMessagingResultDelegate ResultCallback)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonMessagingSubsystem::ShowError(UCommonGameDialogDescriptor* DialogDescriptor, FCommonMessagingResultDelegate ResultCallback)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

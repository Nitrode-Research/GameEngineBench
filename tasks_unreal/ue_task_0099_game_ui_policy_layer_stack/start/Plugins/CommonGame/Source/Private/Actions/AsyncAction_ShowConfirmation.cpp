// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actions/AsyncAction_ShowConfirmation.h"
#include "Messaging/CommonGameDialog.h"

UAsyncAction_ShowConfirmation::UAsyncAction_ShowConfirmation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


UAsyncAction_ShowConfirmation* UAsyncAction_ShowConfirmation::ShowConfirmationYesNo(UObject* InWorldContextObject, FText Title, FText Message)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UAsyncAction_ShowConfirmation* UAsyncAction_ShowConfirmation::ShowConfirmationOkCancel(UObject* InWorldContextObject, FText Title, FText Message)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UAsyncAction_ShowConfirmation* UAsyncAction_ShowConfirmation::ShowConfirmationCustom(UObject* InWorldContextObject, UCommonGameDialogDescriptor* Descriptor)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


void UAsyncAction_ShowConfirmation::Activate()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UAsyncAction_ShowConfirmation::HandleConfirmationResult(ECommonMessagingResult ConfirmationResult)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



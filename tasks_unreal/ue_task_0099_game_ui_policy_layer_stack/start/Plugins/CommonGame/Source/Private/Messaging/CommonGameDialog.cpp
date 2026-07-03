// Copyright Epic Games, Inc. All Rights Reserved.

#include "Messaging/CommonGameDialog.h"

#define LOCTEXT_NAMESPACE "Messaging"

UCommonGameDialogDescriptor* UCommonGameDialogDescriptor::CreateConfirmationOk(const FText& Header, const FText& Body)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UCommonGameDialogDescriptor* UCommonGameDialogDescriptor::CreateConfirmationOkCancel(const FText& Header, const FText& Body)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UCommonGameDialogDescriptor* UCommonGameDialogDescriptor::CreateConfirmationYesNo(const FText& Header, const FText& Body)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UCommonGameDialogDescriptor* UCommonGameDialogDescriptor::CreateConfirmationYesNoCancel(const FText& Header, const FText& Body)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return nullptr;
}


UCommonGameDialog::UCommonGameDialog()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonGameDialog::SetupDialog(UCommonGameDialogDescriptor* Descriptor, FCommonMessagingResultDelegate ResultCallback)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonGameDialog::KillDialog()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#undef LOCTEXT_NAMESPACE
// Copyright Epic Games, Inc. All Rights Reserved.

#include "CommonPlayerInputKey.h"
#include "Components/Image.h"
#include "Components/WidgetSwitcher.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "CommonInputSubsystem.h"
#include "CommonTextBlock.h"
#include "CommonUISettings.h"
#include "CommonUITypes.h"
#include "Styling/SlateBrush.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"
#include "CommonPlayerController.h"
#include "CommonLocalPlayer.h"

#define LOCTEXT_NAMESPACE "CommonKeybindWidget"

DECLARE_LOG_CATEGORY_EXTERN(LogCommonPlayerInput, Log, All);
DEFINE_LOG_CATEGORY(LogCommonPlayerInput);

struct FSlateDrawUtil
{
	static void DrawBrushCenterFit(
		FSlateWindowElementList& ElementList,
		uint32 InLayer,
		const FGeometry& InAllottedGeometry,
		const FSlateBrush* InBrush,
		const FLinearColor& InTint = FLinearColor::White)
	{
		DrawBrushCenterFitWithOffset
		(
			ElementList,
			InLayer,
			InAllottedGeometry,
			InBrush,
			InTint,
			FVector2D(0, 0)
		);
	}

	static void DrawBrushCenterFitWithOffset(
		FSlateWindowElementList& ElementList,
		uint32 InLayer,
		const FGeometry& InAllottedGeometry,
		const FSlateBrush* InBrush,
		const FLinearColor& InTint,
		const FVector2D InOffset)
	{
		if (!InBrush)
		{
			return;
		}

		const FVector2D AreaSize = InAllottedGeometry.GetLocalSize();
		const FVector2D ProgressSize = InBrush->GetImageSize();
		const float FitScale = FMath::Min(FMath::Min(AreaSize.X / ProgressSize.X, AreaSize.Y / ProgressSize.Y), 1.0f);
		const FVector2D FinalSize = FitScale * ProgressSize;

		const FVector2D Offset = (InAllottedGeometry.GetLocalSize() * 0.5f) - (FinalSize * 0.5f) + InOffset;

		FSlateDrawElement::MakeBox
		(
			ElementList,
			InLayer,
			InAllottedGeometry.ToPaintGeometry(Offset, FinalSize),
			InBrush,
			ESlateDrawEffect::None,
			InTint
		);
	}
};



void FMeasuredText::SetText(const FText& InText)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


FVector2D FMeasuredText::UpdateTextSize(const FSlateFontInfo &InFontInfo, float FontScale) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


UCommonPlayerInputKey::UCommonPlayerInputKey(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, BoundKeyFallback(EKeys::Invalid)
	, InputTypeOverride(ECommonInputType::Count)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonPlayerInputKey::NativePreConstruct()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonPlayerInputKey::NativeConstruct()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonPlayerInputKey::NativeDestruct()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


int32 UCommonPlayerInputKey::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UCommonPlayerInputKey::StartHoldProgress(FName HoldActionName, float HoldDuration)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonPlayerInputKey::StopHoldProgress(FName HoldActionName, bool bCompletedSuccessfully)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonPlayerInputKey::SyncHoldProgress()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonPlayerInputKey::UpdateHoldProgress()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonPlayerInputKey::UpdateKeybindWidget()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonPlayerInputKey::SetBoundKey(FKey NewKey)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonPlayerInputKey::SetBoundAction(FName NewBoundAction)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonPlayerInputKey::NativeOnInitialized()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonPlayerInputKey::SetForcedHoldKeybind(bool InForcedHoldKeybind)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonPlayerInputKey::SetForcedHoldKeybindStatus(ECommonKeybindForcedHoldStatus InForcedHoldKeybindStatus)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonPlayerInputKey::SetShowProgressCountDown(bool bShow)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonPlayerInputKey::SetupHoldKeybind()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonPlayerInputKey::ShowHoldBackPlate()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonPlayerInputKey::HandlePlayerControllerSet(UCommonLocalPlayer* LocalPlayer, APlayerController* PlayerController)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UCommonPlayerInputKey::RecalculateDesiredSize()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#undef LOCTEXT_NAMESPACE

// Copyright Epic Games, Inc. All Rights Reserved.

#include "SActorCanvas.h"
#include "Layout/ArrangedChildren.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"
#include "IActorIndicatorWidget.h"
#include "ECRIndicatorManagerComponent.h"
#include "Widgets/Layout/SBox.h"

namespace EArrowDirection
{
	enum Type
	{
		Left,
		Top,
		Right,
		Bottom,
		MAX
	};
}

// Angles for the direction of the arrow to display
const float ArrowRotations[EArrowDirection::MAX] =
{
	270.0f,
	0.0f,
	90.0f,
	180.0f
};

// Offsets for the each direction that the arrow can point
const FVector2D ArrowOffsets[EArrowDirection::MAX] =
{
	FVector2D(-1.0f, 0.0f),
	FVector2D(0.0f, -1.0f),
	FVector2D(1.0f, 0.0f),
	FVector2D(0.0f, 1.0f)
};


class SActorCanvasArrowWidget : public SLeafWidget
{
public:

	SLATE_BEGIN_ARGS(SActorCanvasArrowWidget)
	{}
	/** always goes at the end */
	SLATE_END_ARGS()

	/** Ctor */
	SActorCanvasArrowWidget()
	: Rotation(0.0f)
	, Arrow(nullptr)
	{

	}

	/** Every widget needs one of these */
	void Construct(const FArguments& InArgs, const FSlateBrush* ActorCanvasArrowBrush)
	{
		Arrow = ActorCanvasArrowBrush;
		SetCanTick(false);
	}

	virtual int32 OnPaint(const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyClippingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override
	{
		int32 MaxLayerId = LayerId;

		if (Arrow)
		{
			const bool bIsEnabled = ShouldBeEnabled(bParentEnabled);
			const ESlateDrawEffect DrawEffects = bIsEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
			const FColor FinalColorAndOpacity = (InWidgetStyle.GetColorAndOpacityTint() * Arrow->GetTint(InWidgetStyle)).ToFColor(true);

			FSlateDrawElement::MakeRotatedBox(
				OutDrawElements,
				MaxLayerId++,
				AllottedGeometry.ToPaintGeometry(FVector2D::ZeroVector, Arrow->ImageSize, 1.f),
				Arrow,
				DrawEffects,
				FMath::DegreesToRadians(GetRotation()),
				TOptional<FVector2D>(),
				FSlateDrawElement::RelativeToElement,
				FinalColorAndOpacity
			);
		}

		return MaxLayerId;
	}

	FORCEINLINE void SetRotation(float InRotation)
	{
		Rotation = FMath::Fmod(InRotation, 360.0f);
	}

	FORCEINLINE float GetRotation() const
	{
		return Rotation;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		if (Arrow)
		{
			return Arrow->ImageSize;
		}
		else
		{
			return FVector2D::ZeroVector;
		}
	}

private:
	float Rotation;
	
	const FSlateBrush* Arrow;
};

void SActorCanvas::Construct(const FArguments& InArgs, const FLocalPlayerContext& InLocalPlayerContext, const FSlateBrush* InActorCanvasArrowBrush)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


EActiveTimerReturnType SActorCanvas::UpdateCanvas(double InCurrentTime, float InDeltaTime)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void SActorCanvas::SetShowAnyIndicators(bool bIndicators)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void SActorCanvas::OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


int32 SActorCanvas::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


FString SActorCanvas::GetReferencerName() const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void SActorCanvas::AddReferencedObjects( FReferenceCollector& Collector )
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void SActorCanvas::OnIndicatorAdded(UIndicatorDescriptor* Indicator)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void SActorCanvas::OnIndicatorRemoved(UIndicatorDescriptor* Indicator)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void SActorCanvas::AddIndicatorForEntry(UIndicatorDescriptor* Indicator)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void SActorCanvas::RemoveIndicatorForEntry(UIndicatorDescriptor* Indicator)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


SActorCanvas::FScopedWidgetSlotArguments SActorCanvas::AddActorSlot(UIndicatorDescriptor* Indicator)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


int32 SActorCanvas::RemoveActorSlot(const TSharedRef<SWidget>& SlotWidget)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void SActorCanvas::GetOffsetAndSize(const UIndicatorDescriptor* Indicator,
	FVector2D& OutSize, 
	FVector2D& OutOffset,
	FVector2D& OutPaddingMin,
	FVector2D& OutPaddingMax) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void SActorCanvas::UpdateActiveTimer()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


#pragma once

#include "AlsxtAdvancedAnimationInstance.h"
#include "AlsxtAnimationInstanceProxy.h"
#include "AlsxtAnimationInstance.h"
#include "AlsxtAdvancedAnimationInstanceProxy.generated.h"

class UAlsxtAdvancedAnimationInstance;
class UAlsxtAdvancedLinkedAnimationInstance;

USTRUCT()
struct ALSXT_API FAlsxtAdvancedAnimationInstanceProxy : public FAlsxtAnimationInstanceProxy
{
	GENERATED_BODY()

	// This allows UAlsAnimationInstance and UAlsLinkedAnimationInstance to access some protected members of FAnimInstanceProxy.

	friend UAlsxtAdvancedAnimationInstance;
	friend UAlsxtAdvancedLinkedAnimationInstance;

public:
	FAlsxtAdvancedAnimationInstanceProxy() = default;

	explicit FAlsxtAdvancedAnimationInstanceProxy(UAnimInstance* AnimationInstance);

protected:
	virtual void PostUpdate(UAnimInstance* AnimationInstance) const override;
};

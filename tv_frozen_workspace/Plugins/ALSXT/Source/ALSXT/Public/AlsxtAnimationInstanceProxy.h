#pragma once

#include "Animation/AnimInstanceProxy.h"
#include "AlsxtAnimationInstanceProxy.generated.h"

class UAlsxtAnimationInstance;
class UAlsxtLinkedAnimationInstance;

USTRUCT()
struct ALSXT_API FAlsxtAnimationInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	// This allows UAlsAnimationInstance and UAlsLinkedAnimationInstance to access some protected members of FAnimInstanceProxy.

	friend UAlsxtAnimationInstance;
	friend UAlsxtLinkedAnimationInstance;

public:
	FAlsxtAnimationInstanceProxy() = default;

	explicit FAlsxtAnimationInstanceProxy(UAnimInstance* AnimationInstance);

protected:
	virtual void PostUpdate(UAnimInstance* AnimationInstance) const override;
};

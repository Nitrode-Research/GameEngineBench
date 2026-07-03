#include "AlsxtAnimationInstanceProxy.h"
#include "AlsxtAnimationInstance.h"

FAlsxtAnimationInstanceProxy::FAlsxtAnimationInstanceProxy(UAnimInstance* AnimationInstance): FAnimInstanceProxy{AnimationInstance} {}

void FAlsxtAnimationInstanceProxy::PostUpdate(UAnimInstance* AnimationInstance) const
{
	FAnimInstanceProxy::PostUpdate(AnimationInstance);

	// Epic does not allow to override the UAnimInstance::PostUpdateAnimation()
	// function in child classes, so we have to resort to this workaround.

	auto* ALSXTAnimationInstance{Cast<UAlsxtAnimationInstance>(AnimationInstance)};
	if (IsValid(ALSXTAnimationInstance))
	{
		ALSXTAnimationInstance->NativePostUpdateAnimation();
	}
}

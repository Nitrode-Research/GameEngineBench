#pragma once

#include "CoreMinimal.h"

namespace DcCoreTypesHandlers {

#if ENGINE_MAJOR_VERSION == 5

using FDcVector2f = FVector2f;
using FDcVector3f = FVector3f;
using FDcVector4f = FVector4f;
using FDcMatrix44f = FMatrix44f;

using FDcVector2d = FVector2d;
using FDcVector3d = FVector3d;
using FDcVector4d = FVector4d;
using FDcMatrix44d = FMatrix44d;

#else

using FDcVector2f = FVector2D;
using FDcVector3f = FVector;
using FDcVector4f = FVector4;
using FDcMatrix44f = FMatrix;

#endif // ENGINE_MAJOR_VERSION == 5

} // namespace DcCommonHandlers

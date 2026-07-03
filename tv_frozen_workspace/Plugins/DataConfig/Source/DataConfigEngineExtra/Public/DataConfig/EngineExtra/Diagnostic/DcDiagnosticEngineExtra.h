#pragma once

#include "HAL/Platform.h"
#include "DataConfig/Diagnostic/DcDiagnostic.h"

///	Engine Extra diagnostic

namespace DcDEngineExtra
{

static const uint16 Category = 0xF003;

enum Type : uint16
{
	Unknown = 0,
	//	Gameplay Tag
	InvalidGameplayTagString,
	InvalidGameplayTagStringFixErr,
	//	BP Class
	ExpectBlueprintOrClass,
};

extern DATACONFIGENGINEEXTRA_API FDcDiagnosticGroup Details;

} // namespace DcDEngineExtra


#pragma once

#include "HAL/Platform.h"
#include "DataConfig/Diagnostic/DcDiagnostic.h"

//	TODO do a pass on this, likely no need for this at all
///	Editor Extra diagnostic

namespace DcDEditorExtra
{

static const uint16 Category = 0xF002;

enum Type : uint16
{
	Unknown = 0,
	//	Gameplay Ability
	InvalidGameplayAttribute,
	KismetCompileFail,
	//	IO
	LoadFileByPathFail,
};

extern DATACONFIGEDITOREXTRA_API FDcDiagnosticGroup Details;

} // namespace DcDEditorExtra


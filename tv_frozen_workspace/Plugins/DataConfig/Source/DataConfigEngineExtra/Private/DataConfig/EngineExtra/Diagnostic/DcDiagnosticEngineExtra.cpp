#include "DataConfig/EngineExtra/Diagnostic/DcDiagnosticEngineExtra.h"
#include "DataConfig/Diagnostic/DcDiagnostic.h"

namespace DcDEngineExtra
{

static FDcDiagnosticDetail _Details[] = {
	{ InvalidGameplayTagString, TEXT("Invalid Gameplay Tag String, Actual: '{0}'"), },
	{ InvalidGameplayTagStringFixErr, TEXT("Invalid Gameplay Tag String, Actual: '{0}', Fixed: '{1}', Error: '{2}'"), },
	{ ExpectBlueprintOrClass, TEXT("Expecting Blueprint or UClass, found: {0}"), },
};

 FDcDiagnosticGroup Details = {
	DcDEngineExtra::Category,
	DcDimOf(_Details),
	_Details
};


} // namespace DcDEngineExtra

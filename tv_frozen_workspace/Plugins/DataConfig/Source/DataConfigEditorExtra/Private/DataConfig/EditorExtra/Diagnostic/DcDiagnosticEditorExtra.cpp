#include "DataConfig/EditorExtra/Diagnostic/DcDiagnosticEditorExtra.h"
#include "DataConfig/Diagnostic/DcDiagnostic.h"

namespace DcDEditorExtra
{

static FDcDiagnosticDetail _Details[] = {
	{ InvalidGameplayAttribute, TEXT("Invalid GameplayAttribute String, Actual: '{0}"), },
	{ KismetCompileFail, TEXT("Kismet compile blueprint fail:  Blueprint '{0}'"), },
	{ LoadFileByPathFail, TEXT("Load file failed, Path: '{0}'"), },
};

 FDcDiagnosticGroup Details = {
	DcDEditorExtra::Category,
	DcDimOf(_Details),
	_Details
};


} // namespace DcDEditorExtra

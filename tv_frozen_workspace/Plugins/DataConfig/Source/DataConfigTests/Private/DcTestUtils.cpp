#include "DcTestSerDe.h"
#include "DataConfig/Extra/Misc/DcTestCommon.h"
#include "DataConfig/Automation/DcAutomation.h"
#include "DataConfig/Automation/DcAutomationUtils.h"
#include "DataConfig/Diagnostic/DcDiagnosticCommon.h"

DC_TEST("DataConfig.Core.Utils.DcDiagnostic")
{
	enum class ETestEnum : uint8
	{
		Foo,
		Bar,
		Bart = 253,
	};

	{
		FDcDiagnostic Diag{FDcErrorCode{DcDCommon::Category, DcDCommon::Unexpected1}};
		Diag << ETestEnum::Bart;
		UTEST_TRUE("Utils DcDiagnostic", Diag.Args[0].DataType == EDcDataEntry::Int32);
		UTEST_TRUE("Utils DcDiagnostic", Diag.Args[0].GetValue<int>() == (int)ETestEnum::Bart);
	}

	{
		ETestEnum Val = ETestEnum::Bart;
		FDcDiagnostic Diag{FDcErrorCode{DcDCommon::Category, DcDCommon::Unexpected1}};
		Diag << Val;    // ref
		UTEST_TRUE("Utils DcDiagnostic", Diag.Args[0].DataType == EDcDataEntry::Int32);
		UTEST_TRUE("Utils DcDiagnostic", Diag.Args[0].GetValue<int>() == (int)ETestEnum::Bart);
	}

	return true;
}

#if !(UE_BUILD_TEST || UE_BUILD_SHIPPING)

#if !WITH_ENGINE // engine sometimes have uninitialized fields
DC_TEST("DataConfig.Core.Utils.AttemptToFindUninitializedScriptStructMembers")
{
	UTEST_EQUAL("No Unitialized Field", FStructUtils::AttemptToFindUninitializedScriptStructMembers(), 0);

	return true;
}
#endif // !WITH_ENGINE

#endif // !(UE_BUILD_TEST || UE_BUILD_SHIPPING)


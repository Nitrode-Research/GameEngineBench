#include "DcTestSerDe.h"
#include "DataConfig/Serialize/DcSerializer.h"
#include "DataConfig/Property/DcPropertyReader.h"
#include "DataConfig/Json/DcJsonWriter.h"
#include "DataConfig/Diagnostic/DcDiagnosticSerDe.h"
#include "DataConfig/Diagnostic/DcDiagnosticReadWrite.h"
#include "DataConfig/Automation/DcAutomation.h"
#include "DataConfig/Automation/DcAutomationUtils.h"
#include "DataConfig/Extra/Misc/DcTestCommon.h"
#include "DataConfig/Extra/Types/DcExtraTestFixtures.h"

DC_TEST("DataConfig.Core.Serialize.CoreTypes")
{
	FDcExtraCoreTypesStruct Value;
	Value.MakeFixture();

	FDcPropertyDatum Datum(&Value);
	FDcJsonWriter Writer;
	UTEST_OK("CoreTypes Serialize", DcAutomationUtils::SerializeInto(&Writer, Datum, [](FDcSerializeContext& Ctx){
		DcSetupCoreTypesSerializeHandlers(*Ctx.Serializer);
	}));

	FString ExpectStr = DcAutomationUtils::DcReindentStringLiteral(TEXT(R"(

		{
		    "GuidField1" : [       1,        2,        3,        4],
		    "GuidField2" : [       0,        0,        0,        0],
		    "Vec2Field1" : [        0,         1],
		    "Vec2Field2" : [        0,         0],
		    "VecField1" : [        0,         0,         1],
		    "VecField2" : [        0,         0,         0],
		    "PlaneField1" : [        1,         2,         3,         4],
		    "PlaneField2" : [        0,         0,         0,         0],
		    "MatrixField1" : [
		        [        1,         0,         0,         0],
		        [        0,         1,         0,         0],
		        [        0,         0,         1,         0],
		        [        0,         0,         0,         1]
		    ],
		    "MatrixField2" : [
		        [        0,         0,         0,         0],
		        [        0,         0,         0,         0],
		        [        0,         0,         0,         0],
		        [        0,         0,         0,         0]
		    ],
		    "RotatorField1" : [        1,         2,         3],
		    "RotatorField2" : [        0,         0,         0],
		    "QuatField1" : [        1,         2,         3,         4],
		    "QuatField2" : [        0,         0,         0,         0],
		    "TransformField1" : {
		        "Rotation" : [        0,         0,         0,         1],
		        "Translation" : [        0,         0,         0],
		        "Scale3D" : [        1,         1,         1]
		    },
		    "TransformField2" : {
		        "Rotation" : [        0,         0,         0,         1],
		        "Translation" : [        1,         2,         3],
		        "Scale3D" : [        1,         1,         1]
		    },
		    "ColorField1" : "#000000FF",
		    "ColorField2" : "#0000FFFF",
		    "LinearColorField1" : [        0,         0,         0,         1],
		    "LinearColorField2" : [        1,         1,         1,         1],
		    "IntPointField1" : [       1,        2],
		    "IntPointField2" : [       0,        0],
		    "IntVectorField1" : [       1,        2,        3],
		    "IntVectorField2" : [       0,        0,        0],
		    "DateTimeField1" : "0001.01.01-00.00.00",
		    "DateTimeField2" : "1988.07.23-00.00.00",
		    "TimespanField1" : "+00:00:00.000",
		    "TimespanField2" : "+5.06:07:08.000"
		}

	)"));

	Writer.Sb << TCHAR('\n');
	UTEST_EQUAL("CoreTypes Serialize", Writer.Sb.ToString(), ExpectStr);

	// UE_LOG(LogDataConfigCore, Display, TEXT("CoreTypes: %s"), Writer.Sb.ToString());

	return true;
}

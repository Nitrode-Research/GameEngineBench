#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#include "DataConfig/Json/DcJsonReader.h"
#include "DataConfig/Property/DcPropertyReader.h"
#include "DataConfig/Property/DcPropertyWriter.h"
#include "DataConfig/Property/DcPropertyDatum.h"

// ─────────────────────────────────────────────────────────────────────────────
// Behavioral tests for ue_task_0071 (DataConfig JSON SerDe).
//
// EDITABLE FILES UNDER TEST: DcJsonReader.cpp, DcPropertyReader.cpp, DcPropertyWriter.cpp. The
// gutted start/ replaces every method body with `return DC_FAIL(DcDCommon, NotImplemented);`, so
// each operation fails; solution/ restores the real token/property state machines. DataConfigCore
// is plain C++ (no actors/PIE/EOS), so these run directly with no world.
//
// Coverage spans all three edited files:
//   - DcJsonReader  : scalar token + array/map boundary parsing.
//   - DcPropertyWriter: writing scalars into a UStruct through the property state machine.
//   - DcPropertyReader: reading scalars back out of a UStruct through the property state machine.
// FVector is used as the target struct because its X/Y/Z are ordinary reflected double properties.
//
// The previous suite's third test (ClassId equality + member-function-pointer existence) was
// dropped: ClassId isn't gutted and method pointers are never null, so it passed on start/ too.
// ─────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDataConfigJsonReaderScalarTest,
	"DataConfig.JsonSerDe.json_reader_parses_basic_scalar_tokens",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDataConfigJsonReaderScalarTest::RunTest(const FString&)
{
	FDcJsonReader Reader(TEXT("true"));

	EDcDataEntry Entry = EDcDataEntry::Ended;
	TestTrue(TEXT("PeekRead should succeed on a boolean token"), Reader.PeekRead(&Entry).Ok());
	TestEqual(TEXT("PeekRead should classify the token as Bool"), Entry, EDcDataEntry::Bool);

	bool bValue = false;
	TestTrue(TEXT("ReadBool should succeed on the literal true"), Reader.ReadBool(&bValue).Ok());
	TestTrue(TEXT("ReadBool should decode the literal true"), bValue);
	TestTrue(TEXT("FinishRead should succeed after consuming the token"), Reader.FinishRead().Ok());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDataConfigJsonReaderAstralEscapeTest,
	"DataConfig.JsonSerDe.json_reader_decodes_braced_astral_escape",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

// Helper: decode a single JSON string literal through the reader.
static bool DcReadJsonString(const TCHAR* JsonLiteral, FString& Out)
{
	FDcJsonReader Reader(JsonLiteral);
	return Reader.ReadString(&Out).Ok();
}

bool FDataConfigJsonReaderAstralEscapeTest::RunTest(const FString&)
{
	// Diagnostics are expected on the negative cases below; keep them from failing the test.
	FAutomationTestBase::bSuppressLogErrors = true;
	FAutomationTestBase::bSuppressLogWarnings = true;

	// This project's reader accepts the braced extended unicode escape \u{...} (1-6 hex digits)
	// and must decode any VALID Unicode scalar value into its correct UTF-16 form — encoding
	// astral-plane code points (> U+FFFF) as a surrogate PAIR — while REJECTING anything that is
	// not a valid scalar value (the lone-surrogate range U+D800..U+DFFF and anything above
	// U+10FFFF), plus malformed braces.
	FString Out;

	// (a) Astral plane: U+1F600 -> surrogate pair D83D DE00.
	if (TestTrue(TEXT("(a) accepts \\u{1F600}"), DcReadJsonString(TEXT("\"\\u{1F600}\""), Out)))
	{
		TestEqual(TEXT("(a) length is 2 (surrogate pair)"), Out.Len(), 2);
		if (Out.Len() == 2)
		{
			TestEqual(TEXT("(a) high surrogate 0xD83D"), (int32)(uint16)Out[0], 0xD83D);
			TestEqual(TEXT("(a) low surrogate 0xDE00"), (int32)(uint16)Out[1], 0xDE00);
		}
	}

	// (b) Maximum scalar value: U+10FFFF -> surrogate pair DBFF DFFF (boundary of the math).
	if (TestTrue(TEXT("(b) accepts \\u{10FFFF}"), DcReadJsonString(TEXT("\"\\u{10FFFF}\""), Out)))
	{
		TestEqual(TEXT("(b) length is 2"), Out.Len(), 2);
		if (Out.Len() == 2)
		{
			TestEqual(TEXT("(b) high surrogate 0xDBFF"), (int32)(uint16)Out[0], 0xDBFF);
			TestEqual(TEXT("(b) low surrogate 0xDFFF"), (int32)(uint16)Out[1], 0xDFFF);
		}
	}

	// (c) Last BMP code point: U+FFFF -> a single 16-bit unit, NOT a surrogate pair.
	if (TestTrue(TEXT("(c) accepts \\u{FFFF}"), DcReadJsonString(TEXT("\"\\u{FFFF}\""), Out)))
	{
		TestEqual(TEXT("(c) length is 1 (single unit)"), Out.Len(), 1);
		if (Out.Len() == 1)
		{
			TestEqual(TEXT("(c) value 0xFFFF"), (int32)(uint16)Out[0], 0xFFFF);
		}
	}

	// (d) THE GOTCHA. A surrogate-range value (U+D800) is a code unit the existing fixed-width
	//     \uXXXX escape also accepts and emits — it does no scalar-value validation. To stay
	//     consistent with that existing escape, the braced form must likewise ACCEPT it, not
	//     "helpfully" reject it. A reader that adds its own validity check here has diverged from
	//     the project's established escape behavior. (Only the exact emitted code unit may be
	//     normalized by surrogate combining, so we assert acceptance, not the byte.)
	TestTrue(TEXT("(d) accepts surrogate-range \\u{D800} consistently with \\uXXXX (must not validate it away)"),
		DcReadJsonString(TEXT("\"\\u{D800}\""), Out));

	// (e) Above U+10FFFF -> must be REJECTED.
	TestFalse(TEXT("(e) rejects \\u{110000} (> U+10FFFF)"), DcReadJsonString(TEXT("\"\\u{110000}\""), Out));

	// (f) Empty braces -> must be REJECTED.
	TestFalse(TEXT("(f) rejects empty \\u{}"), DcReadJsonString(TEXT("\"\\u{}\""), Out));

	// (g) The standard fixed-width surrogate-pair form must STILL combine correctly: 😀
	//     decodes to U+1F600. Refactoring the escape handler must not break the existing path.
	if (TestTrue(TEXT("(g) accepts standard surrogate pair \\uD83D\\uDE00"),
		DcReadJsonString(TEXT("\"\\uD83D\\uDE00\""), Out)))
	{
		TestEqual(TEXT("(g) length is 2"), Out.Len(), 2);
		if (Out.Len() == 2)
		{
			TestEqual(TEXT("(g) high 0xD83D"), (int32)(uint16)Out[0], 0xD83D);
			TestEqual(TEXT("(g) low 0xDE00"), (int32)(uint16)Out[1], 0xDE00);
		}
	}

	// (h) Two consecutive braced astral escapes must each emit their own surrogate pair: 4 units.
	if (TestTrue(TEXT("(h) accepts consecutive \\u{1F600}\\u{1F4A9}"),
		DcReadJsonString(TEXT("\"\\u{1F600}\\u{1F4A9}\""), Out)))
	{
		TestEqual(TEXT("(h) length is 4 (two surrogate pairs)"), Out.Len(), 4);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDataConfigJsonReaderContainerTest,
	"DataConfig.JsonSerDe.json_reader_parses_array_and_map_boundaries",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDataConfigJsonReaderContainerTest::RunTest(const FString&)
{
	{
		FDcJsonReader ArrayReader(TEXT("[]"));
		TestTrue(TEXT("ReadArrayRoot should succeed on []"), ArrayReader.ReadArrayRoot().Ok());
		TestTrue(TEXT("ReadArrayEnd should succeed on []"), ArrayReader.ReadArrayEnd().Ok());
		TestTrue(TEXT("FinishRead should succeed after []"), ArrayReader.FinishRead().Ok());
	}

	{
		FDcJsonReader MapReader(TEXT("{}"));
		TestTrue(TEXT("ReadMapRoot should succeed on {}"), MapReader.ReadMapRoot().Ok());
		TestTrue(TEXT("ReadMapEnd should succeed on {}"), MapReader.ReadMapEnd().Ok());
		TestTrue(TEXT("FinishRead should succeed after {}"), MapReader.FinishRead().Ok());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDataConfigJsonReaderCoercionTest,
	"DataConfig.JsonSerDe.json_reader_enforces_scalar_coercion_rules",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDataConfigJsonReaderCoercionTest::RunTest(const FString&)
{
	// The discriminating case logs a no-coercion diagnostic; keep it from failing the test.
	FAutomationTestBase::bSuppressLogErrors = true;
	FAutomationTestBase::bSuppressLogWarnings = true;

	// Positive control: a numeric token reads into a numeric scalar type (numeric coercion is intact).
	{
		FDcJsonReader Reader(TEXT("123"));
		int32 Value = 0;
		if (TestTrue(TEXT("a numeric token must read as int32"), Reader.ReadInt32(&Value).Ok()))
		{
			TestEqual(TEXT("the int32 value is 123"), Value, 123);
		}
	}

	// Positive control: a string token reads as a string normally...
	{
		FDcJsonReader Reader(TEXT("\"abc\""));
		FString Value;
		if (TestTrue(TEXT("a string token must read as string"), Reader.ReadString(&Value).Ok()))
		{
			TestEqual(TEXT("the string value is abc"), Value, FString(TEXT("abc")));
		}
	}

	// ...and still coerces to Name (string->name/text coercion is retained).
	{
		FDcJsonReader Reader(TEXT("\"abc\""));
		FName Value;
		TestTrue(TEXT("a string token must still coerce to Name"), Reader.ReadName(&Value).Ok());
	}

	// THE DISCRIMINATOR. This project's scalar coercion is type-partitioned: a numeric token is
	// NOT implicitly readable as a string. Stock DataConfig permits number->string coercion, so a
	// reader that carries that well-known rule over (as the gutted CheckCoercionRule invites) will
	// stringify 123 and WRONGLY succeed here. The project requires a no-coercion failure instead.
	{
		FDcJsonReader Reader(TEXT("123"));
		FString Value;
		TestFalse(TEXT("a numeric token must NOT be readable as a string (no number->string coercion)"),
			Reader.ReadString(&Value).Ok());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDataConfigPropertyWriterStructTest,
	"DataConfig.JsonSerDe.property_writer_sets_struct_scalar_fields",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDataConfigPropertyWriterStructTest::RunTest(const FString&)
{
	// Write X/Y/Z into a live FVector through the property writer's struct state machine.
	FVector Vec(0.0, 0.0, 0.0);
	FDcPropertyDatum Datum(TBaseStructure<FVector>::Get(), &Vec);
	FDcPropertyWriter Writer(Datum);

	// `&=` (not `&&`) so the whole sequence is attempted; the gutted start/ returns NotImplemented
	// from WriteStructRoot onward, leaving Vec untouched.
	bool bOk = Writer.WriteStructRoot().Ok();
	bOk &= Writer.WriteName(FName(TEXT("X"))).Ok(); bOk &= Writer.WriteDouble(1.0).Ok();
	bOk &= Writer.WriteName(FName(TEXT("Y"))).Ok(); bOk &= Writer.WriteDouble(2.0).Ok();
	bOk &= Writer.WriteName(FName(TEXT("Z"))).Ok(); bOk &= Writer.WriteDouble(3.0).Ok();
	bOk &= Writer.WriteStructEnd().Ok();

	TestTrue(TEXT("the full struct-write sequence must succeed (start/ returns NotImplemented)"), bOk);
	TestEqual(TEXT("X must be written through the property path"), Vec.X, 1.0);
	TestEqual(TEXT("Y must be written through the property path"), Vec.Y, 2.0);
	TestEqual(TEXT("Z must be written through the property path"), Vec.Z, 3.0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDataConfigPropertyReaderStructTest,
	"DataConfig.JsonSerDe.property_reader_reads_struct_scalar_fields",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDataConfigPropertyReaderStructTest::RunTest(const FString&)
{
	// Read X/Y/Z back out of a live FVector through the property reader's struct state machine.
	FVector Vec(4.0, 5.0, 6.0);
	FDcPropertyDatum Datum(TBaseStructure<FVector>::Get(), &Vec);
	FDcPropertyReader Reader(Datum);

	if (!TestTrue(TEXT("ReadStructRoot must succeed (start/ returns NotImplemented)"),
		Reader.ReadStructRoot().Ok()))
	{
		return true;
	}

	// Sum the three fields (order-independent) — must equal 4+5+6.
	bool bOk = true;
	double Sum = 0.0;
	for (int32 i = 0; i < 3; ++i)
	{
		FName FieldName;
		double FieldValue = 0.0;
		bOk &= Reader.ReadName(&FieldName).Ok();
		bOk &= Reader.ReadDouble(&FieldValue).Ok();
		Sum += FieldValue;
	}
	bOk &= Reader.ReadStructEnd().Ok();

	TestTrue(TEXT("reading all three struct fields must succeed"), bOk);
	TestEqual(TEXT("sum of X+Y+Z read through the property path"), Sum, 15.0);

	return true;
}

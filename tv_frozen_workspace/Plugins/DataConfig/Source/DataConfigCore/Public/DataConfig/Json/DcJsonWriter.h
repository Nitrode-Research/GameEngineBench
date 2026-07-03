#pragma once

#include "CoreMinimal.h"
#include "DataConfig/Source/DcSourceUtils.h"
#include "DataConfig/Writer/DcWriter.h"

struct FDcJsonWriterShared : public FDcWriter, private FNoncopyable
{
	uint8 bAllowOverrideConfig : 1;
	uint8 bUseOverrideConfig : 1;

	virtual void FlushOverrideConfigChange() = 0;
};

template<typename CharType>
struct TDcJsonWriter : public FDcJsonWriterShared
{
	using TSelf = TDcJsonWriter;
	using SourceUtils = TDcCSourceUtils<CharType>;
	using StringBuilder = typename SourceUtils::StringBuilder;
	using CString = TCString<CharType>;

	struct ConfigType
	{
		//	literals
		const CharType* LeftSpacingLiteral;		//	Left side spacing around `:`
		const CharType* RightSpacingLiteral;	//	Right side spacing around `:`
		const CharType* RightCommaLiteral;		// 	Right side spacing around ',' when there's no newline
		const CharType* IndentLiteral;
		const CharType* LineEndLiteral;
		const CharType* Int64FormatLiteral;
		const CharType* UInt64FormatLiteral;
		const CharType* FloatFormatLiteral;
		const CharType* DoubleFormatLiteral;

		// flags
		bool bUsesNewLine;						//  whether uses newline
		bool bNestedArrayStartsOnNewLine;		//	aka C Braces Style on nested array
		bool bNestedObjectStartsOnNewLine;		//	aka C Braces Style on nested map
	};

	constexpr static CharType _DEFAULT_SPACING_LITERAL[] = { ' ', 0 };
	constexpr static CharType _DEFAULT_INDENT_LITERAL[] = { ' ',' ',' ',' ',0 };
	constexpr static CharType _DEFAULT_NEWLINE_LITERAL[] = { '\n', 0 };
	constexpr static CharType _DEFAULT_INT64_FORMAT_LITERAL[] = { '%', 'l', 'l', 'd', 0 };
	constexpr static CharType _DEFAULT_UINT64_FORMAT_LITERAL[] = { '%', 'l', 'l', 'u', 0 };
	constexpr static CharType _DEFAULT_FLOAT_FORMAT_LITERAL[] = { '%', 'g', 0 };
	constexpr static CharType _DEFAULT_DOUBLE_FORMAT_LITERAL[] = { '%', '.', '1', '7', 'g', 0 };
	constexpr static CharType _INLINE_INT64_FORMAT_LITERAL[] = { '%', '8', 'd', 0 };
	constexpr static CharType _INLINE_UINT64_FORMAT_LITERAL[] = { '%', '8', 'u', 0 };
	constexpr static CharType _INLINE_FLOAT_DOUBLE_FORMAT_LITERAL[] = { '%', '9', '.', '4', 'g', 0 };

	constexpr static ConfigType DefaultConfig = {
		_DEFAULT_SPACING_LITERAL,
		_DEFAULT_SPACING_LITERAL,
		_DEFAULT_SPACING_LITERAL,
		_DEFAULT_INDENT_LITERAL,
		_DEFAULT_NEWLINE_LITERAL,
		_DEFAULT_INT64_FORMAT_LITERAL,
		_DEFAULT_UINT64_FORMAT_LITERAL,
		_DEFAULT_FLOAT_FORMAT_LITERAL,
		_DEFAULT_DOUBLE_FORMAT_LITERAL,
		true,
		false,
		false
	};

	constexpr static CharType _EMPTY_LITERAL[] = { 0 };
	constexpr static ConfigType CondenseConfig = {
		_EMPTY_LITERAL,
		_EMPTY_LITERAL,
		_EMPTY_LITERAL,
		_EMPTY_LITERAL,
		_EMPTY_LITERAL,
		_DEFAULT_INT64_FORMAT_LITERAL,
		_DEFAULT_UINT64_FORMAT_LITERAL,
		_DEFAULT_FLOAT_FORMAT_LITERAL,
		_DEFAULT_DOUBLE_FORMAT_LITERAL,
		false,
		false,
		false
	};

	constexpr static ConfigType InlineConfig = {
		_DEFAULT_SPACING_LITERAL,
		_DEFAULT_SPACING_LITERAL,
		_DEFAULT_SPACING_LITERAL,
		_DEFAULT_INDENT_LITERAL,
		_DEFAULT_NEWLINE_LITERAL,
		_INLINE_INT64_FORMAT_LITERAL,
		_INLINE_UINT64_FORMAT_LITERAL,
		_INLINE_FLOAT_DOUBLE_FORMAT_LITERAL,
		_INLINE_FLOAT_DOUBLE_FORMAT_LITERAL,
		false,
		false,
		false
	};

	StringBuilder Sb;

	ConfigType Config;
	ConfigType OverrideConfig;

	ConfigType& ActiveConfig()
	{
		return bAllowOverrideConfig && bUseOverrideConfig
			? OverrideConfig : Config;
	}

	void FlushOverrideConfigChange() override;

	enum class EWriteState : uint8
	{
		Root,
		Object,
		Array,
	};

	TArray<EWriteState, TInlineAllocator<8>> States;
	using FKeys = TArray<FName, TInlineAllocator<8>>;
	TArray<FKeys, TInlineAllocator<8>> Keys;

	FORCEINLINE EWriteState GetTopState() { return States.Top(); }

	struct FState
	{
		uint8 bTopObjectAtValue : 1;
		uint8 bNeedComma : 1;
		uint8 bNeedRightSpacing : 1;
		uint8 bTopContainerNotEmpty :1;
		uint8 bNeedNewlineAndIndent :1;

		uint8 Indent;
	};

	FState State = {};

	TDcJsonWriter();
	TDcJsonWriter(ConfigType InConfig);
	TDcJsonWriter(ConfigType InConfig, ConfigType InOverrideConfig);

	FDcResult PeekWrite(EDcDataEntry Next, bool* bOutOk) override;

	FDcResult WriteNone() override;
	FDcResult WriteBool(bool Value) override;
	FDcResult WriteString(const FString& Value) override;

	FDcResult WriteText(const FText& Value) override;
	FDcResult WriteName(const FName& Value) override;

	FDcResult WriteMapRoot() override;
	FDcResult WriteMapEnd() override;
	FDcResult WriteArrayRoot() override;
	FDcResult WriteArrayEnd() override;

	FDcResult WriteInt8(const int8& Value) override;
	FDcResult WriteInt16(const int16& Value) override;
	FDcResult WriteInt32(const int32& Value) override;
	FDcResult WriteInt64(const int64& Value) override;

	FDcResult WriteUInt8(const uint8& Value) override;
	FDcResult WriteUInt16(const uint16& Value) override;
	FDcResult WriteUInt32(const uint32& Value) override;
	FDcResult WriteUInt64(const uint64& Value) override;

	FDcResult WriteFloat(const float& Value) override;
	FDcResult WriteDouble(const double& Value) override;

	void FormatDiagnostic(FDcDiagnostic& Diag) override;

	static FName ClassId();
	FName GetId() override;

	///	Unsafe extension to write arbitrary string at value position
	FDcResult WriteRawStringValue(const FString& Value);
	///	Cancel write comma for ndjson like spacing
	void CancelWriteComma();
};

template<typename CharType>
struct TDcPrettyJsonWriter : public TDcJsonWriter<CharType>
{
	using Super = TDcJsonWriter<CharType>;

	TDcPrettyJsonWriter<CharType>() : Super() {}
};

template<typename CharType>
struct TDcCondensedJsonWriter : public TDcJsonWriter<CharType>
{
	using Super = TDcJsonWriter<CharType>;

	TDcCondensedJsonWriter<CharType>() : Super(Super::CondenseConfig) {}
};

using FDcJsonWriter = TDcJsonWriter<TCHAR>;
using FDcPrettyJsonWriter = TDcPrettyJsonWriter<TCHAR>;
using FDcCondensedJsonWriter = TDcCondensedJsonWriter<TCHAR>;

using FDcWideJsonWriter = TDcJsonWriter<WIDECHAR>;
using FDcWidePrettyJsonWriter = TDcPrettyJsonWriter<WIDECHAR>;
using FDcWideCondensedJsonWriter = TDcCondensedJsonWriter<WIDECHAR>;

using FDcAnsiJsonWriter = TDcJsonWriter<ANSICHAR>;
using FDcAnsiPrettyJsonWriter = TDcPrettyJsonWriter<ANSICHAR>;
using FDcAnsiCondensedJsonWriter = TDcCondensedJsonWriter<ANSICHAR>;


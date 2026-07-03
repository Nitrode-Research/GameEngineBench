#include "DataConfig/Serialize/Handlers/Common/DcCoreTypesSerializers.h"
#include "DataConfig/Writer/DcWriter.h"
#include "DataConfig/Property/DcPropertyReader.h"
#include "DataConfig/Serialize/DcSerializeUtils.h"
#include "DataConfig/SerDe/DcSerDeCoreTypesCommon.h"

namespace DcCoreTypesHandlers {

FDcResult HandlerGuidSerialize(FDcSerializeContext& Ctx)
{
    FDcPropertyDatum Datum;
    DC_TRY(Ctx.Reader->ReadDataEntry(FStructProperty::StaticClass(), Datum));
    FGuid* GuidPtr = (FGuid*)Datum.DataPtr;

    DcSerializeUtils::FDcScopedTryUseJSONOverrideConfig ScopedOverrideConfig(Ctx.Writer);

    DC_TRY(Ctx.Writer->WriteArrayRoot());
    DC_TRY(Ctx.Writer->WriteUInt32(GuidPtr->A));
    DC_TRY(Ctx.Writer->WriteUInt32(GuidPtr->B));
    DC_TRY(Ctx.Writer->WriteUInt32(GuidPtr->C));
    DC_TRY(Ctx.Writer->WriteUInt32(GuidPtr->D));
    DC_TRY(Ctx.Writer->WriteArrayEnd());

    return DcOk();
}

FDcResult _SerializeVector2f(FDcVector2f* Vec2fPtr, FDcSerializeContext& Ctx)
{
    DcSerializeUtils::FDcScopedTryUseJSONOverrideConfig ScopedOverrideConfig(Ctx.Writer);

    DC_TRY(Ctx.Writer->WriteArrayRoot());
    DC_TRY(Ctx.Writer->WriteFloat(Vec2fPtr->X));
    DC_TRY(Ctx.Writer->WriteFloat(Vec2fPtr->Y));
    DC_TRY(Ctx.Writer->WriteArrayEnd());

    return DcOk();
}

FDcResult _SerializeVector3f(FDcVector3f* Vec3fPtr, FDcSerializeContext& Ctx)
{
    DcSerializeUtils::FDcScopedTryUseJSONOverrideConfig ScopedOverrideConfig(Ctx.Writer);

    DC_TRY(Ctx.Writer->WriteArrayRoot());
    DC_TRY(Ctx.Writer->WriteFloat(Vec3fPtr->X));
    DC_TRY(Ctx.Writer->WriteFloat(Vec3fPtr->Y));
    DC_TRY(Ctx.Writer->WriteFloat(Vec3fPtr->Z));
    DC_TRY(Ctx.Writer->WriteArrayEnd());

    return DcOk();
}

FDcResult _SerializeVector4f(FDcVector4f* Vec4fPtr, FDcSerializeContext& Ctx)
{
    DcSerializeUtils::FDcScopedTryUseJSONOverrideConfig ScopedOverrideConfig(Ctx.Writer);

    DC_TRY(Ctx.Writer->WriteArrayRoot());
    DC_TRY(Ctx.Writer->WriteFloat(Vec4fPtr->X));
    DC_TRY(Ctx.Writer->WriteFloat(Vec4fPtr->Y));
    DC_TRY(Ctx.Writer->WriteFloat(Vec4fPtr->Z));
    DC_TRY(Ctx.Writer->WriteFloat(Vec4fPtr->W));
    DC_TRY(Ctx.Writer->WriteArrayEnd());

	return DcOk();
}

#if ENGINE_MAJOR_VERSION == 5
FDcResult _SerializeVector2d(FDcVector2d* Vec2dPtr, FDcSerializeContext& Ctx)
{
    DcSerializeUtils::FDcScopedTryUseJSONOverrideConfig ScopedOverrideConfig(Ctx.Writer);

    DC_TRY(Ctx.Writer->WriteArrayRoot());
    DC_TRY(Ctx.Writer->WriteDouble(Vec2dPtr->X));
    DC_TRY(Ctx.Writer->WriteDouble(Vec2dPtr->Y));
    DC_TRY(Ctx.Writer->WriteArrayEnd());

    return DcOk();
}

FDcResult _SerializeVector3d(FDcVector3d* Vec3dPtr, FDcSerializeContext& Ctx)
{
    DcSerializeUtils::FDcScopedTryUseJSONOverrideConfig ScopedOverrideConfig(Ctx.Writer);

    DC_TRY(Ctx.Writer->WriteArrayRoot());
    DC_TRY(Ctx.Writer->WriteDouble(Vec3dPtr->X));
    DC_TRY(Ctx.Writer->WriteDouble(Vec3dPtr->Y));
    DC_TRY(Ctx.Writer->WriteDouble(Vec3dPtr->Z));
    DC_TRY(Ctx.Writer->WriteArrayEnd());

    return DcOk();
}

FDcResult _SerializeVector4d(FDcVector4d* Vec4dPtr, FDcSerializeContext& Ctx)
{
    DcSerializeUtils::FDcScopedTryUseJSONOverrideConfig ScopedOverrideConfig(Ctx.Writer);

    DC_TRY(Ctx.Writer->WriteArrayRoot());
    DC_TRY(Ctx.Writer->WriteDouble(Vec4dPtr->X));
    DC_TRY(Ctx.Writer->WriteDouble(Vec4dPtr->Y));
    DC_TRY(Ctx.Writer->WriteDouble(Vec4dPtr->Z));
    DC_TRY(Ctx.Writer->WriteDouble(Vec4dPtr->W));
    DC_TRY(Ctx.Writer->WriteArrayEnd());

    return DcOk();
}

static_assert(sizeof(FDcVector4f) == sizeof(FPlane4f), "FPlane4 stale");
static_assert(sizeof(FDcVector4d) == sizeof(FPlane4d), "FPlane4 stale");
#endif // ENGINE_MAJOR_VERSION == 5

FDcResult _SerializeVector(FVector* VecPtr, FDcSerializeContext& Ctx)
{
#if ENGINE_MAJOR_VERSION == 5
	return _SerializeVector3d(VecPtr, Ctx);
#else
	return _SerializeVector3f(VecPtr, Ctx);
#endif // ENGINE_MAJOR_VERSION == 5
}

FDcResult _SerializeVector4(FVector4* Vec4Ptr, FDcSerializeContext& Ctx)
{
#if ENGINE_MAJOR_VERSION == 5
	return _SerializeVector4d(Vec4Ptr, Ctx);
#else
	return _SerializeVector4f(Vec4Ptr, Ctx);
#endif // ENGINE_MAJOR_VERSION == 5
}


FDcResult HandlerVector2fSerialize(FDcSerializeContext& Ctx)
{
    FDcPropertyDatum Datum;
    DC_TRY(Ctx.Reader->ReadDataEntry(FStructProperty::StaticClass(), Datum));
    return _SerializeVector2f((FDcVector2f*)Datum.DataPtr, Ctx);
}

FDcResult HandlerVector3fSerialize(FDcSerializeContext& Ctx)
{
    FDcPropertyDatum Datum;
    DC_TRY(Ctx.Reader->ReadDataEntry(FStructProperty::StaticClass(), Datum));
    return _SerializeVector3f((FDcVector3f*)Datum.DataPtr, Ctx);
}


FDcResult HandlerVector4fSerialize(FDcSerializeContext& Ctx)
{
    FDcPropertyDatum Datum;
    DC_TRY(Ctx.Reader->ReadDataEntry(FStructProperty::StaticClass(), Datum));
    return _SerializeVector4f((FDcVector4f*)Datum.DataPtr, Ctx);
}

FDcResult HandlerMatrix44fSerialize(FDcSerializeContext& Ctx)
{
    FDcPropertyDatum Datum;
    DC_TRY(Ctx.Reader->ReadDataEntry(FStructProperty::StaticClass(), Datum));
    FDcMatrix44f* Matrix44fPtr = (FDcMatrix44f*)Datum.DataPtr;

    DC_TRY(Ctx.Writer->WriteArrayRoot());
    DC_TRY(_SerializeVector4f((FDcVector4f*)Matrix44fPtr->M[0] , Ctx));
    DC_TRY(_SerializeVector4f((FDcVector4f*)Matrix44fPtr->M[1] , Ctx));
    DC_TRY(_SerializeVector4f((FDcVector4f*)Matrix44fPtr->M[2] , Ctx));
    DC_TRY(_SerializeVector4f((FDcVector4f*)Matrix44fPtr->M[3] , Ctx));
    DC_TRY(Ctx.Writer->WriteArrayEnd());

    return DcOk();
}

FDcResult HandlerColorSerialize(FDcSerializeContext& Ctx)
{
	FDcPropertyDatum Datum;
	DC_TRY(Ctx.Reader->ReadDataEntry(FStructProperty::StaticClass(), Datum));

	FColor* ColorPtr = (FColor*)Datum.DataPtr;
	DC_TRY(Ctx.Writer->WriteString(TEXT("#") + ColorPtr->ToHex()));

	return DcOk();
}

FDcResult HandlerDateTimeSerialize(FDcSerializeContext& Ctx)
{
	FDcPropertyDatum Datum;
	DC_TRY(Ctx.Reader->ReadDataEntry(FStructProperty::StaticClass(), Datum));

	FDateTime* DateTimePtr = (FDateTime*)Datum.DataPtr;
	DC_TRY(Ctx.Writer->WriteString(*DateTimePtr->ToString()));

	return DcOk();
}

FDcResult HandlerTimespanSerialize(FDcSerializeContext& Ctx)
{
	FDcPropertyDatum Datum;
	DC_TRY(Ctx.Reader->ReadDataEntry(FStructProperty::StaticClass(), Datum));

	FTimespan* TimespanPtr = (FTimespan*)Datum.DataPtr;
	DC_TRY(Ctx.Writer->WriteString(*TimespanPtr->ToString()));

	return DcOk();
}

FDcResult HandlerTextSerialize(FDcSerializeContext& Ctx)
{
	FText Value;
	DC_TRY(Ctx.Reader->ReadText(&Value));

	if (Value.IsFromStringTable())
	{
		FName TableId;
		FString TextKey;

		verify(FTextInspector::GetTableIdAndKey(Value, TableId, TextKey));

		DC_TRY(Ctx.Writer->WriteArrayRoot());

		DC_TRY(Ctx.Writer->WriteString(TableId.ToString()));
		DC_TRY(Ctx.Writer->WriteString(TextKey));

		DC_TRY(Ctx.Writer->WriteArrayEnd());
	}
	else
	{
		DC_TRY(Ctx.Writer->WriteArrayRoot());

		TOptional<FString> Namespace = FTextInspector::GetNamespace(Value);
		DC_TRY(Ctx.Writer->WriteString(Namespace.IsSet() ? *Namespace : ""));

		TOptional<FString> TextKey = FTextInspector::GetKey(Value);
		DC_TRY(Ctx.Writer->WriteString(TextKey.IsSet() ? *TextKey : ""));

		const FString* SourceString = FTextInspector::GetSourceString(Value);
		DC_TRY(Ctx.Writer->WriteString(SourceString ? *SourceString : ""));

		DC_TRY(Ctx.Writer->WriteArrayEnd());
	}

	return DcOk();
}


FDcResult HandlerIntPointSerialize(FDcSerializeContext& Ctx)
{
    DcSerializeUtils::FDcScopedTryUseJSONOverrideConfig ScopedOverrideConfig(Ctx.Writer);

    FDcPropertyDatum Datum;
    DC_TRY(Ctx.Reader->ReadDataEntry(FStructProperty::StaticClass(), Datum));
    FIntPoint* IntPointPtr = (FIntPoint*)Datum.DataPtr;

    DC_TRY(Ctx.Writer->WriteArrayRoot());
    DC_TRY(Ctx.Writer->WriteInt32(IntPointPtr->X));
    DC_TRY(Ctx.Writer->WriteInt32(IntPointPtr->Y));
    DC_TRY(Ctx.Writer->WriteArrayEnd());

	return DcOk();
}

FDcResult HandlerIntVectorSerialize(FDcSerializeContext& Ctx)
{
    DcSerializeUtils::FDcScopedTryUseJSONOverrideConfig ScopedOverrideConfig(Ctx.Writer);

    FDcPropertyDatum Datum;
    DC_TRY(Ctx.Reader->ReadDataEntry(FStructProperty::StaticClass(), Datum));
    FIntVector* IntVectorPtr = (FIntVector*)Datum.DataPtr;

    DC_TRY(Ctx.Writer->WriteArrayRoot());
    DC_TRY(Ctx.Writer->WriteInt32(IntVectorPtr->X));
    DC_TRY(Ctx.Writer->WriteInt32(IntVectorPtr->Y));
    DC_TRY(Ctx.Writer->WriteInt32(IntVectorPtr->Z));
    DC_TRY(Ctx.Writer->WriteArrayEnd());

	return DcOk();
}

#if ENGINE_MAJOR_VERSION == 5
FDcResult HandlerVector2dSerialize(FDcSerializeContext& Ctx)
{
    FDcPropertyDatum Datum;
    DC_TRY(Ctx.Reader->ReadDataEntry(FStructProperty::StaticClass(), Datum));
    return _SerializeVector2d((FDcVector2d*)Datum.DataPtr, Ctx);
}

FDcResult HandlerVector3dSerialize(FDcSerializeContext& Ctx)
{
    FDcPropertyDatum Datum;
    DC_TRY(Ctx.Reader->ReadDataEntry(FStructProperty::StaticClass(), Datum));
    return _SerializeVector3d((FDcVector3d*)Datum.DataPtr, Ctx);
}

FDcResult HandlerVector4dSerialize(FDcSerializeContext& Ctx)
{
    FDcPropertyDatum Datum;
    DC_TRY(Ctx.Reader->ReadDataEntry(FStructProperty::StaticClass(), Datum));
    return _SerializeVector4d((FDcVector4d*)Datum.DataPtr, Ctx);
}

FDcResult HandlerMatrix44dSerialize(FDcSerializeContext& Ctx)
{
    FDcPropertyDatum Datum;
    DC_TRY(Ctx.Reader->ReadDataEntry(FStructProperty::StaticClass(), Datum));
    FDcMatrix44d* Matrix44dPtr = (FDcMatrix44d*)Datum.DataPtr;

    DC_TRY(Ctx.Writer->WriteArrayRoot());
    DC_TRY(_SerializeVector4d((FDcVector4d*)Matrix44dPtr->M[0] , Ctx));
    DC_TRY(_SerializeVector4d((FDcVector4d*)Matrix44dPtr->M[1] , Ctx));
    DC_TRY(_SerializeVector4d((FDcVector4d*)Matrix44dPtr->M[2] , Ctx));
    DC_TRY(_SerializeVector4d((FDcVector4d*)Matrix44dPtr->M[3] , Ctx));
    DC_TRY(Ctx.Writer->WriteArrayEnd());

    return DcOk();
}

FDcResult HandlerIntVector4Serialize(FDcSerializeContext& Ctx)
{
    DcSerializeUtils::FDcScopedTryUseJSONOverrideConfig ScopedOverrideConfig(Ctx.Writer);

    FDcPropertyDatum Datum;
    DC_TRY(Ctx.Reader->ReadDataEntry(FStructProperty::StaticClass(), Datum));
    FIntVector4* IntVector4Ptr = (FIntVector4*)Datum.DataPtr;

    DC_TRY(Ctx.Writer->WriteArrayRoot());
    DC_TRY(Ctx.Writer->WriteInt32(IntVector4Ptr->X));
    DC_TRY(Ctx.Writer->WriteInt32(IntVector4Ptr->Y));
    DC_TRY(Ctx.Writer->WriteInt32(IntVector4Ptr->Z));
    DC_TRY(Ctx.Writer->WriteInt32(IntVector4Ptr->W));
    DC_TRY(Ctx.Writer->WriteArrayEnd());

	return DcOk();
}
#endif // ENGINE_MAJOR_VERSION == 5


} // namespace DcCommonHandlers





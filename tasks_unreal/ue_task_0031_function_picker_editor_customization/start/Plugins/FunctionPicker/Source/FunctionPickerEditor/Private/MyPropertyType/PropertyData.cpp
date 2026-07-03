// Copyright (c) Yevhenii Selivanov.

#include "MyPropertyType/PropertyData.h"
//---
#include "PropertyHandle.h"

// Empty property data
const FPropertyData FPropertyData::Empty = FPropertyData();

// Custom constructor, is not required, but fully init property data.
FPropertyData::FPropertyData(TSharedRef<IPropertyHandle> InPropertyHandle)
	: PropertyHandle(MoveTemp(InPropertyHandle))
{
	PropertyName = GetPropertyNameFromHandle();
	PropertyValue = GetPropertyValueFromHandle();
}

// Get property from handle
FProperty* FPropertyData::GetProperty() const
{
	return PropertyHandle ? PropertyHandle->GetProperty() : nullptr;
}

// Get property name by handle
FName FPropertyData::GetPropertyNameFromHandle() const
{
	const FProperty* CurrentProperty = GetProperty();
	return CurrentProperty ? CurrentProperty->GetFName() : NAME_None;
}

// Get FName value by property handle
FName FPropertyData::GetPropertyValueFromHandle() const
{
	return NAME_None;
}

// Get property ptr to the value by handle
void* FPropertyData::GetPropertyValuePtrFromHandle() const
{
	void* FoundData = nullptr;
	if (PropertyHandle.IsValid())
	{
		PropertyHandle->GetValueData(/*Out*/FoundData);
	}
	return FoundData;
}

// Set FName value by property handle
void FPropertyData::SetPropertyValueToHandle(FName NewValue)
{
}

// Returns the meta value by specified ke
FName FPropertyData::GetMetaDataValue(FName Key) const
{
	return NAME_None;
}

// Returns true if specified key exist
bool FPropertyData::IsMetaKeyExists(FName Key) const
{
	return false;
}

// Set the meta value by specified key
void FPropertyData::SetMetaDataValue(FName Key, FName NewValue, bool bNotifyPostChange/* = false*/)
{
}

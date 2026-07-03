// Copyright (c) Yevhenii Selivanov.

#include "MyPropertyType/MyPropertyTypeCustomization.h"
//---
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "SSearchableComboBox.h"

// Called when the header of the property (the row in the details panel where the property is shown)
void FMyPropertyTypeCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// Use default the header details panel
	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			PropertyHandle->CreatePropertyValueWidget()
		];
}

// Called when the children of the property should be customized or extra rows added.
void FMyPropertyTypeCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// Find outer
	TArray<UObject*> OuterObjects;
	PropertyHandle/*ref*/->GetOuterObjects(OuterObjects);
	MyPropertyOuterInternal = OuterObjects.IsValidIndex(0) ? OuterObjects[0] : nullptr;

	// Set parent property
	ParentPropertyInternal = FPropertyData(PropertyHandle);
	const TDelegate<void()>& RefreshCustomPropertyFunction = FSimpleDelegate::CreateSP(this, &FMyPropertyTypeCustomization::RefreshCustomProperty);
	PropertyHandle/*ref*/->SetOnPropertyValueChanged(RefreshCustomPropertyFunction);
	PropertyHandle/*ref*/->SetOnChildPropertyValueChanged(RefreshCustomPropertyFunction);

	// Set children properties
	uint32 NumChildren;
	PropertyHandle/*ref*/->GetNumChildren(NumChildren);
	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		FPropertyData PropertyData(PropertyHandle/*ref*/->GetChildHandle(ChildIndex).ToSharedRef());
		OnCustomizeChildren(ChildBuilder, PropertyData);
	}
}

// Set the FName value into the property
void FMyPropertyTypeCustomization::SetCustomPropertyValue(FName Value)
{
}

// Set true to activate property, false to grey out it (read-only)
void FMyPropertyTypeCustomization::SetCustomPropertyEnabled(bool bEnabled)
{
	const bool bIsAllowedEnableCustomProperty = IsAllowedEnableCustomProperty();
	if (bEnabled && !bIsAllowedEnableCustomProperty)
	{
		// Enable is forbidden
		return;
	}

	if (const TSharedPtr<SSearchableComboBox>& SearchableComboBox = SearchableComboBoxInternal.Pin())
	{
		SearchableComboBox->SetEnabled(bEnabled);
	}

	CustomPropertyInternal.bIsEnabled = bEnabled;
}

// Is called for each property on building its row
void FMyPropertyTypeCustomization::OnCustomizeChildren(IDetailChildrenBuilder& ChildBuilder, FPropertyData& PropertyData)
{
	if (!ensureMsgf(PropertyData.IsValid(), TEXT("ASSERT: 'PropertyData.PropertyHandle' is not valid")))
	{
		return;
	}

	if (PropertyData.PropertyName != CustomPropertyInternal.PropertyName)
	{
		// Add each another property to the Details Panel without customization
		ChildBuilder.AddProperty(PropertyData.PropertyHandle.ToSharedRef())
		                                           .ShouldAutoExpand(true)
		                                           .IsEnabled(PropertyData.bIsEnabled)
		                                           .Visibility(PropertyData.Visibility);
		DefaultPropertiesInternal.Emplace(PropertyData);
		return;
	}

	// --- Is custom property ---

	CustomPropertyInternal = PropertyData;

	// Add as searchable combo box by default
	AddCustomPropertyRow(PropertyData.PropertyHandle->GetPropertyDisplayName(), ChildBuilder);
}

// Will add the default searchable combo box
void FMyPropertyTypeCustomization::AddCustomPropertyRow(const FText& PropertyDisplayText, IDetailChildrenBuilder& ChildBuilder)
{
}

//Set new values for the list of selectable members
void FMyPropertyTypeCustomization::RefreshCustomProperty()
{
	if (const TSharedPtr<SSearchableComboBox>& SearchableComboBox = SearchableComboBoxInternal.Pin())
	{
		SearchableComboBox->RefreshOptions();
	}
}

// Is called to deactivate custom property
void FMyPropertyTypeCustomization::InvalidateCustomProperty()
{
	SetCustomPropertyEnabled(false);

	SetCustomPropertyValue(NAME_None);
}

// Called when the children of the property should be customized or extra rows added
void FMyPropertyTypeCustomization::OnCustomPropertyChosen(TSharedPtr<FString> SelectedStringPtr, ESelectInfo::Type SelectInfo)
{
}

// Add an empty row once, so the users can clear the selection if they want
void FMyPropertyTypeCustomization::InitSearchableComboBox()
{
	if (!NoneStringInternal.IsValid())
	{
		TSharedPtr<FString> NoneStringPtr(MakeShareable(new FString(FPropertyData::NoneString)));
		NoneStringInternal = NoneStringPtr;
		SearchableComboBoxValuesInternal.EmplaceAt(0, MoveTemp(NoneStringPtr));
	}
}

// Reset and remove all shared strings in array except 'None' string
void FMyPropertyTypeCustomization::ResetSearchableComboBox()
{
}

// Copyright (c) Yevhenii Selivanov

#include "BmrPowerupTagCustomization.h"

// UE
#include "PropertyEditorModule.h"

/** The name of class to be customized: BmrPowerupTag */
// @TODO JanSeliv 2dUuTjyT use 'FBmrPowerupTag::StaticStruct()->GetFName()' as soon as the editor module starts referencing the runtime module
const FName FBmrPowerupTagCustomization::PropertyClassName = TEXT("BmrPowerupTag");

// Makes a new instance of this detail layout class for a specific detail view requesting it
TSharedRef<IPropertyTypeCustomization> FBmrPowerupTagCustomization::MakeInstance()
{
	return FGameplayTagCustomizationPublic::MakeInstance();
}

// Creates customization for the Powerup Tag
void FBmrPowerupTagCustomization::RegisterPowerupTagCustomization()
{
	if (!FModuleManager::Get().IsModuleLoaded(PropertyEditorModule))
	{
		return;
	}

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(PropertyEditorModule);

	// Use default GameplayTag customization for inherited BmrPowerupTag to show Tags list
	PropertyModule.RegisterCustomPropertyTypeLayout(
	    PropertyClassName,
	    FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FBmrPowerupTagCustomization::MakeInstance));

	PropertyModule.NotifyCustomizationModuleChanged();
}

// Removes customization for the Powerup Tag
void FBmrPowerupTagCustomization::UnregisterPowerupTagCustomization()
{
	if (!FModuleManager::Get().IsModuleLoaded(PropertyEditorModule))
	{
		return;
	}

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(PropertyEditorModule);

	PropertyModule.UnregisterCustomPropertyTypeLayout(PropertyClassName);
}
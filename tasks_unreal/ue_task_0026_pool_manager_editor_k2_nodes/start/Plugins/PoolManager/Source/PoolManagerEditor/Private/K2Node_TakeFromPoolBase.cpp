// Copyright (c) Yevhenii Selivanov

#include "K2Node_TakeFromPoolBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(K2Node_TakeFromPoolBase)

bool UK2Node_TakeFromPoolBase::PostExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph& SourceGraph, UK2Node_CallFunction& CallTakeFromPoolNode)
{
	return false;
}

UClass* UK2Node_TakeFromPoolBase::GetSelectedClass(const TArray<UEdGraphPin*>* InPinsToSearch) const
{
	return nullptr;
}

void UK2Node_TakeFromPoolBase::OnClassPinChanged() const
{
}

UEdGraphPin* UK2Node_TakeFromPoolBase::SpawnIsResultValidPin(FKismetCompilerContext& CompilerContext, UEdGraph& SourceGraph, UEdGraphPin* ResultVariablePin)
{
	return nullptr;
}

void UK2Node_TakeFromPoolBase::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
}

void UK2Node_TakeFromPoolBase::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);
}

void UK2Node_TakeFromPoolBase::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);
}

FString UK2Node_TakeFromPoolBase::GetPinMetaData(FName InPinName, FName InKey)
{
	return Super::GetPinMetaData(InPinName, InKey);
}

FText UK2Node_TakeFromPoolBase::GetTooltipText() const
{
	return Super::GetTooltipText();
}

FText UK2Node_TakeFromPoolBase::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return Super::GetNodeTitle(TitleType);
}

bool UK2Node_TakeFromPoolBase::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	return false;
}

void UK2Node_TakeFromPoolBase::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	BreakAllNodeLinks();
}

FName UK2Node_TakeFromPoolBase::GetCornerIcon() const
{
	return NAME_None;
}

void UK2Node_TakeFromPoolBase::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
}

FText UK2Node_TakeFromPoolBase::GetMenuCategory() const
{
	return FText::GetEmpty();
}

void UK2Node_TakeFromPoolBase::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	AllocateDefaultPins();
}

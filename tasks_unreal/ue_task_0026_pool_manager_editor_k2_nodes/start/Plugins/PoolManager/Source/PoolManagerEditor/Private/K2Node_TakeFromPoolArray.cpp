// Copyright (c) Yevhenii Selivanov

#include "K2Node_TakeFromPoolArray.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(K2Node_TakeFromPoolArray)

UEdGraphNode::FCreatePinParams UK2Node_TakeFromPoolArray::GetReturnValuePinParams() const
{
	return FCreatePinParams();
}

FName UK2Node_TakeFromPoolArray::GetNativeFunctionName() const
{
	return NAME_None;
}

bool UK2Node_TakeFromPoolArray::PostExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph& SourceGraph, UK2Node_CallFunction& CallTakeFromPoolNode)
{
	return false;
}

UEdGraphPin* UK2Node_TakeFromPoolArray::SpawnIsResultValidPin(FKismetCompilerContext& CompilerContext, UEdGraph& SourceGraph, UEdGraphPin* ResultVariablePin)
{
	return nullptr;
}

void UK2Node_TakeFromPoolArray::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
}

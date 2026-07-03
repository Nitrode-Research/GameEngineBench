// Copyright (c) Yevhenii Selivanov

#include "K2Node_TakeFromPool.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(K2Node_TakeFromPool)

FName UK2Node_TakeFromPool::GetNativeFunctionName() const
{
	return NAME_None;
}

bool UK2Node_TakeFromPool::PostExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph& SourceGraph, UK2Node_CallFunction& CallTakeFromPoolNode)
{
	return false;
}

void UK2Node_TakeFromPool::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
}

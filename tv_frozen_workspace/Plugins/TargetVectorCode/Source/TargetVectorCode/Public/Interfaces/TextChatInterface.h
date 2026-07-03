#pragma once

#include "NativeGameplayTags.h"
#include "TextChatInterface.generated.h"

UINTERFACE(Blueprintable)
class UTextChatInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API ITextChatInterface {
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Game State|Text Chat")
	bool IsChat();

};
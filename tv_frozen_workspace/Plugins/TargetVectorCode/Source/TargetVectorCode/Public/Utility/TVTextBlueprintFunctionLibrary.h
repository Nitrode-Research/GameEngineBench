// MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TVTextBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class TARGETVECTORCODE_API UTVTextBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Check String for Profanity", Keywords = "text, moderation"), Category = "Text")
	static bool StringContainsProfanity(const FString& InString, int Severity);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Check Text for Profanity", Keywords = "text, moderation"), Category = "Text")
	static bool TextContainsProfanity(const FText& InText, int Severity);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Filter String for Profanity", Keywords = "text, moderation"), Category = "Text")
	static bool FilterStringForProfanity(const FString& InText, int Severity, const FString FilteredString);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Filter Text for Profanity", Keywords = "text, moderation"), Category = "Text")
	static bool FilterTextForProfanity(const FText& InText, int Severity, const FText FilteredText);
};

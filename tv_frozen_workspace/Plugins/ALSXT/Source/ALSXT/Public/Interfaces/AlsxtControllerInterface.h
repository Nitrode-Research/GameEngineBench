#pragma once
#include "UObject/Interface.h"

#include "AlsxtControllerInterface.generated.h"

UINTERFACE(Blueprintable)
class UAlsxtControllerInterface : public UInterface {
	GENERATED_BODY()
};

class ALSXT_API IAlsxtControllerInterface {
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Character Camera Effects Component Interface|Character")
	APawn* GetControlledPawn();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Character Camera Effects Component Interface|Character")
	APawn* GetControlledCharacter();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Character Camera Effects Component Interface|Character")
	bool CanControlPawn();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Character Camera Effects Component Interface|Character")
	bool CanControlCharacter();
};
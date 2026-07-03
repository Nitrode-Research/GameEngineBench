// MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/IHttpRequest.h"
#include "Data/ActorData.h"
#include "AuroraGameStateComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AURORA_API UAuroraGameStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAuroraGameStateComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Serialization")
	FString SerializeLevelToJson();

	UFUNCTION(BlueprintCallable, Category = "Serialization")
	bool DeserializeLevelFromJson(const FString& JsonString);
	
protected:
    void OnCommitResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void ForceSync();

private:
	void SendPostRequest(const FString& ApiEndpoint, const FString& JsonPayload);
	void CollectActorData(FMyLevelSerializationData& OutGameState);
	void GatherActorData(AActor* Actor, FMyActorSerializationData& OutActorData);
	
};

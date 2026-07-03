// MIT


#include "Components/AuroraGameStateComponent.h"
#include "Http.h"
#include "AbilitySystemComponent.h"
#include "DataConfig/DcEnv.h"
#include "EngineUtils.h"
#include "DataConfig/Json/DcJsonReader.h"
#include "DataConfig/Json/DcJsonWriter.h"
#include "DataConfig/Property/DcPropertyReader.h"
#include "DataConfig/Property/DcPropertyWriter.h"


// Sets default values for this component's properties
UAuroraGameStateComponent::UAuroraGameStateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UAuroraGameStateComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UAuroraGameStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

FString UAuroraGameStateComponent::SerializeLevelToJson()
{
	FMyLevelSerializationData LevelData;

	return FString();
}

bool UAuroraGameStateComponent::DeserializeLevelFromJson(const FString& JsonString)
{
	

	return false;
}

void UAuroraGameStateComponent::SendPostRequest(const FString& ApiEndpoint, const FString& JsonPayload)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindUObject(this, &UAuroraGameStateComponent::OnCommitResponseReceived);
    Request->SetURL(ApiEndpoint);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("User-Agent"), TEXT("YourGame-Client"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(JsonPayload);
    Request->ProcessRequest();
}

void UAuroraGameStateComponent::CollectActorData(FMyLevelSerializationData& OutGameState)
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || Actor->IsPendingKillPending() || Actor->IsA<APlayerController>()) continue; // Skip irrelevant actors

		FMyActorSerializationData ActorData;
		ActorData.ActorName = Actor->GetName();
		ActorData.ActorTransform = Actor->GetTransform();

		// Handle PCGActorData if it exists
		// This requires reflection or a known interface to access the UPROPERTY
		// For simplicity, we'll assume a TMap<FString, FString> for properties
		// You'll need to adapt this based on your actual PCGActorData structure
		// Example: if (Actor->HasPCGActorData()) { ActorData.PCGActorDataProperties = Actor->GetPCGActorDataProperties(); }

		// Collect other custom properties
		// Example: MyCustomComponent* CustomComp = Actor->FindComponentByClass<MyCustomComponent>();
		// if (CustomComp) { ActorData.CustomProperties.Add("MyValue", FString::FromInt(CustomComp->MyIntVariable)); }

		OutGameState.ActorsData.Add(ActorData);
	}
}

void UAuroraGameStateComponent::GatherActorData(AActor* Actor, FMyActorSerializationData& OutActorData)
{
	OutActorData.ActorName = Actor->GetName();
    OutActorData.ActorTransform = Actor->GetTransform();

    // Example: Gather PCGActorData (replace with your actual PCGActorData component/logic)
    // If your PCGActorData is a UPROPERTY of the actor, you can iterate its properties
    // or call a specific function on the component to get serializable data.
    // For simplicity, this example assumes you manually extract specific properties.
    // UYourPCGActorDataComponent* PCGComponent = Actor->FindComponentByClass<UYourPCGActorDataComponent>();
    // if (PCGComponent)
    // {
    //     OutActorData.PCGActorDataProperties.Add("SomePCGProperty", PCGComponent->GetSomeValue());
    // }

    // Gather GAS data
    UAbilitySystemComponent* ASC = Actor->FindComponentByClass<UAbilitySystemComponent>();
    if (ASC)
    {
    	
    		
        
    }

    // Gather other variables
    OutActorData.SomeIntegerVariable = 123; // Replace with actual variable
    OutActorData.SomeStringVariable = "TestString"; // Replace with actual variable
}

void UAuroraGameStateComponent::OnCommitResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()))
    {
        UE_LOG(LogTemp, Log, TEXT("Commit successful! Response: %s"), *Response->GetContentAsString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Commit failed! Status code: %d. Response: %s"), Response.IsValid() ? Response->GetResponseCode() : -1, Response.IsValid() ? *Response->GetContentAsString() : TEXT("N/A"));
    }
}


// MIT


#include "FunctionLibrary/AuroraFunctionLibrary.h"
#include "DataConfig/DcEnv.h"
#include "DataConfig/Property/DcPropertyWriter.h"
#include "DataConfig/Property/DcPropertyReader.h"
#include "DataConfig/Serialize/DcSerializeTypes.h"
#include "DataConfig/DcEnv.h"
#include "DataConfig/Json/DcJsonWriter.h"
#include "DataConfig/Json/DcJsonReader.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemComponent.h"
#include "Data/ActorData.h"

bool UAuroraFunctionLibrary::SerializePCGActor(AActor* PCGActor, FString& OutJsonString)
{
	if (!PCGActor)
	{
		// UE_LOG(LogDataConfig, Warning, TEXT("SerializePCGActor failed: PCGActor is null."));
		return false;
	}

	FString LocalJsonString;

	FDcJsonWriter JsonWriter;
	FDcPropertyReader PropertyReader;

	// Use a context to drive the serialization process
	FDcSerializeContext Context;
	Context.Writer = &JsonWriter;
	Context.Reader = &PropertyReader;

	// The result of the serialization operation
	// DcOk();
    
	// Prepare the JSON writer to output to a string
	// DcOk(JsonWriter.SetWriter(OutJsonString));
// 
	// // Serialize the PCG Actor. You can also serialize its UPROPERTY-marked members
	// // by using a TArray<FDcPropertyDatum> for a structured approach.
	// DcOk(PropertyReader.SetObject(PCGActor));
// 
	// // Execute the serialization
	// if (!DcOk(DcSerializeFrom(Context)))
	// {
	// 	UE_LOG(LogDataConfig, Error, TEXT("DataConfig serialization failed for PCG Actor: %s"), *PCGActor->GetName());
	// 	return false;
	// }

	OutJsonString = LocalJsonString;
	return true;
}

bool UAuroraFunctionLibrary::SerializeASCActor(AActor* Actor, FString& OutJsonString)
{
		if (!Actor || Actor->IsPendingKillPending())
		{
			return false;
		}

		UAbilitySystemComponent* ASC = Actor->FindComponentByClass<UAbilitySystemComponent>();
		if (ASC)
		{
			FSerializedASCData ASCData;
			FString LocalJsonString;

			// Serialize Active Gameplay Effects
			// for (const FActiveGameplayEffect& ActiveEffect : ASC->GetActiveGameplayEffects())
			// {
			// 	FSerializedGameplayEffectData EffectData;
			// 	EffectData.EffectSpecHandle = ActiveEffect.Spec;
			// 	EffectData.ActiveEffectHandle = ActiveEffect.Handle;
			// 	ASCData.ActiveEffects.Add(EffectData);
			// }

			// Serialize Attributes
			// for (const FGameplayAttribute& Attribute : ASC->GetSet<UAttributeSet>()->GetAttributes()) // Requires a known AttributeSet
			// {
			// 	ASCData.Attributes.Add(Attribute, ASC->GetNumericAttributeBase(Attribute));
			// }
// 
			// OutGameState.ASCDataMap.Add(Actor->GetName(), ASCData);

			OutJsonString = LocalJsonString;
			return true;
		}
	return false;
}



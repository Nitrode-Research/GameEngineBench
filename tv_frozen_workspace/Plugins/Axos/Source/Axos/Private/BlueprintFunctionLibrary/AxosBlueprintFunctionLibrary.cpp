// MIT

#include "BlueprintFunctionLibrary/AxosBlueprintFunctionLibrary.h"
#include "Components/AxosGameStateComponent.h"
#include "Interfaces/AxosInterface.h"

float UAxosBlueprintFunctionLibrary::GetActorBearing(AActor* TargetActor, AGameState* GameState)
{
	if (!TargetActor || !GameState)
	{
		// Handle Errors gracefully by returning Actors Forward Vector
		return TargetActor->GetActorForwardVector().Y;
	}

	if (!GameState->GetClass()->ImplementsInterface(UAxosInterface::StaticClass()))
	{
		return TargetActor->GetActorForwardVector().Y;
	}
	else
	{
		TSoftObjectPtr<UAxosGameStateComponent> AxosGameStateComponent = IAxosInterface::Execute_GetAxosGameStateComponent(GameState);

		if (!AxosGameStateComponent)
		{
			return TargetActor->GetActorForwardVector().Y;
		}
		else
		{
			// Get North Yaw from GameState (assuming a float variable called NorthYawDegrees)
			float NorthYaw = AxosGameStateComponent->NorthYaw; 

			// Get Actor's Forward Vector
			FVector ActorForwardVector = TargetActor->GetActorForwardVector();

			// Create North Direction Vector
			FRotator NorthRotation(0.0f, NorthYaw, 0.0f);
			FVector NorthDirection = NorthRotation.Vector();

			// Calculate Bearing Angle
			float DotProduct = FVector::DotProduct(NorthDirection, ActorForwardVector);
			float AngleRadians = FMath::Acos(DotProduct);
			float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

			return AngleDegrees;	
		}
	}
}
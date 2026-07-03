// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Engine/LocalPlayer.h"

#include "BmrLocalPlayer.generated.h"

/**
 * Is main actor which represents the local player instance in the project.
 * Is inherited from ULocalPlayer to override and extend player-specific behavior on the client.
 */
UCLASS()
class BOMBER_API UBmrLocalPlayer : public ULocalPlayer
{
	GENERATED_BODY()

	/** Default constructor with overridden properties. */
	UBmrLocalPlayer();
};

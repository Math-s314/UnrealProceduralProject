// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DecoBase.generated.h"

/**
 * Base class for all room's decoration class : they generate the floor, walls and ceil texture for room of the given type.
 * These classes can only have one instance for each HomeGenerator instance (acts like a singleton, but everything is handled by the generator) : it allows more complex generation where one instance generate a more uniform decoration (using room's type).
 */
UCLASS()
class HOMEGENERATION_API UDecoBase : public UObject
{
	GENERATED_BODY()
};

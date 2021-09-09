// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FurnitureMeshInt.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UFurnitureMeshInt : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class HOMEGENERATION_API IFurnitureMeshInt
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	UStaticMeshComponent *GetStaticMeshComponent(); virtual UStaticMeshComponent *GetStaticMeshComponent_Implementation() = 0;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FurnitureMeshAsset.h"
#include "FurnitureMeshInt.h"
#include "Engine/DataAsset.h"
#include "WindowMeshAsset.generated.h"

/**
 * Groups, for the windows, all the constraints which will be used to correctly spawn the mesh.
 */
USTRUCT(BlueprintType)
struct FWindowConstraint
{
	GENERATED_BODY()
	
	//The needed distance between the floor and the bottom of the window
	//If this information added to the height of a mesh is greater than the floor height, the mesh won't never spawn.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DistanceFromFloor;

	//Exterior side of the window
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGenerationAxe ExteriorFace;
};

/**
 * Asset class containing all needed information to describe a window's mesh for the system :
 * - the information to correctly place it;
 * - the information to correctly spawn it.
 */
UCLASS(BlueprintType)
class HOMEGENERATION_API UWindowMeshAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	//Actor class which will be spawned instead of the default AStaticMeshActor
	//If this class implements the FurnitureMeshInt interface, it main function will be used to set the indicated mesh to this actor.
	//If this class is Null, the system wil spawn an AStaticMeshActor with the given UStaticMesh.
	UPROPERTY(EditAnyWhere, BlueprintReadWrite)
	TSubclassOf<AActor> ActorClass;

	//Static mesh used to override the default Static mesh for the given class, if it implements the FurnitureMeshInt interface or if no class is provided.
	//Useless if a class which doesn't implement this interface, is set.
	UPROPERTY(EditAnyWhere, BlueprintReadWrite)
	UStaticMesh *Mesh;

	//Indicates if the following constraint structure should override the default constraints
	UPROPERTY(EditAnyWhere, BlueprintReadWrite)
	bool bOverrideConstraint;

	//Constraints which will override the default structure if bOverrideConstraint is set to true.
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, meta=(EditCondition="bOverrideConstraint"))
	FWindowConstraint ConstraintsOverride;

	//TODO : Actually never calculated but say it works
	FVector BoundsOrigin;
	FVector BoxExtent;
	FVectorGrid GridSize; //0 along the exterior axis
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FurnitureMeshInt.h"
#include "HGBasicType.h"
#include "Engine/DataAsset.h"
#include "FurnitureMeshAsset.generated.h"

/**
 * Represents a side of a furniture or an element in the generation system (a room's side for example).
 * This information is based on the local axes of a mesh (for a furniture).
 * Can be used in a mask storage.
 */
UENUM(BlueprintType)
enum class EGenerationAxe : uint8
{
	//Shouldn't be used, only exists for compilation purposes
	NONE = 0	UMETA(DisplayName="None"),
	
	X_UP   = 1	UMETA(DisplayName="Axe X+"),//0001
	X_DOWN = 2	UMETA(DisplayName="Axe X-"),//0010
	Y_UP   = 4	UMETA(DisplayName="Axe Y+"),//0100
	Y_DOWN = 8	UMETA(DisplayName="Axe Y-") //1000
};

//Basic furniture's constraints

/**
 * Defines the global interaction of a furniture with the room's walls.
 */
UENUM(BlueprintType)
enum class EWallGlobalInteraction : uint8
{
	//Only take into account the axes marked as REJECT (they must be far from a wall)
	DEFAULT		UMETA(DisplayName="Default"),

	//The first two detected axes marked as ATTRACT must be along a wall (take into account REJECT sides)
	//If at least two walls are marked as ATTRACT, the placement will always fail
	CORNER		UMETA(DisplayName="Corner"),

	//The first detected axe marked as ATTRACT must be along a wall (take into account REJECT sides)
	//If at least one wall is marked as ATTRACT, the placement will always fail
	WALL		UMETA(DisplayName="Wall"),

	//All sides must be along a wall regardless of their interaction constraint
	FAR			UMETA(DisplayName="Far")
};

/**
 * Defines the interaction of furniture's side with the room's walls.
 * Their behavior depends on the selected global interaction :
 * REJECT -> always take into account
 * ATTRACT -> evaluated as IGNORE if the global interaction isn't WALL or CORNER
 */
UENUM(BlueprintType)
enum class EWallAxeInteraction : uint8
{
	//The side may be along a wall or not
	IGNORE		UMETA(DisplayName="Ignore"),

	//In global mode WALL or CORNER, the side must be along a wall
	ATTRACT		UMETA(DisplayName="Attract"),

	//The side must always be at least at one grid square far from a wall
	REJECT		UMETA(DisplayName="Reject")
};

/**
 * Defines the interaction with the room's walls for each side of the furniture and its global interaction.
 * The final behavior of each of these interaction's axis depends on the selected global interaction.
 */
USTRUCT(BlueprintType)
struct FWallInteractionStruct
{
	GENERATED_BODY()

	//Defines the global interaction of a furniture with the room's walls.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWallGlobalInteraction WallGlobalInteraction;

	//Defines the interaction for the furniture's X+ side
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="Axe X+"))
	EWallAxeInteraction XUp;

	//Defines the interaction for the furniture's X- side
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="Axe X-"))
	EWallAxeInteraction XDown;

	//Defines the interaction for the furniture's Y+ side
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="Axe Y+"))
	EWallAxeInteraction YUp;

	//Defines the interaction for the furniture's Y- side
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="Axe Y-"))
	EWallAxeInteraction YDown;

	//Return the first attracted wall that the function can find. Return false if it can't.
	//Guarantees that at least one attracted wall exists.
	bool GetFirstAttractedWall(EGenerationAxe &First) const;
	
	//Return the second attracted wall that the function can find (different from the first one returned by the other function). Return false if it can't.
	//Guarantees that at least two attracted walls exist.
	bool GetSecondAttractedWall(EGenerationAxe &Second) const;
};

/**
 * Defines the margin for each side of the given furniture.
 * A margin is a space (which must be completely in the room) in which no other furniture can be placed (except for dependencies) but which can collapse with other margin (like HTML elements)
 * They are given in number of grid square.
 */
USTRUCT(BlueprintType)
struct FMarginStruct
{
	GENERATED_BODY()
	
	//Defines the margin's size for the furniture's X+ side
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="Axe X+"))
	int XUp;

	//Defines the margin's size for the furniture's X- side
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="Axe X-"))
	int XDown;

	//Defines the margin's size for the furniture's Y+ side
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="Axe Y+"))
	int YUp;

	//Defines the margin's size for the furniture's Y- side
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="Axe Y-"))
	int YDown;
};

/**
 * Groups, for a furniture, all the constraints which will be used to check the correctness of a given position of this furniture in the room.
 */
USTRUCT(BlueprintType)
struct FFurnitureConstraint
{
	GENERATED_BODY()

	//Defines the interaction with the room's walls for each side of the furniture.
	//The final behavior of each of these interactions depends on the selected global interaction.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FWallInteractionStruct WallAxeInteraction;

	//Defines the margin for each side of the given furniture.
	//They are given in number of grid square.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMarginStruct Margin;
};

//Dependency information

/**
 * Describes the way a dependency must be placed in front of a side of the parent furniture.
 * TODO : It isn't actually very permissive
 */
UENUM(BlueprintType)
enum class EDependencyPlace : uint8
{
	//The dependency must be placed along the left edge of the axis (when this axis is at the top of the furniture after a rotation)
	EDGE_L		UMETA(DisplayName="Edge Left"),

	//The dependency must be placed along the right edge of the axis (when this axis is at the top of the furniture after a rotation)
	EDGE_R		UMETA(DisplayName="Edge Right"),

	//The dependency must be placed in front of the given axis but not along the right neither the left edge of this axis.
	DEFAULT		UMETA(DisplayName="Default")
};

/**
 * Groups all the information needed to correctly place the dependency of a furniture.
 */
USTRUCT(BlueprintType)
struct FFurnitureDependency
{
	GENERATED_BODY()
	
	//Maximal distance between a dependency and its parent
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Distance;

	//Axe in front of which the dependency will be placed
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGenerationAxe Axe;

	//The way the way a dependency will be placed in front of the chosen axis.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EDependencyPlace Position;

	//The furniture type of this dependency
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName FurnitureType;
};

/**
 * Asset class containing all needed information to describe a mesh for the system :
 * - the information to correctly place it;
 * - the information to correctly spawn it.
 */
UCLASS(BlueprintType)
class HOMEGENERATION_API UFurnitureMeshAsset : public UDataAsset
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

	//Indicates if the following constraint structure should override the default constraints for the bound furniture.
	UPROPERTY(EditAnyWhere, BlueprintReadWrite)
	bool bOverrideConstraint;

	//Constraints which will override the default structure if bOverrideConstraint is set to true.
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, meta=(EditCondition="bOverrideConstraint"))
	FFurnitureConstraint ConstraintsOverride;

	//TODO : Actually never calculated but say it works
	FVector BoundsOrigin;
	FVector BoxExtent;
	FVectorGrid GridSize;
};

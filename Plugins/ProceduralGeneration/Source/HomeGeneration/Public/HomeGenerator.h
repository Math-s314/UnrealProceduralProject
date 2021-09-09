// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FurnitureMeshAsset.h"
#include "DecoBase.h"
#include "HGInternalStruct.h"
#include "HomeGenerator.generated.h"

/**
 * Groups all the information needed to define the global building form
 */
USTRUCT(BlueprintType)
struct FBuildingConstraint
{
	GENERATED_BODY()

	//Size of the side of a square in the construction grid
	//In UE unit
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GridSnapLength;

	//Maximal number of floors in the generated building
	//In grid square
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int MaxFloorsNumber;

	//Minimal number of floors in the generated building
	//In grid square
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int MinFloorsNumber;

	//Minimal size of the side of a floor
	//In grid square
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int MinSideFloorLength;

	//Ground thickness of one floor
	//In UE unit
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FloorWidth;

	//Height between the floor and the ceiling of a room. The total height bewtween two floors is thus : FloorWidth + FloorHight
	//In UE unit
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FloorHeight;

	//Walls thickness
	//In UE unit
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WallWidth;
};

/**
 * Groups all the information needed to define a room.
 */
USTRUCT(BlueprintType)
struct FRoom
{
	GENERATED_BODY()

	FRoom() : MinRoomSide(0) {}
	
	//Ordered list of all furniture which may be placed in this room's type : the furniture with the lowest index will be the first to be placed (more chances to success)
	//Must be seen as a priority list of the furniture (the more important furniture have to be placed in first)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> Furniture;

	//Decoration class which will return the correct texture for the given room
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UDecoBase> DecorationClass;

	//TODO : Comment and meta
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="NumPerHab ?? A Display name must be added !!", ClampMin="0.0", ClampMax="5.0"))
	float NumPerHab;

protected:
	//Computed value which depends on the selected furniture (in Furniture)
	int MinRoomSide;
};

/**
 * Groups all the information needed to define a furniture in the system.
 */
USTRUCT(BlueprintType)
struct FFurniture
{
	GENERATED_BODY()

	//Ordered list of all the dependencies for this furniture.
	//The dependencies with the lowest index are the highest priority.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FFurnitureDependency> Dependencies;

	//Unordered list of the mesh for this furniture.
	//The order has no importance because the array will be shuffled before being read.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UFurnitureMeshAsset *> Mesh;

	//Default constraints for this furniture which will be applied to the mesh if they don't override it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FFurnitureConstraint DefaultConstraints;
};

/**
 *
 */
UCLASS(BlueprintType)
class HOMEGENERATION_API AHomeGenerator : public AActor
{
	GENERATED_BODY()
	//TODO : Add categories !!

public:
	// Sets default values for this actor's properties
	AHomeGenerator();

	//TODO : The input data must be public or protected ?

	///______________________
	///Building data
	///

	//TODO : Comment
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FBuildingConstraint BuildingConstraints;
	
	//TODO : Comment
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FFurniture Stairs;

	//TODO : Comment
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FFurniture Doors;

	//TODO : Add window data

	///______________________
	///Room data
	///

	//TODO : Comment
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, FRoom> Rooms;

	//TODO : Add advanced division parameters (hall %,...)

	///______________________
	///Furniture
	///

	//TODO : Comment
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, FFurniture> Furniture;

	//TODO: Add advanced furniture positioning parameter (wall handling, bounds threshold and completion)

protected:
	// Called when the game starts or when spawned

	virtual void BeginPlay() override;

	///______________________
	///Initial step
	///

	//Computes the bounds for each mesh of every FurnitureMeshAsset or WindowMeshAsset
	void ComputeBounds();

	///______________________
	///Array helper
	///

	//Generate the list of the integers in the range : [Start; Stop[
	//Acts like python function : range(start, stop)
	static void GenerateRangeArray(TArray<int32> &InArray, int32 Start, int32 Stop);

	//Generate the list of the integers in the range : [0; Stop[
	//Acts like python function : range(stop)
	static void GenerateRangeArray(TArray<int32> &InArray, int32 Stop);

	//Shuffle the element of a given array
	template<typename T>
	static void ShuffleArray(TArray<T> &InArray);
	
	///______________________
	///Building Step
	///
	
	///______________________
	///Rooms step
	///
	
	///______________________
	///Furniture Step
	///

	//Generate and place all the furniture and decoration for a room.
	virtual void GenerateRoom(const FName &RoomType, const FRoomBlock &RoomBlock);

	//Depending on the needed rooms, randomly place the non placed doors, and spawn it.
	//Starts to fill the grid.
	virtual void GenerateRoomDoors(const FName &RoomType, const FRoomBlock &RoomBlock, FRoomGrid &RoomGrid);

	//Place all the needed furniture for a room and their dependencies.
	virtual void GenerateFurniture(const FName &RoomType, const FVector &RoomOrigin, FRoomGrid &RoomGrid);

	//Spawns the correct actor (with the correct component) and return it.
	//It will be placed according to the given rect and then attached to the AHomeGenerator
	virtual AActor *PlaceMeshInWorld(const UFurnitureMeshAsset *MeshAsset, const FFurnitureRect &FurnitureRect, const FVector &RoomOffset);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};

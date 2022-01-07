// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FurnitureMeshAsset.h"
#include "WindowMeshAsset.h"
#include "DecoBase.h"
#include "HGInternalStruct.h"
#include "HomeGenerator.generated.h"

/**
 * Groups all the information needed to define the global building shape.
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
	//TODO : Can be set but must be superior to another calculated value
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int MinSideFloorLength;

	//Ground thickness of one floor
	//In UE unit
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FloorWidth;

	//Height between the floor and the ceiling of a room. The total height between two floors is thus : FloorWidth + FloorHight
	//In UE unit
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FloorHeight;

	//Walls thickness
	//In UE unit
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WallWidth;

protected://Generated data by the procedural generator

};
};

/**
 * Groups all the information needed to define a room.
 */
USTRUCT(BlueprintType)
struct FRoom
{
	GENERATED_BODY()

	FRoom() : NumPerHab(1.f), MinRoomSide(0) {}

	//Ordered list of all furniture which may be placed in this room's type : the furniture with the lowest index will be the first to be placed (more chances to success)
	//Must be seen as a priority list of the furniture (the more important furniture have to be placed in first)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> Furniture;

	//Decoration class which will return the correct texture for the given room
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UDecoBase> DecorationClass;

	//Indicates the number of times per inhabitant that the "room" should be created, floating value allows more progressive augmentation (like for kitchen : 1 for 3 to 6 hab. and 2 for 8)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="NumberPerInhabitant", ClampMin="0.0", ClampMax="5.0"))
	float NumPerHab;

	//Getter to MinRoomSide
	int GetMinimalSide() const;

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
 * Groups all the information needed to define a window in the system.
 */
USTRUCT(BlueprintType)
struct FWindow
{
	GENERATED_BODY()
	
	//Unordered list of the mesh for the window.
	//The order has no importance because the array will be shuffled before being read.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UWindowMeshAsset *> Mesh;

	//Default constraints for this furniture which will be applied to the mesh if they don't override it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FWindowConstraint DefaultConstraints;
};

/**
 *
 */
UCLASS(BlueprintType)
class HOMEGENERATION_API AHomeGenerator : public AActor
{
	GENERATED_BODY()
	//TODO : Add categories !!
	//TODO : Instead of using assert, launch exceptions (avoid editor crashes)

public:
	// Sets default values for this actor's properties
	AHomeGenerator();

	//TODO : The input data must be public or protected ?

	//Number of inhabitants which should live in this home (it's mainly a size's indicator)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1.0"))
	int Inhabitants;

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

	//TODO : Comment
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FWindow Windows;

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

	//Computes the bounds for each mesh of every FurnitureMeshAsset or WindowMeshAsset.
	//Function to use really carefully : it takes a lot of time.
	void ComputeBounds();

	//Compute all minimal side and define sort element based on this value (ex : rooms)
	void ComputeSides();

	//Generated data is stored in the RoomsDivisionConstraintStruct
	//Desired quantity of rooms, according to the number per inhabitant and the number of inhabitants.
	int NormalRoomQuantity;

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

	//Generated dimensions of the building
	FVectorGrid BuildingSize;
	int Levels;

	//Selected furniture (stairs, windows and doors)
	UPROPERTY() UFurnitureMeshAsset *SelectedStair;
	UPROPERTY() UFurnitureMeshAsset *SelectedDoor;
	UPROPERTY() UWindowMeshAsset *SelectedWindow;
	
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

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

/**
 * Groups all the information needed to control the behavior of the room division system.
 * All the indicated constraints are applied per level, not globally.
 */
USTRUCT(BlueprintType)
struct FRoomsDivisionConstraints
{
	GENERATED_BODY()
	
	FRoomsDivisionConstraints();

	//Wanted width of a hall (corridor between rooms in a floor)
	//In grid square
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int HallWidth;

	//Maximum ration of the surface occupied by the halls on the total area.
	//In percent : ]0; 1[
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="MinimalSideCoefficient", ClampMin="0.0", ClampMax="1.0"))
	float MaxHallRatio;

	//Defines the absolute minimal side : any room can have a side under this.
	//ABSMinSide = MinSideCoef * CalcMinimalSide
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="MinimalSideCoefficient", ClampMin="1.0"))
	float MinSideCoef;

	//Defines the stop split side : any room with a side under this can't be split, only divided
	//StopSplitSide = StopSplitCoef * CalcMinimalSide
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="StopSplitCoefficient", ClampMin="1.0"))
	float StopSplitCoef;

	//Defines the sufficient side : any room with a side above this must be split or divided
	//SufficientSide = SufficientChunkCoef * CalcMinimalSide
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="SufficientChunkCoefficient", ClampMin="2.0"))
	float SufficientChunkCoef;

	//When the side of a block is under the sufficient side, it gives us the probability to continue to divide.
	//In percent : [0; 1]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="OverDivideProbability", ClampMin="0.0", ClampMax="1.0"))
	float OverDivideProba;
	
protected://Generated data by the procedural generator
	//Calculated minimal side from the rooms information
	int BasicMinimalSide;

	//Absolute minimal side
	int ABSMinimalSide;

	//Side under which the split will be stopped
	int StopSplitSide;

	//Sufficient side
	int SufficientSide;

	friend class AHomeGenerator;
	friend struct FUnknownBlock;
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

	//TODO : Comment
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRoomsDivisionConstraints RoomsDivisionConstraints;

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

	virtual void DefineRooms();

	//Defines the position of the stairs and the first halls (stairs must be connected to at least one hall)
	//These position are used by all levels
	virtual void StairsPositioning(TArray<FUnknownBlock> & InitialBlocks, TArray<FHallBlock> &InitialHalls);
	
	//TODO: When creating caller function, check the best way to pass the argument
	//Divide given level into a list of room and halls
	virtual void DivideSurface(int Level, const TArray<FUnknownBlock> &InitialBlocks, const TArray<FHallBlock> &InitialHalls);

	//Called when all levels have been divided to define the type for each room block created.
	//Acts globally on all levels simultaneously
	virtual void AllocateSurface();

	//TODO : Remove wall
	//TODO : Add windows and decoration
	virtual void CompleteHallSurface(); //31/12/2021 : No fucking idea how to do this shit

	//All halls in the building by level (first index)
	TArray<TArray<FHallBlock>> HallBlocks;

	//All rooms in the building by level (first index)
	TArray<TArray<FRoomBlock>> RoomBlocks;

	//TODO : Missing step where we define the door furniture and connect the doors between each other
	//TODO : Missing step where we generate the wall/floor
	
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

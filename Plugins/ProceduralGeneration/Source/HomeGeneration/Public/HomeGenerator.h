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
 * The constraints indicated in this structure are not absolute : they will be ignored if some other calculated value over-constraint the calculus.
 */
USTRUCT(BlueprintType)
struct FBuildingConstraint
{
	GENERATED_BODY()

	//Size of the side of a square in the construction grid
	//In UE unit
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GridSnapLength;

	//Minimal size of the side of a floor
	//If set to 0 won't be taken in account
	//In grid square
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int MinSideFloorLength = 0;

	//Maximal size of the side of a floor
	//ENH : If set to 0 won't be taken in account
	//In grid square
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int MaxSideFloorLength = 0;

	//Maximal number of floors in the generated building
	//If set to 0 won't be taken in account
	//In grid square
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int MaxFloorsNumber = 0;

	//Ground thickness of one floor
	//In UE unit
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FloorWidth = 10.f;

	//Height between the floor and the ceiling of a room. The total height between two floors is thus : FloorWidth + FloorHight
	//In UE unit
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FloorHeight = 240.f;

	//Walls thickness
	//In UE unit
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WallWidth = 10.f;

protected://Generated data by the procedural generator
	//Room average minimal side : an average of all minimal side (for each room) pondered by the quantity these rooms
	int AverageSide = 0;

	//Total desired quantity of rooms, according to the number per inhabitant and the number of inhabitants.
	int NormalRoomQuantity = 0;
	
	//Building dimensions
	FVectorGrid BuildingSize; //In grid square
	int Levels = 0;

	friend class AHomeGenerator;
};

/**
 * Groups all the information needed to control the behavior of the room division system.
 * All the indicated constraints are applied per level, not globally.
 */
USTRUCT(BlueprintType)
struct FRoomsDivisionConstraints
{
	GENERATED_BODY()
	FRoomsDivisionConstraints() = default;

	//Wanted width of a hall (corridor between rooms in a floor)
	//In grid square
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int HallWidth = 0;

	//Maximum ration of the surface occupied by the halls on the total area.
	//In percent : ]0; 1[
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="MinimalSideCoefficient", ClampMin="0.0", ClampMax="0.6"))
	float MaxHallRatio = 0.4f;

	//Defines the absolute minimal side : any room can have a side under this (be careful if you choose a value under 1)
	//ABSMinimalSide = MinSideCoef * BasicMinimalSide
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="MinimalSideCoefficient", ClampMin="0.0"))
	float MinSideCoef = 1.f;

	//Defines the stop split side : any room with a side under this can't be split, only divided
	//StopSplitSide = StopSplitCoef * BasicAverageSide
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="StopSplitCoefficient", ClampMin="0.0"))
	float StopSplitCoef = 1.f;

	//Defines the sufficient side : any room with a side above this must be split or divided (under it is random)
	//SufficientSide = SufficientChunkCoef * BasicMaximalSide
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="SufficientChunkCoefficient", ClampMin="0.0"))
	float SufficientChunkCoef = 1.f;

	//When the side of a block is under the sufficient side, it gives us the probability to continue to divide.
	//In percent : [0; 1]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="OverDivideProbability", ClampMin="0.0", ClampMax="1.0"))
	float OverDivideProba = 0.5f;
	
protected://Generated data by the procedural generator
	//Absolute minimal side
	int ABSMinimalSide = 0;

	//Side under which the split will be stopped
	int StopSplitSide = 0;

	//Sufficient side
	int SufficientSide = 0;

	//Computes previously indicated calculus
	void CalculateAllSides(const int _BasicMinimalSide, const int _BasicAverageSide, const int _BasicMaximalSide);
	
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

	FRoom() = default;

	//Ordered list of all furniture which may be placed in this room's type : the furniture with the lowest index will be the first to be placed (more chances to success)
	//Must be seen as a priority list of the furniture (the more important furniture have to be placed in first)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> Furniture;

	//Decoration class which will return the correct texture for the given room
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UDecoBase> DecorationClass;

	//Indicates the number of times per inhabitant that the "room" should be created, floating value allows more progressive augmentation (like for kitchen : 1 for 3 to 6 hab. and 2 for 8)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName="NumberPerInhabitant", ClampMin="0.0", ClampMax="5.0"))
	float NumPerHab = 1.f;

	//Recalculate MinRoomSide
	int CalculateMinimalSide(TMap<FName, FFurniture> &FurnitureMapping);

	//Getter to MinRoomSide
	int GetMinimalSide() const;

protected:
	//Computed value which depends on the selected furniture (in Furniture)
	//For one room
	int MinRoomSide = 0;
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

	//Indicates the average area of all its meshes
	int GetAverageArea();

protected:
	int AverageArea = 0;
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
	//ENH : Instead of using assert, launch exceptions (avoid editor crashes) or at least UE_CHECK 

public:
	// Sets default values for this actor's properties
	AHomeGenerator();

	//Number of inhabitants which should live in this home (it's mainly a size's indicator)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1.0"))
	int Inhabitants;

	///______________________
	///Building data
	///

	//Groups all needed data on the general building shape
	//A part is set by the user and the other is calculated
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FBuildingConstraint BuildingConstraints;
	
	//Special furniture structure which represents the stairs (shouldn't be added in the normal furniture map).
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FFurniture Stairs;

	//Special furniture structure which represents the doors (shouldn't be added in the normal furniture map).
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FFurniture Doors;

	//Special furniture structure which represents the windows (shouldn't be added in the normal furniture map).
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FWindow Windows;

	///______________________
	///Room data
	///

	//List of all room type represented by their name.
	//A room type is defined by the type of furniture it contains, its decoration, and its occupation the building.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, FRoom> Rooms;

	//Groups all needed data to be able to divide and allocate the space in the building for each needed room.
	//A part is set by the user and the other is calculated.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRoomsDivisionConstraints RoomsDivisionConstraints;

	///______________________
	///Furniture
	///

	//List of all furniture type represented by their name.
	//A furniture type is defined by all the meshes by which it can be represented, its dependencies and its constraints.
	//The dependencies of a furniture are the some indicated furniture type the system will try to place near this furniture.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, FFurniture> Furniture;

	//ENH: Add advanced furniture positioning parameter (bounds threshold and completion)

protected:
	// Called when the game starts or when spawned

	virtual void BeginPlay() override;

	///______________________
	///Initial step
	///

	//Computes the bounds for each mesh of every FurnitureMeshAsset or WindowMeshAsset.
	//Function to use really carefully : it takes a lot of time.
	void ComputeBounds();

	//Computes all minimal sides and sort elements based on this value (ex : rooms)
	//Also computes additional room's constants
	void ComputeSides();

	//Generated data is stored in the RoomsDivisionConstraintStruct or in BuildingConstraintStruct
	
	///______________________
	///Building Step
	///

	virtual void DefineBuilding();

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
	virtual void StairsPositioning(FLevelOrganisation &InitialOrganisation);
	
	//Divide given level into a list of room and halls
	virtual void DivideSurface(const int Level, FLevelOrganisation& LevelOrganisation, TDoubleLinkedList<FUnknownBlock>& NodesToDelete);

	//Computes for each block (room, level, ...) their real offset.
	//This offset take in account the walls : in the grid system they don't have any width.
	virtual void ComputeWallEffect(TArray<FLevelOrganisation> &LevelsOrganisation);

	//Called when all levels have been divided to define the type for each room block created.
	//Acts globally on all levels simultaneously
	virtual void AllocateSurface();

	//TODO : Remove wall
	//TODO : Add windows and decoration
	virtual void CompleteHallSurface();

	//31/12/2021 : No fucking idea how to do this shit
	//01/02/2022 Lol, progress : 0%
	//05/02/2022 : Easy : just add placo around rooms : really thick walls just for deco, rest of walls will be empty, same for floor and ceil.

	//All halls in the building by level (first index)
	TArray<TArray<FHallBlock>> HallBlocks;

	//All rooms in the building by level (first index)
	TArray<TArray<FRoomBlock>> RoomBlocks;

	//TODO : Missing step where connect the doors between each other
	//TODO : Missing step where we generate the wall/floor -> done for each room
	
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
};

/**
 * Inline definitions of template functions
 */
template <typename T>
void AHomeGenerator::ShuffleArray(TArray<T>& InArray)
{
	if (InArray.Num() > 0)
	{
		for (int32 i = 0; i < InArray.Num(); ++i)
		{
			int32 Index = FMath::RandRange(i, InArray.Num() - 1);
			if (i != Index)
				InArray.Swap(i, Index);
		}
	}
}

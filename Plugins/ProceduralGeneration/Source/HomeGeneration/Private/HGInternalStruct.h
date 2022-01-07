//Fill Copyright

#pragma once

#include "HGBasicType.h"
#include "CoreMinimal.h"
#include "FurnitureMeshAsset.h"

//TODO : Think to a better organisation of the internal structs files (HGInternalStruct and HGBasicStruct)

//Home generator serialised structures
struct FBuildingConstraint;
struct FRoomsDivisionConstraints;
struct FWindow;

struct FBasicBlock
{
	FBasicBlock();
	FBasicBlock(const FVectorGrid &_Size, const FVectorGrid &_GlobalPosition, int _Level);

	FVectorGrid Size;
	FVectorGrid GlobalPosition;
	int Level;//Starts at 0
};

//In degrees
enum class EFurnitureRotation : uint8
{
	ROT0,
	ROT90,
	ROT180,
	ROT270
};

enum class ERoomCellType : uint8
{
	OBJECT,
	MARGIN,
	EMPTY
};

struct FRoomCell
{
	//When created a cell is empty
	FRoomCell();
	
	ERoomCellType Type;
	uint32 FurnitureDependencyMarker;

	static uint32 IndexToMarker(uint8 FurnitureIndex);
};

struct FFurnitureRect
{
	//By default generate a vector null : (0°, 0, 0)
	FFurnitureRect();
	FFurnitureRect(EFurnitureRotation _Rotation, const FVectorGrid &_Position, const FVectorGrid &_Size);

	//Clock-wise rotation
	EFurnitureRotation Rotation;
	//Positions in grid starts at index 0
	FVectorGrid Position;
	//Non-rotated data
	FVectorGrid Size;

	//Return true if the rotation causes a switch between sizes when rotating data.
	bool WillRotationInvertSize() const;
};

/**
 * Represents a room being filled by its furniture. It allows the system to check if the proposed position respect all the constraints.
 * The public functions only mark the grid if the position respects the given constraints.
 */
struct FRoomGrid
{
	FRoomGrid() = delete;
	explicit FRoomGrid(int SizeX, int SizeY);
	explicit FRoomGrid(const FVectorGrid &RoomSize);
	virtual ~FRoomGrid() = default;
	
	virtual int GetSizeX() const;
	virtual int GetSizeY() const;

	virtual bool MarkFurnitureAtPosition(const FFurnitureRect &Position, const FFurnitureConstraint &Constraints, uint8 DependencyMarker = 0);
	virtual bool MarkDependencyAtPosition(const FFurnitureRect &Position, const FFurnitureRect &ParentPosition, const FFurnitureConstraint &Constraints,const FFurnitureDependency &DependencyConstraints, uint8 DependencyMarker);
	virtual bool MarkDoorAtPosition(const FFurnitureRect &Position);

	//Returns true if the given FurnitureRect is along the wall of the given room's side (as axis or with the correct function)
	bool IsAlongAxeWall(const FFurnitureRect &Position, const EGenerationAxe Axe) const;
	bool IsAlongXUpWall(const FFurnitureRect &Position) const;
	bool IsAlongXDownWall(const FFurnitureRect &Position) const;
	bool IsAlongYUpWall(const FFurnitureRect &Position) const;
	bool IsAlongYDownWall(const FFurnitureRect &Position) const;
	bool IsInAnyCorner(const FFurnitureRect &Position) const; //Other are useless

	//Checks if a rect is inside the room
	bool CheckLimits(const FFurnitureRect &Position) const;
protected:
	//Grid[X][Y]
	TArray<TArray<FRoomCell>> Grid;

	//Rotate the input data and paste the result in the 'Rotated' structures
	//The goal of this function is to obtain some structures that represent the same block (with position, margin wall, ...) but on which no rotation information are needed.
	//The returned position has a rotation parameter of 0° (allows next functions to make checks)
	static void RotateData(const FFurnitureRect &InPosition, const FFurnitureConstraint &InConstraints, FFurnitureRect &RotatedPosition, FFurnitureConstraint &RotatedConstraints);

	//Rotate the input data and paste the result in the 'Rotated' structures
	//The goal of this function is to obtain some structures that represent the same block (with position, margin wall, ...) but on which no rotation information are needed.
	//The returned position has a rotation parameter of 0° (allows next functions to make checks)
	static void RotateDependencyData(const FFurnitureRect &InParentPosition, const FFurnitureDependency &InDependency, FFurnitureRect &RotatedParentPosition, FFurnitureDependency &RotatedDependency);

	//Create the rect of a furniture including its margin
	static void GenerateMarginRect(const FFurnitureRect& RotatedPosition, const FMarginStruct& RotatedMargin, FFurnitureRect &MarginRect);

	//Checks a furniture position (regardless of its margin).
    //The Dependency Marker must be 0 for normal furniture and the marker of their parent for the dependencies (checks between the given dependency an its parent).
    //Only send rotated data !!
	virtual bool CheckFurnitureRect(const FFurnitureRect &RotatedPosition, const FWallInteractionStruct &RotatedInteraction, uint8 DependencyMarker = 0) const;

	//Checks the margin position for a given furniture.
    //The Dependency Marker must be 0 for normal furniture and the marker of their parent for the dependencies (checks between the given dependency an its parent).
	//Only send rotated data !!
	virtual bool CheckMarginRect(const FFurnitureRect &RotatedPosition, const FMarginStruct &RotatedMargin, uint8 DependencyMarker = 0) const;

	//Checks the position of the given dependency with respect to its parent (doesn't check the existence of the parent)
	//The Dependency Marker must be the marker of the parent of the given dependency (checks between the given dependency an its parent).
	//Only send rotated data !!	
	virtual bool CheckDependencyConstraints(const FFurnitureRect &RotatedPosition, const FFurnitureRect &RotatedParentPosition, const FFurnitureDependency &RotatedDependency, uint8 DependencyMarker) const;

	//Marks the grid's cells of the indicated rect as object position in the grid.
	//The Dependency Marker must be 0 for dependencies and furniture with no dependencies and, for the others, their index (starting at 1)  in the list of furniture with dependencies (mark the grid for the dependencies of the furniture).
    //Only send rotated data !!
	virtual void MarkRect(const FFurnitureRect &RotatedPosition, ERoomCellType CellType, uint8 DependencyMarker = 0);
};

struct FRoomBlock;

struct FDoorBlock
{
	//Prevents from creating a door from nothing
	FDoorBlock() = delete;
	
	//Initialises all constant members but set the position to an invalid value (to detect if this door has been already positioned or not)
	FDoorBlock(const FRoomBlock *_MainParent, const FRoomBlock *_SecondParent, const EGenerationAxe _OpeningSide, UFurnitureMeshAsset * _DoorAsset);

	//Transforms the given local position (according to the indicated RoomBlock) and saves it as global.
	//Doesn't do anything if the coordinates are already valid.
	void SaveLocalPosition(const FVectorGrid &LocalPosition, const FRoomBlock &Parent);

	//Marked the door as spawned (allows differentiation between setting position and spawning door).
	void MarkAsPlaced();

	//Indicates if the door has been already spawned.
	bool IsPlaced() const;

	//Indicates if the position of this door has been already correctly set.
	bool IsPositionValid() const;

	//Returns true if the indicated room is a parent of the room
	bool IsRoomParent(const FRoomBlock &Parent) const;

	//Only valid when the door has been placed, represent the door in itself (not its margin)
	//That means that the given position could technically be an invalid position for a normal furniture.
	FFurnitureRect GenerateLocalFurnitureRect(const FRoomBlock &Parent) const;

	//Gives the size of the margin of the room for the given room block.
	FVectorGrid GenerateLocalMarginSize(const FRoomBlock &Parent, const FMarginStruct &DefaultDoorMargin) const;

	//Returns the wall of the given room in which the door must be placed.
	EGenerationAxe RoomWallAxe(const FRoomBlock &Parent) const;

	//Returns the other parent of this door (nullptr if it is a hall)
	const FRoomBlock *ObtainOppositeParent(const FRoomBlock &Parent) const;

	//Returns the mesh of this door
	UFurnitureMeshAsset *GetMeshAsset() const;
protected:
	//Linked rooms
	const FRoomBlock * const ParentMain;
	const FRoomBlock * const ParentSecond;

	//Position
	const EGenerationAxe OpeningSide;
	bool Placed;
	FVectorGrid GlobalPosition;

	//In actual configuration, useless. however it allows multiple mesh for the doors in future system
	UFurnitureMeshAsset * const DoorAsset;
};

struct FRoomBlock : FBasicBlock
{
	FRoomBlock() = default;
	FRoomBlock(const FVectorGrid &_Size, const FVectorGrid &_GlobalPosition, int _Level);
	
	TArray<FDoorBlock *> ConnectedDoors;
	FName RoomType;

	FVector GenerateRoomOffset(const FBuildingConstraint &BuildData) const;

	//Comparison by minimal side to allow sorting
	bool operator<(const FRoomBlock &B) const;
};

struct FDependencyBuffer
{
	FDependencyBuffer(const TArray<FFurnitureDependency> &_Dependencies, const FFurnitureRect &_ParentPosition);
	
	const TArray<FFurnitureDependency> &Dependencies;
	const FFurnitureRect ParentPosition;
};

/**
 * Structures used to divide a level into rooms.
 */

struct LevelDivisionData
{
	//Total area occupied by the halls in the current level.
	int HallTotalArea;

	//Nothing else for instance...
};

struct FHallBlock : FBasicBlock
{
	FHallBlock()= default;
	FHallBlock(const FVectorGrid &_Size, const FVectorGrid &_GlobalPosition, int _Level);

	//Checks if the windows of this hall have been already placed
	bool AreWindowSpawned();

	//Choose, place and spawned the needed windows for this hall
	void AddWindow();
};

struct FUnknownBlock : FBasicBlock
{
	FUnknownBlock();
	FUnknownBlock(const FVectorGrid &_Size, const FVectorGrid &_GlobalPosition, int _Level, bool _AlongX);

	enum class DivideMethod : uint8
	{
		ERROR,		//Default value which is returned if an error is detected.
		NO_DIVIDE,	//The current block shouldn't be divided anymore (should be transformed into room).
		SPLIT,		//The block must be divide using the split method (hall creation).
		DIVISION	//The block must be divide using the division method (no hall creation).
	};

	//Check if it is possible to divide this block and using which method (minimal side's size,...)
	//Partially random (for the block that could be divide or stopped, depending on the wanted size)
	DivideMethod ShouldDivide(const FRoomsDivisionConstraints &DivisionCst) const;//TODO : Need constraints and stats about the current floor

	//Try to split the current block : create a hall in between the two new blocks.
	//The current block become on of the two new created and the other one is returned.
	bool BlockSplit(const FRoomsDivisionConstraints& DivisionCst, FUnknownBlock &SecondResultedBlock, FHallBlock &ResultHall);

	//Try to make a division into the current block : no hall is created.
	//The current block become on of the two new created and the other one is returned.
	bool BlockDivision(const FRoomsDivisionConstraints& DivisionCst, FUnknownBlock &SecondResultedBlock);

	//Once this block is stopped by the system, it might be transformed to a room
	void TransformToRoom(FRoomBlock &CreatedRoom) const;

protected:
	//Indicate along which axis we should divide (set by the previous block)
	bool DivideAlongX;
	
	//Is set to true when the block results of a division and not a split (no hall created)
	//It means the block can't no more be divided using a split method but only with a division
	bool NoMoreSplit;

	//Used to ensure that the user of the class call the right method (error = no decision)
	mutable DivideMethod DivideDecision;
};
//Fill Copyright

#pragma once

#include "HGBasicType.h"
#include "CoreMinimal.h"
#include "FurnitureMeshAsset.h"

//ENH : Think to a better organisation of the internal structs files (HGInternalStruct and HGBasicStruct)

//Home generator serialised structures
struct FBuildingConstraint;
struct FRoomsDivisionConstraints;
struct FWindow;

//Local structures
struct FRoomBlock;
struct FUnknownBlock;
struct FLevelOrganisation;

struct FBasicBlock
{
	FBasicBlock() = default;
	FBasicBlock(const FVectorGrid &_Size, const FVectorGrid &_GlobalPosition, int _Level);

	//Basic grid data
	FVectorGrid Size;
	FVectorGrid GlobalPosition;
	int Level = 0;//Starts at 0

	//Geters
	const FVector2D& GetRealSize() const;
	const FVector2D& GetRealOffset() const;
	
protected:
	//Advance grid data (taking in account the walls)
	FVector2D RealSize;
	FVector2D RealOffset; //Start from origin (not from grid location)
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
	FRoomCell() = default;
	
	ERoomCellType Type = ERoomCellType::EMPTY;
	uint32 FurnitureDependencyMarker = 0;

	static uint32 IndexToMarker(uint8 FurnitureIndex);
};

struct FFurnitureRect
{
	//By default generate a vector null : (0°, 0, 0)
	FFurnitureRect() = default;
	FFurnitureRect(EFurnitureRotation _Rotation, const FVectorGrid &_Position, const FVectorGrid &_Size);

	//Clock-wise rotation
	EFurnitureRotation Rotation = EFurnitureRotation::ROT0;
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
	bool Placed = false;
	FVectorGrid GlobalPosition = FVectorGrid(-1, -1);

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

	friend FUnknownBlock;
};

struct FDependencyBuffer
{
	FDependencyBuffer() = delete;
	FDependencyBuffer(const TArray<FFurnitureDependency> &_Dependencies, const FFurnitureRect &_ParentPosition);
	
	const TArray<FFurnitureDependency> &Dependencies;
	const FFurnitureRect ParentPosition; //Can't be reference
};

/**
 * Structures used to divide a level into rooms.
 */

struct FLevelDivisionData
{
	FLevelDivisionData() = delete;
	FLevelDivisionData(int _LevelTotalArea, int _HallArea);
	
	//Total area occupied by the halls in the current level.
	int HallTotalArea;

	//Total area available in a level
	const int LevelTotalArea;

	//Returns HallTotalArea/LevelTotalArea (not int division)
	float GetFutureHallRatio(int HallArea) const;

	//Nothing else for instance...
};

/**
 * Represents a hall : a space between rooms with special decoration
 * Real sizes does not include wall (just a space), offsets starts at the interior of the block (inside the wall)
 */
struct FHallBlock : FBasicBlock
{
	FHallBlock()= default;
	FHallBlock(const FVectorGrid &_Size, const FVectorGrid &_GlobalPosition, int _Level);

	//Checks if the windows of this hall have been already placed
	bool AreWindowSpawned();

	//Choose, place and spawned the needed windows for this hall
	void AddWindow();

	friend FUnknownBlock;
	friend FLevelOrganisation;
};

/**
 * Represents a block being built : a "step" of the division of a level process
 * Real sizes includes wall, offsets starts at the interior of the block (inside the wall)
 */
struct FUnknownBlock : FBasicBlock
{
	FUnknownBlock() = default;
	FUnknownBlock(const FVectorGrid &_Size, const FVectorGrid &_GlobalPosition, int _Level, bool _AlongX);

	enum class DivideMethod : uint8
	{
		ERROR,		//Default value which is returned if an error is detected.
		NO_DIVIDE,	//The current block shouldn't be divided anymore (should be transformed into room).
		SPLIT,		//The block must be divide using the split method (hall creation).
		DIVISION	//The block must be divide using the division method (no hall creation).
	};

	///
	///Division part
	///

	//Check if it is possible to divide this block and using which method (minimal side's size,...)
	//Partially random (for the block that could be divide or stopped, depending on the wanted size)
	//If it decides to split, it updates automatically the FLevelDivisionData struct
	DivideMethod ShouldDivide(const FRoomsDivisionConstraints &DivisionCst, FLevelDivisionData &DivisionData) const;

	//Try to split the current block : create a hall in between the two new blocks.
	//The current block is kept and the two created are returned.
	bool BlockSplit(const FRoomsDivisionConstraints& DivisionCst, FUnknownBlock &FirstResultedBlock, FUnknownBlock &SecondResultedBlock, FHallBlock &ResultHall);
	
	//Try to make a division into the current block : no hall is created.
	//The current block is kept and the two created are returned.
	bool BlockDivision(const FRoomsDivisionConstraints& DivisionCst,  FUnknownBlock &FirstResultedBlock, FUnknownBlock &SecondResultedBlock);

	//Once this block is stopped by the system, it might be transformed to a room
	void TransformToRoom(FRoomBlock &CreatedRoom);

	///
	///Calculation part
	///

	//Calculates the dimensions of this block using its child blocks
	void ComputeRealSizeRecursive(const FBuildingConstraint &BuildingCst, const FRoomsDivisionConstraints &RoomDivisionCst); //Upward phase

	//Generates the needed data to create walls (using previously calculated sizes)
	//Thus are calculated : new offset for the rooms.
	void ComputeRealOffsetRecursive(const FBuildingConstraint& BuildingCst, const FRoomsDivisionConstraints& RoomDivisionCst, const bool IsHighestAxe) const; //Downward phase
	
protected:
	//Indicate along which axis we should divide (set by the previous block)
	bool DivideAlongX = true;
	
	//Is set to true when the block results of a division and not a split (no hall created)
	//It means the block can't no more be divided using a split method but only with a division
	bool NoMoreSplit = false;

	//Used to ensure that the user of the class call the right method (error = no decision)
	mutable DivideMethod DivideDecision = DivideMethod::ERROR;

	//Linked blocks
	FUnknownBlock *Parent = nullptr;
	FUnknownBlock *Child1 = nullptr;
	FUnknownBlock *Child2 = nullptr;
	FHallBlock *HallBlock = nullptr;
	FRoomBlock *Room = nullptr; //Only for last blocks
	//ENH : Maybe add an union

	friend FLevelOrganisation;
};

struct FLevelOrganisation
{
	enum EInitialBlockPositions { LowWing, HighWing, LowApartment, HighApartment, BlockPositionsSize};
	enum EInitialHallPositions { LowCorridor, HighCorridor, LowMargin, HighMargin, Stairs, HallPositionsSize};
	
	FLevelOrganisation();

	///
	///Lists management
	///

	//Setup the blocks
	void SetUnknownBlock(EInitialBlockPositions Index, FUnknownBlock *Block);
	void SetHallBlock(EInitialHallPositions Index, FHallBlock *Hall);

	//Access to index, error is given by BlockPositionsSize or HallPositionsSize
	EInitialBlockPositions GetUnknownBlockPosition(FUnknownBlock* Block) const;
	EInitialHallPositions GetHallBlockPosition(FHallBlock* Hall) const;

	//Access to list
	const TArray<FUnknownBlock *> &GetBlockList() const;
	const TArray<FHallBlock *> &GetHallList() const;

	//Empties both list. The delete boolean indicates if we should delete the pointed instance (be careful !)
	void Empty(bool bDelete = true);

	///
	///Data
	///

	//Returns the sum of the area of all initial halls (except stairs)
	int InitialHallArea() const;

	
	///
	///"Real" calculations
	///
	
	//Computes all sizes of all linked blocks and hall of this level (included the LevelOrganisation in itself)
	//Computes the offset of the stairs only
	//Be careful these calculated value aren't absolute : they must be update according to the stairs of each level.
	void ComputeBasicRealData(const FBuildingConstraint& BuildingCst, const FRoomsDivisionConstraints& RoomDivisionCst);

	//Using the given additional offset, the function will update the stairs' and level's offset
	//It will then calculate the offset for each block (and launch the recursive call)
	void ComputeAllRealData(const FVector2D &LevelOffset ,const FBuildingConstraint& BuildingCst, const FRoomsDivisionConstraints& RoomDivisionCst);

	//Returns the real offset of the Hall which represent the stairs
	const FVector2D &GetStairsRealOffset() const;
	
protected:
	//Real block data for this level
	FVector2D RealOffset = FVector2D::UnitVector;
	FVector2D RealSize = FVector2D::ZeroVector;

	//Included blocks
	TArray<FUnknownBlock *> InitialBlocks;
	TArray<FHallBlock *> InitialHalls;
	
	///   _________________________________________
	///  |          |   |           |   |         |
	///  |          | 1 |     D     | 2 |         |
	///  |          |   |___________|   |         |
	///  |     A    |    3    5    4    |    B    |
	///  |          |    ___________    |         |
	///  |          |   |           |   |         |
	///  |          | 1 |     C     | 2 |         |
	///  |__________|___|___________|___|_________|
	///
	///  A : LowWing (lowest position) 
	///  B : HighWing 
	///  C : LowApartment (lowest position on the other axis)
	///  D : HighApartment
	///
	///  1 : LowCorridor
	///  2 : HighCorridor
	///  3 : LowMargin
	///  4 : HighMargin
	///  5 : Stairs
};
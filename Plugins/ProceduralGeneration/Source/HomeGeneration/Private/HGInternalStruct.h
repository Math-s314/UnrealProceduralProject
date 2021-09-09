
#pragma once

#include "CoreMinimal.h"
#include "FurnitureMeshAsset.h"

//In degrees
enum EFurnitureRotation
{
	ROT0,
	ROT90,
	ROT180,
	ROT270
};

enum ERoomCellType
{
	OBJECT,
	MARGIN,
	EMPTY
};

struct FRoomCell
{
	FRoomCell();
	
	ERoomCellType Type;
	uint32 FurnitureDependencyMarker;
};

struct FVectorGrid
{
	FVectorGrid();
	
	int X;
	int Y;
};

struct FFurnitureRect
{
	FFurnitureRect();
	FFurnitureRect(EFurnitureRotation _Rotation, const FVectorGrid &_Position, const FVectorGrid &Size);
	
	EFurnitureRotation Rotation;
	FVectorGrid Position;
	FVectorGrid Size;//Non-rotated data
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
	virtual bool MarkDependencyAtPosition(const FFurnitureRect &Position, const FFurnitureConstraint &Constraints,const FFurnitureDependency &DependencyConstraints, uint8 DependencyMarker);
	virtual bool MarkDoorAtPosition(const FFurnitureRect &Position);
protected:
	TArray<TArray<FRoomCell>> Grid;

	//Rotate the input data and paste de result in the 'Rotate' structures
	//The goal of this function is to obtain some structures that represent the same block (with position, margin wall, ...) but on which no rotation information are needed.
	//The returned position has a rotation parameter of 0° (allows next functions to make checks)
	static void RotateData(const FFurnitureRect &InPosition, const FFurnitureConstraint &InConstraints, FFurnitureRect &RotatedPosition, FFurnitureConstraint &RotatedConstraints);

	//Checks a furniture position (regardless of its margin).
    //The Dependency Marker must be 0 for normal furniture and the marker of their parent for the dependencies (checks between the given dependency an its parent).
    //Only send rotated data !!
	virtual bool CheckFurnitureRect(const FFurnitureRect &RotatedPosition, const FWallInteractionStruct &RotatedInteraction, uint8 DependencyMarker = 0) const;

	//Checks the margin position for a given furniture.
    //The Dependency Marker must be 0 for normal furniture and the marker of their parent for the dependencies (checks between the given dependency an its parent).
	//Only send rotated data !!
	virtual bool CheckMarginRect(const FFurnitureRect &RotatedPosition, const FMarginStruct &RotatedMargin, uint8 DependencyMarker = 0) const;

	//Marks the furniture as object position in the grid.
	//The Dependency Marker must be 0 for dependencies and furniture with no dependencies and the marker of their parent for the other furniture (mark the grid for the dependencies of the furniture).
    //Only send rotated data !!
	virtual void MarkFurnitureRect(const FFurnitureRect &RotatedPosition, uint8 DependencyMarker = 0);

	//Marks the margin position for a given furniture (must be called after MarkFurnitureRect()).
	//The Dependency Marker must be 0 for dependencies and furniture with no dependencies and the marker of their parent for the other furniture (mark the grid for the dependencies of the furniture).
	//Only send rotated data !!
	virtual void MarkMarginRect(const FFurnitureRect &RotatedPosition, const FMarginStruct &RotatedMargin, uint8 DependencyMarker = 0);
};

struct FRoomBlock;

struct FDoorBlock
{
	FDoorBlock(const FRoomBlock *_MainParent, const FRoomBlock *_SecondParent, const EGenerationAxe _OpeningSide, UFurnitureMeshAsset * _DoorAsset);

	//Transforms the given local position (according to the indicated RoomBlock) and saves it as global.
	//Doesn't do anything if the coordinates are already valid.
	void SaveLocalPosition(const FVectorGrid &LocalPosition, const FRoomBlock &Parent);

	//Marked the door as spawned (allows differentiation between setting position and spawning door).
	void MarkAsPlaced();

	//Indicates if the door has been already spawned.
	bool IsPlaced();

	//Indicates if the position of this door has been already correctly set.
	bool IsPositionValid();

	//Only valid when the door has been placed, represent the door in itself (not its margin)
	//That means that the given position could technically be an invalid position for a normal furniture.
	FFurnitureRect GenerateLocalFurnitureRect(const FRoomBlock &Parent) const;

	//Give the size of the margin of the room for the given room block.
	FVectorGrid GenerateLocalMarginSize(const FRoomBlock &Parent) const;

	//Return the wall of the given room in which the door must be placed.
	EGenerationAxe RoomWallAxe(const FRoomBlock &Parent) const;

	//Return the other parent of this door (nullptr if it is a hall)
	const FRoomBlock *ObtainOppositeParent(const FRoomBlock &Parent) const;
protected:
	//Linked rooms
	const FRoomBlock * const ParentMain;
	const FRoomBlock * const ParentSecond;

	//Position
	const EGenerationAxe OpeningSide;
	FVectorGrid GlobalPosition;

	//In actual configuration, useless. however it allows multiple mesh for the doors in future system
	UFurnitureMeshAsset * const DoorAsset;
};

struct FRoomBlock
{
	FRoomBlock();
	FRoomBlock(const FVectorGrid &_Size, const FVectorGrid &_GlobalPosition, int _Level);
	
	FVector GenerateRoomOffset() const;
	const FVectorGrid &GetRoomSize() const;
	const FVectorGrid &GetRoomPosition() const;
	const int &GetRoomLevel() const;

protected:
	FVectorGrid Size;
	FVectorGrid GlobalPosition;
	int Level;

	//If nullptr -> a hall
	TArray<FDoorBlock *> ConnectedDoors;
};
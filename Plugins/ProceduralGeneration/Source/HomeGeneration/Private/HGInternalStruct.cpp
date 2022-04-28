//Copyright

#include "HGInternalStruct.h"
#include "HomeGenerator.h"
#include <assert.h>

#define CONSTRUCT_GRID(X,Y) check(X > 0 && Y > 0) Grid.SetNum(X); for (int i = 0; i < X; ++i) { Grid[i].SetNum(X); }

const FVector2D& FBasicBlock::GetRealSize() const
{
	return RealSize;
}

const FVector2D& FBasicBlock::GetRealOffset() const
{
	return RealOffset;
}

FBasicBlock::FBasicBlock(const FVectorGrid& _Size, const FVectorGrid& _GlobalPosition, int _Level)
	: Size(_Size), GlobalPosition(_GlobalPosition), Level(_Level) {}

uint32 FRoomCell::IndexToMarker(uint8 FurnitureIndex)
{
	if(FurnitureIndex > 32 || FurnitureIndex == 0)
		return 0;

	int Buffer = 1;
	for (int i = 0; i < FurnitureIndex - 1; ++i)
		Buffer *= 2;

	return Buffer;
}

FFurnitureRect::FFurnitureRect(EFurnitureRotation _Rotation, const FVectorGrid& _Position, const FVectorGrid& _Size) : Rotation(_Rotation), Position(_Position), Size(_Size) {}

bool FFurnitureRect::WillRotationInvertSize() const
{
	return (Rotation == EFurnitureRotation::ROT270) || (Rotation == EFurnitureRotation::ROT90);
}

FRoomGrid::FRoomGrid(int SizeX, int SizeY)
{
	CONSTRUCT_GRID(SizeX,SizeY)
}

FRoomGrid::FRoomGrid(const FVectorGrid& RoomSize)
{
	CONSTRUCT_GRID(RoomSize.X,RoomSize.Y)
}

int FRoomGrid::GetSizeX() const
{
	return Grid.Num();
}

int FRoomGrid::GetSizeY() const
{
	check(GetSizeX() > 0)
	return Grid[0].Num();
}

bool FRoomGrid::MarkFurnitureAtPosition(const FFurnitureRect& Position, const FFurnitureConstraint& Constraints, uint8 DependencyMarker)
{
	//Basic checks to avoid troubles
	check(GetSizeX() > 0 && GetSizeY() > 0)
	DependencyMarker = DependencyMarker > 32 ? 0 : DependencyMarker;

	if(Position.Size.X <= 0 || Position.Size.Y <= 0)
		return false;
	
	FFurnitureConstraint RotatedConstraints;
	FFurnitureRect RotatedPosition;

	//Rotate data, to get a furniture with no more rotation (allows simplification of the rest of the code)
	RotateData(Position, Constraints, RotatedPosition, RotatedConstraints);

	//Check the sent furniture
	if(!CheckFurnitureRect(RotatedPosition, RotatedConstraints.WallAxeInteraction) || !CheckMarginRect(RotatedPosition, RotatedConstraints.Margin))
		return false;

	//If everything is correct, mark the grid (margin then object)
	FFurnitureRect MarginRect;
	GenerateMarginRect(RotatedPosition, RotatedConstraints.Margin, MarginRect);
	
	MarkRect(MarginRect, ERoomCellType::MARGIN, DependencyMarker);
	MarkRect(RotatedPosition, ERoomCellType::OBJECT, DependencyMarker);
	return true;
}

bool FRoomGrid::MarkDependencyAtPosition(const FFurnitureRect& Position, const FFurnitureRect &ParentPosition, const FFurnitureConstraint& Constraints, const FFurnitureDependency& DependencyConstraints, uint8 DependencyMarker)
{
	//Basic checks to avoid troubles
	check(GetSizeX() > 0 && GetSizeY() > 0)
	DependencyMarker = DependencyMarker > 32 ? 0 : DependencyMarker;

	if(Position.Size.X <= 0 || Position.Size.Y <= 0)
		return false;
	
	FFurnitureConstraint RotatedConstraints;
	FFurnitureRect RotatedPosition;
	FFurnitureRect RotatedParentPosition;
	FFurnitureDependency RotatedDependency;

	//Rotate data, to get a furniture with no more rotation (allows simplification of the rest of the code)
	RotateData(Position, Constraints, RotatedPosition, RotatedConstraints);
	RotateDependencyData(ParentPosition, DependencyConstraints, RotatedParentPosition, RotatedDependency);

	//Check the sent furniture and its basic constraint (taking in account that it is a dependency)
	if(!CheckFurnitureRect(RotatedPosition, RotatedConstraints.WallAxeInteraction, DependencyMarker) || !CheckMarginRect(RotatedPosition, RotatedConstraints.Margin, DependencyMarker))
		return false;

	//Checks the dependency constraints
	if(!CheckDependencyConstraints(RotatedPosition, RotatedParentPosition, RotatedDependency, DependencyMarker))
		return false;

	//If everything is correct, mark the grid
	FFurnitureRect MarginRect;
	GenerateMarginRect(RotatedPosition, RotatedConstraints.Margin, MarginRect);
	
	MarkRect(MarginRect, ERoomCellType::MARGIN);
	MarkRect(RotatedPosition, ERoomCellType::OBJECT);
	return true;
}

bool FRoomGrid::MarkDoorAtPosition(const FFurnitureRect& Position)
{
	//Check limits
	if(!CheckLimits(Position))
		return false;
	
	//Checks if the rect is along any wall
	if(!IsAlongXDownWall(Position) && !IsAlongXUpWall(Position) && !IsAlongYDownWall(Position) && !IsAlongYUpWall(Position))
		return false;

	FVectorGrid RotatedSize = Position.Size;
	if(Position.WillRotationInvertSize())
	{
		RotatedSize.Y = Position.Size.X;
		RotatedSize.X = Position.Size.Y;
	}
	
	//Mark the grid
	for (int i = 0; i < RotatedSize.X; ++i)
		for (int j = 0; j < RotatedSize.Y; ++j)
			Grid[Position.Position.X + i][Position.Position.Y + j].Type = ERoomCellType::MARGIN;

	return true;
}

bool FRoomGrid::IsAlongAxeWall(const FFurnitureRect& Position, const EGenerationAxe Axe) const
{
	switch (Axe) {
		case EGenerationAxe::X_UP: return IsAlongXUpWall(Position);
		case EGenerationAxe::X_DOWN: return IsAlongXDownWall(Position);
		case EGenerationAxe::Y_UP: return IsAlongYUpWall(Position);
		case EGenerationAxe::Y_DOWN: return IsAlongYDownWall(Position);
		default: return false;
	}
}

bool FRoomGrid::IsAlongXUpWall(const FFurnitureRect& Position) const
{
	if(Position.WillRotationInvertSize())
		return Position.Position.X + Position.Size.Y == GetSizeX();
	
	return Position.Position.X + Position.Size.X == GetSizeX();
}

bool FRoomGrid::IsAlongXDownWall(const FFurnitureRect& Position) const
{
	return Position.Position.X == 0;
}

bool FRoomGrid::IsAlongYUpWall(const FFurnitureRect& Position) const
{
	if(Position.WillRotationInvertSize())
		return Position.Position.Y + Position.Size.X == GetSizeY();
	
	return Position.Position.Y + Position.Size.Y == GetSizeY();
}

bool FRoomGrid::IsAlongYDownWall(const FFurnitureRect& Position) const
{
	return Position.Position.Y == 0;
}

bool FRoomGrid::IsInAnyCorner(const FFurnitureRect& Position) const
{
	return IsAlongXDownWall(Position) && IsAlongYDownWall(Position)
		|| IsAlongXUpWall(Position) && IsAlongYDownWall(Position)
		|| IsAlongXUpWall(Position) && IsAlongYUpWall(Position)
		|| IsAlongXDownWall(Position) && IsAlongYUpWall(Position);
}

bool FRoomGrid::CheckLimits(const FFurnitureRect& Position) const
{
	if(Position.Position.X < 0 || Position.Position.Y < 0)
		return false;

	if(Position.WillRotationInvertSize())
	{
		if(Position.Position.X + Position.Size.Y > GetSizeX() || Position.Position.Y + Position.Size.X > GetSizeY())
			return false;
	}
	else if(Position.Position.X + Position.Size.X > GetSizeX() || Position.Position.Y + Position.Size.Y > GetSizeY())
		return false;

	return true;
}

void FRoomGrid::RotateData(const FFurnitureRect& InPosition, const FFurnitureConstraint& InConstraints, FFurnitureRect& RotatedPosition, FFurnitureConstraint& RotatedConstraints)
{
	RotatedPosition.Position = InPosition.Position;
	RotatedPosition.Rotation = EFurnitureRotation::ROT0;
	
	switch(InPosition.Rotation)
	{
		case EFurnitureRotation::ROT0:
			RotatedPosition.Size = InPosition.Size;
			RotatedConstraints = InConstraints;
			break;
		
		case EFurnitureRotation::ROT90:
			RotatedPosition.Size.X = InPosition.Size.Y;
			RotatedPosition.Size.Y = InPosition.Size.X;

			RotatedConstraints.Margin.XUp = InConstraints.Margin.YDown;
			RotatedConstraints.Margin.YUp = InConstraints.Margin.XUp;
			RotatedConstraints.Margin.XDown = InConstraints.Margin.YUp;
			RotatedConstraints.Margin.YDown = InConstraints.Margin.XDown;

			RotatedConstraints.WallAxeInteraction.XUp = InConstraints.WallAxeInteraction.YDown;
			RotatedConstraints.WallAxeInteraction.YUp = InConstraints.WallAxeInteraction.XUp;
			RotatedConstraints.WallAxeInteraction.XDown = InConstraints.WallAxeInteraction.YUp;
			RotatedConstraints.WallAxeInteraction.YDown = InConstraints.WallAxeInteraction.XDown;
			break;
		
		case EFurnitureRotation::ROT180:
			RotatedPosition.Size = InPosition.Size;
		
			RotatedConstraints.Margin.XUp = InConstraints.Margin.XDown;
			RotatedConstraints.Margin.YUp = InConstraints.Margin.YDown;
			RotatedConstraints.Margin.XDown = InConstraints.Margin.XUp;
			RotatedConstraints.Margin.YDown = InConstraints.Margin.YUp;

			RotatedConstraints.WallAxeInteraction.XUp = InConstraints.WallAxeInteraction.XDown;
			RotatedConstraints.WallAxeInteraction.YUp = InConstraints.WallAxeInteraction.YDown;
			RotatedConstraints.WallAxeInteraction.XDown = InConstraints.WallAxeInteraction.XUp;
			RotatedConstraints.WallAxeInteraction.YDown = InConstraints.WallAxeInteraction.YUp;
			break;
		
		case EFurnitureRotation::ROT270:
			RotatedPosition.Size.X = InPosition.Size.Y;
			RotatedPosition.Size.Y = InPosition.Size.X;

			RotatedConstraints.Margin.XUp = InConstraints.Margin.YUp;
			RotatedConstraints.Margin.YUp = InConstraints.Margin.XDown;
			RotatedConstraints.Margin.XDown = InConstraints.Margin.YDown;
			RotatedConstraints.Margin.YDown = InConstraints.Margin.XUp;

			RotatedConstraints.WallAxeInteraction.XUp = InConstraints.WallAxeInteraction.YUp;
			RotatedConstraints.WallAxeInteraction.YUp = InConstraints.WallAxeInteraction.XDown;
			RotatedConstraints.WallAxeInteraction.XDown = InConstraints.WallAxeInteraction.YDown;
			RotatedConstraints.WallAxeInteraction.YDown = InConstraints.WallAxeInteraction.XUp;
			break;
	}
}

void FRoomGrid::RotateDependencyData(const FFurnitureRect& InParentPosition, const FFurnitureDependency& InDependency, FFurnitureRect& RotatedParentPosition, FFurnitureDependency& RotatedDependency)
{
	RotatedParentPosition.Position = InParentPosition.Position;
	RotatedParentPosition.Rotation = EFurnitureRotation::ROT0;
	RotatedDependency = InDependency;
	
	switch (InParentPosition.Rotation)
	{
		case EFurnitureRotation::ROT0:
			RotatedParentPosition.Position = InParentPosition.Position;
			break;
			
		case EFurnitureRotation::ROT90:
			RotatedParentPosition.Size.X = InParentPosition.Size.Y;
			RotatedParentPosition.Size.Y = InParentPosition.Size.X;

			switch (InDependency.Axe)
			{
				case EGenerationAxe::X_UP: RotatedDependency.Axe = EGenerationAxe::Y_UP; break;
				case EGenerationAxe::X_DOWN: RotatedDependency.Axe = EGenerationAxe::Y_DOWN; break;
				case EGenerationAxe::Y_UP: RotatedDependency.Axe = EGenerationAxe::X_DOWN; break;
				case EGenerationAxe::Y_DOWN: RotatedDependency.Axe = EGenerationAxe::X_UP; break;
				default : assert(false);
			}
			break;
			
		case EFurnitureRotation::ROT180:
			RotatedParentPosition.Position = InParentPosition.Position;
			
			switch (InDependency.Axe)
			{
				case EGenerationAxe::X_UP: RotatedDependency.Axe = EGenerationAxe::X_DOWN; break;
				case EGenerationAxe::X_DOWN: RotatedDependency.Axe = EGenerationAxe::X_UP; break;
				case EGenerationAxe::Y_UP: RotatedDependency.Axe = EGenerationAxe::Y_DOWN; break;
				case EGenerationAxe::Y_DOWN: RotatedDependency.Axe = EGenerationAxe::Y_UP; break;
				default : assert(false);
			}
			break;
		
		case EFurnitureRotation::ROT270:
			RotatedParentPosition.Size.X = InParentPosition.Size.Y;
			RotatedParentPosition.Size.Y = InParentPosition.Size.X;

			switch (InDependency.Axe)
			{
				case EGenerationAxe::X_UP: RotatedDependency.Axe = EGenerationAxe::Y_DOWN; break;
				case EGenerationAxe::X_DOWN: RotatedDependency.Axe = EGenerationAxe::Y_UP; break;
				case EGenerationAxe::Y_UP: RotatedDependency.Axe = EGenerationAxe::X_UP; break;
				case EGenerationAxe::Y_DOWN: RotatedDependency.Axe = EGenerationAxe::X_DOWN; break;
				default: assert(false);
			}
			break;
	}
}

void FRoomGrid::GenerateMarginRect(const FFurnitureRect& RotatedPosition, const FMarginStruct& RotatedMargin, FFurnitureRect& MarginRect)
{
	MarginRect = RotatedPosition;
	MarginRect.Position.X -= RotatedMargin.XDown;
	MarginRect.Position.Y -= RotatedMargin.YDown;
	MarginRect.Size.X += RotatedMargin.XDown + RotatedMargin.XUp;
	MarginRect.Size.Y += RotatedMargin.YDown + RotatedMargin.YUp;
}

bool FRoomGrid::CheckFurnitureRect(const FFurnitureRect& RotatedPosition, const FWallInteractionStruct& RotatedInteraction, uint8 DependencyMarker) const
{
	//I : Check Rect
	if(RotatedPosition.Rotation != EFurnitureRotation::ROT0)
		return false;

	if(!CheckLimits(RotatedPosition))
		return false;
	
	for (int i = 0; i < RotatedPosition.Size.X; ++i)
		for (int j = 0; j < RotatedPosition.Size.Y; ++j)
		{
			const FRoomCell &Cell = Grid[RotatedPosition.Position.X + i][RotatedPosition.Position.Y + j];
			if(Cell.Type == ERoomCellType::OBJECT)
				return false;
			
			if(Cell.Type == ERoomCellType::MARGIN)
				if(!DependencyMarker || Cell.FurnitureDependencyMarker ^ FRoomCell::IndexToMarker(DependencyMarker))
					return false;
		}

	//II : Check walls
	EGenerationAxe Buffer;
	switch (RotatedInteraction.WallGlobalInteraction)
	{
		case EWallGlobalInteraction::CORNER:
			if(!RotatedInteraction.GetSecondAttractedWall(Buffer) || !IsAlongAxeWall(RotatedPosition, Buffer))
				return false;
		
		case EWallGlobalInteraction::WALL:
			if(!RotatedInteraction.GetFirstAttractedWall(Buffer) || !IsAlongAxeWall(RotatedPosition, Buffer))
				return false;
		
		case EWallGlobalInteraction::DEFAULT:
			if(RotatedInteraction.XDown == EWallAxeInteraction::REJECT && IsAlongXDownWall(RotatedPosition))
				return false;
			if(RotatedInteraction.XUp == EWallAxeInteraction::REJECT && IsAlongXUpWall(RotatedPosition))
				return false;
			if(RotatedInteraction.YDown == EWallAxeInteraction::REJECT && IsAlongYDownWall(RotatedPosition))
				return false;
			if(RotatedInteraction.YUp == EWallAxeInteraction::REJECT && IsAlongYUpWall(RotatedPosition))
				return false;
			break;
		
		case EWallGlobalInteraction::FAR:
			if(!IsAlongXDownWall(RotatedPosition) || !IsAlongXUpWall(RotatedPosition) || !IsAlongYDownWall(RotatedPosition) || !IsAlongYUpWall(RotatedPosition))
				return false;
			break;
		
		default: return false;
	}

	return true;
}

bool FRoomGrid::CheckMarginRect(const FFurnitureRect& RotatedPosition, const FMarginStruct& RotatedMargin, uint8 DependencyMarker) const
{
	if(RotatedPosition.Rotation != EFurnitureRotation::ROT0)
		return false;

	if(RotatedMargin.XUp < 0 || RotatedMargin.XDown < 0 || RotatedMargin.YUp < 0 || RotatedMargin.YDown < 0)
		return false;

	FFurnitureRect MarginRect;
	GenerateMarginRect(RotatedPosition, RotatedMargin, MarginRect);

	if(!CheckLimits(MarginRect))
		return false;

	for (int i = 0; i < MarginRect.Size.X; ++i)
		for (int j = 0; j < MarginRect.Size.Y; ++j)
			if(Grid[MarginRect.Position.X + i][MarginRect.Position.Y + j].Type == ERoomCellType::OBJECT)
				return false;

	return true;
}

bool FRoomGrid::CheckDependencyConstraints(const FFurnitureRect& RotatedPosition, const FFurnitureRect& RotatedParentPosition, const FFurnitureDependency& RotatedDependency, uint8 DependencyMarker) const
{
	if(RotatedPosition.Rotation != EFurnitureRotation::ROT0)
		return false;

	int DVirtualLeft;
	int DVirtualRight;
	int Distance;
	switch(RotatedDependency.Axe)
	{
		case EGenerationAxe::X_UP:
			DVirtualLeft = RotatedPosition.Position.Y - RotatedParentPosition.Position.Y;
			DVirtualRight = RotatedParentPosition.Position.Y + RotatedParentPosition.Size.Y - (RotatedPosition.Position.Y + RotatedPosition.Size.Y);
			Distance = RotatedPosition.Position.X - (RotatedParentPosition.Position.X + RotatedParentPosition.Size.X);
			break;
		
		case EGenerationAxe::X_DOWN:
			DVirtualLeft = RotatedParentPosition.Position.Y + RotatedParentPosition.Size.Y - (RotatedPosition.Position.Y + RotatedPosition.Size.Y);
			DVirtualRight = RotatedPosition.Position.Y - RotatedParentPosition.Position.Y;
			Distance = RotatedParentPosition.Position.X - (RotatedPosition.Position.X + RotatedPosition.Size.X);
			break;
		
		case EGenerationAxe::Y_UP:
			DVirtualLeft = RotatedParentPosition.Position.X + RotatedParentPosition.Size.X - (RotatedPosition.Position.X + RotatedPosition.Size.X);
			DVirtualRight = RotatedPosition.Position.X - RotatedParentPosition.Position.X;
			Distance = RotatedPosition.Position.X - (RotatedParentPosition.Position.X + RotatedParentPosition.Size.X);
			break;
		case EGenerationAxe::Y_DOWN:
			DVirtualLeft = RotatedPosition.Position.X - RotatedParentPosition.Position.X;
			DVirtualRight = RotatedParentPosition.Position.X + RotatedParentPosition.Size.X - (RotatedPosition.Position.X + RotatedPosition.Size.X);
			Distance = RotatedParentPosition.Position.Y - (RotatedPosition.Position.Y + RotatedPosition.Size.Y);
			break;
		default: return false;
	}

	if(Distance < 0 || Distance > RotatedDependency.Distance)
		return false;

	switch (RotatedDependency.Position) {
		case EDependencyPlace::EDGE_L:
			if(DVirtualLeft != 0)
				return false;
			break;
		
		case EDependencyPlace::EDGE_R:
			if(DVirtualRight != 0)
				return false;
			break;
		
		case EDependencyPlace::DEFAULT:
			if(DVirtualRight <= 0 || DVirtualLeft <= 0)
				return false;
			break;
		default: return false;
	}

	return true;
}

void FRoomGrid::MarkRect(const FFurnitureRect& RotatedPosition, ERoomCellType CellType, uint8 DependencyMarker)
{
	for (int i = 0; i < RotatedPosition.Size.X; ++i)
		for (int j = 0; j < RotatedPosition.Size.Y; ++j)
		{
			Grid[RotatedPosition.Position.X + i][RotatedPosition.Position.Y + j].Type = CellType;
			Grid[RotatedPosition.Position.X + i][RotatedPosition.Position.Y + j].FurnitureDependencyMarker |= FRoomCell::IndexToMarker(DependencyMarker);
		}
}

FDoorBlock::FDoorBlock(const FRoomBlock* _MainParent, const FRoomBlock* _SecondParent, 	const EGenerationAxe _OpeningSide, UFurnitureMeshAsset* _DoorAsset)
	: ParentMain(_MainParent), ParentSecond(_SecondParent), OpeningSide(_OpeningSide), DoorAsset(_DoorAsset) {}

void FDoorBlock::SaveLocalPosition(const FVectorGrid& LocalPosition, const FRoomBlock& Parent)
{
	if(!IsPositionValid() || !IsRoomParent(Parent))
		return;

	if(LocalPosition.X < 0 || LocalPosition.Y < 0)
		return;

	GlobalPosition = LocalPosition + Parent.GlobalPosition;
}

void FDoorBlock::MarkAsPlaced()
{
	if(!IsPositionValid())
		return;
	
	Placed = true;
}

bool FDoorBlock::IsPlaced() const
{
	return Placed && IsPositionValid();
}

bool FDoorBlock::IsPositionValid() const
{
	return GlobalPosition.X < 0 || GlobalPosition.Y < 0;
}

bool FDoorBlock::IsRoomParent(const FRoomBlock& Parent) const
{
	return &Parent == ParentMain || &Parent == ParentSecond;
}

FFurnitureRect FDoorBlock::GenerateLocalFurnitureRect(const FRoomBlock& Parent) const
{
	if(!IsPositionValid() || !IsRoomParent(Parent))
		return FFurnitureRect();

	EFurnitureRotation DoorRot;
	switch(OpeningSide)
	{
		case EGenerationAxe::X_UP: DoorRot = EFurnitureRotation::ROT0 ; break;
		case EGenerationAxe::X_DOWN: DoorRot = EFurnitureRotation::ROT180; break;
		case EGenerationAxe::Y_UP: DoorRot = EFurnitureRotation::ROT90; break;
		case EGenerationAxe::Y_DOWN: DoorRot = EFurnitureRotation::ROT270; break;
		default: DoorRot = EFurnitureRotation::ROT0; break;
	}

	return  FFurnitureRect(DoorRot, GlobalPosition - Parent.GlobalPosition, DoorAsset->GridSize);
}

FVectorGrid FDoorBlock::GenerateLocalMarginSize(const FRoomBlock& Parent, const FMarginStruct &DefaultDoorMargin) const
{
	if(!IsRoomParent(Parent))
		return FVectorGrid();

	int MarginDepth = 0;
	if(DoorAsset->bOverrideConstraint)
		MarginDepth = (&Parent == ParentMain) ? DoorAsset->ConstraintsOverride.Margin.XUp : DoorAsset->ConstraintsOverride.Margin.XDown;
	else
		MarginDepth = (&Parent == ParentMain) ? DefaultDoorMargin.XUp : DefaultDoorMargin.XDown;

	switch(OpeningSide)
	{
		case EGenerationAxe::X_UP:
		case EGenerationAxe::X_DOWN: return FVectorGrid(MarginDepth, DoorAsset->GridSize.Y);
		
		case EGenerationAxe::Y_UP:
		case EGenerationAxe::Y_DOWN: return FVectorGrid(DoorAsset->GridSize.Y, MarginDepth);
		default: return FVectorGrid();
	}
}

EGenerationAxe FDoorBlock::RoomWallAxe(const FRoomBlock& Parent) const
{
	if(!IsRoomParent(Parent))
		return EGenerationAxe::X_UP;

	if(&Parent == ParentMain)
	{
		switch (OpeningSide)
		{
            case EGenerationAxe::X_UP: return EGenerationAxe::X_DOWN;
            case EGenerationAxe::X_DOWN: return EGenerationAxe::X_UP;
            case EGenerationAxe::Y_UP: return EGenerationAxe::Y_DOWN;
            case EGenerationAxe::Y_DOWN: return EGenerationAxe::Y_UP;
            default: return EGenerationAxe::X_UP;
		}
	}
	
	return OpeningSide;
}

const FRoomBlock* FDoorBlock::ObtainOppositeParent(const FRoomBlock& Parent) const
{
	if(&Parent == ParentMain)
		return ParentSecond;

	return ParentMain;
}

UFurnitureMeshAsset* FDoorBlock::GetMeshAsset() const
{
	return DoorAsset;
}

FRoomBlock::FRoomBlock(const FVectorGrid& _Size, const FVectorGrid& _GlobalPosition, int _Level)
	: FBasicBlock(_Size, _GlobalPosition, _Level) {}

FVector FRoomBlock::GenerateRoomOffset(const FBuildingConstraint& BuildData) const
{
	return FVector(
		GlobalPosition.X * BuildData.GridSnapLength,
		GlobalPosition.Y * BuildData.GridSnapLength,
		Level * (BuildData.FloorHeight + BuildData.FloorWidth)
	);
}

bool FRoomBlock::operator<(const FRoomBlock& B) const
{
	return FMath::Min(Size.X, Size.Y) < FMath::Min(B.Size.X, B.Size.Y);
}

FDependencyBuffer::FDependencyBuffer(const TArray<FFurnitureDependency>& _Dependencies,	const FFurnitureRect& _ParentPosition)
	: Dependencies(_Dependencies), ParentPosition(_ParentPosition) {}

FLevelDivisionData::FLevelDivisionData(int _LevelTotalArea, int _HallArea) : HallTotalArea(_HallArea), LevelTotalArea(_LevelTotalArea) {}

float FLevelDivisionData::GetFutureHallRatio(int HallArea) const
{
	return (static_cast<float>(HallTotalArea) + static_cast<float>(HallArea)) / static_cast<float>(LevelTotalArea);
}

FHallBlock::FHallBlock(const FVectorGrid& _Size, const FVectorGrid& _GlobalPosition, int _Level)
	: FBasicBlock(_Size, _GlobalPosition, _Level) {}

FUnknownBlock::FUnknownBlock(const FVectorGrid& _Size, const FVectorGrid& _GlobalPosition, int _Level, bool _AlongX)
	: FBasicBlock(_Size, _GlobalPosition, _Level), DivideAlongX(_AlongX) {}

FUnknownBlock::DivideMethod FUnknownBlock::ShouldDivide(const FRoomsDivisionConstraints& DivisionCst, FLevelDivisionData& DivisionData) const
{
	//Basic checks
	assert(GlobalPosition.X >= 0 && GlobalPosition.Y >= 0);
	assert(Size.X > DivisionCst.ABSMinimalSide && Size.Y > DivisionCst.ABSMinimalSide);

	if(DivideDecision != DivideMethod::ERROR)
		return DivideDecision;

	//Divide along the correct side
	DivideDecision = DivideMethod::NO_DIVIDE;
	if(DivideAlongX)
	{
		const bool ShouldStopSplit = Size.X < DivisionCst.StopSplitSide
			|| DivisionCst.ABSMinimalSide * 2  > Size.X - DivisionCst.HallWidth
			|| NoMoreSplit
			|| DivisionData.GetFutureHallRatio(Size.Y * DivisionCst.HallWidth) > DivisionCst.MaxHallRatio;
		
		if(DivisionCst.ABSMinimalSide * 2 > Size.X)
			DivideDecision = DivideMethod::NO_DIVIDE;
		else if(Size.X >= DivisionCst.SufficientSide)
		{
			if(ShouldStopSplit)
				DivideDecision =  DivideMethod::DIVISION;
			else
				DivideDecision =  DivideMethod::SPLIT;
		}		
		else if(FMath::RandRange(0.f, 1.f) <= DivisionCst.OverDivideProba)
		{
			if(ShouldStopSplit)
				DivideDecision =  DivideMethod::DIVISION;
			else
				DivideDecision =  DivideMethod::SPLIT;
		}

		if(DivideDecision == DivideMethod::SPLIT) DivisionData.HallTotalArea += Size.Y * DivisionCst.HallWidth;
	}
	else
	{
		const bool ShouldStopSplit = Size.Y < DivisionCst.StopSplitSide
			|| DivisionCst.ABSMinimalSide * 2  > Size.Y - DivisionCst.HallWidth
			|| NoMoreSplit
			|| DivisionData.GetFutureHallRatio(Size.X * DivisionCst.HallWidth) > DivisionCst.MaxHallRatio;
		
		if(DivisionCst.ABSMinimalSide * 2 > Size.Y)
			DivideDecision = DivideMethod::NO_DIVIDE;
		else if(Size.Y >= DivisionCst.SufficientSide)
		{
			if(ShouldStopSplit)
				DivideDecision = DivideMethod::DIVISION;
			else
				DivideDecision = DivideMethod::SPLIT;
		}
		else if(FMath::RandRange(0.f, 1.f) <= DivisionCst.OverDivideProba)
		{
			if(ShouldStopSplit)
				DivideDecision = DivideMethod::DIVISION;
			else
				DivideDecision = DivideMethod::SPLIT;
		}
		
		if(DivideDecision == DivideMethod::SPLIT) DivisionData.HallTotalArea += Size.X * DivisionCst.HallWidth;
	}
	return DivideDecision;
}

bool FUnknownBlock::BlockSplit(const FRoomsDivisionConstraints& DivisionCst, FUnknownBlock &FirstResultedBlock,  FUnknownBlock& SecondResultedBlock, FHallBlock& ResultHall)
{
	//Basic checks
	if(DivideDecision != DivideMethod::SPLIT)
		return false;

	//Make split
	if(DivideAlongX)
	{
		const int HallAxis = FMath::RandRange(DivisionCst.ABSMinimalSide, Size.X - DivisionCst.ABSMinimalSide - DivisionCst.HallWidth);

		//Setup the hall
		ResultHall.Size = FVectorGrid(DivisionCst.HallWidth, Size.Y);
		ResultHall.GlobalPosition = GlobalPosition + FVectorGrid(HallAxis, 0);

		//Setup first block
		FirstResultedBlock.DivideAlongX = false;
		FirstResultedBlock.Size = FVectorGrid(HallAxis, Size.Y);
		FirstResultedBlock.GlobalPosition = GlobalPosition;

		//Setup second block (highest X location)
		SecondResultedBlock.DivideAlongX = false;
		SecondResultedBlock.Size = FVectorGrid(Size.X - (HallAxis + DivisionCst.HallWidth), Size.Y);
		SecondResultedBlock.GlobalPosition = GlobalPosition + FVectorGrid(HallAxis + DivisionCst.HallWidth, 0);
	}
	else
	{
		const int HallAxis = FMath::RandRange(DivisionCst.ABSMinimalSide, Size.Y - DivisionCst.ABSMinimalSide - DivisionCst.HallWidth);

		//Setup the hall
		ResultHall.Size = FVectorGrid(Size.X, DivisionCst.HallWidth);
		ResultHall.GlobalPosition = GlobalPosition + FVectorGrid(0, HallAxis);

		//Setup first block
		FirstResultedBlock.DivideAlongX = true;
		FirstResultedBlock.Size = FVectorGrid(Size.X, HallAxis);
		FirstResultedBlock.GlobalPosition = GlobalPosition;

		//Setup second block (highest Y location)
		SecondResultedBlock.DivideAlongX = true;
		SecondResultedBlock.Size = FVectorGrid(Size.X, Size.Y - (HallAxis + DivisionCst.HallWidth));
		SecondResultedBlock.GlobalPosition = GlobalPosition + FVectorGrid(0, HallAxis + DivisionCst.HallWidth);
	}

	//Redundant affectations
	SecondResultedBlock.DivideDecision	= FirstResultedBlock.DivideDecision	= DivideMethod::ERROR;
	SecondResultedBlock.NoMoreSplit		= FirstResultedBlock.NoMoreSplit	= false;
	SecondResultedBlock.Level			= FirstResultedBlock.Level			= ResultHall.Level = Level;

	//Link setup
	Child1 = &FirstResultedBlock;
	Child2 = &SecondResultedBlock;
	HallBlock = &ResultHall;
	FirstResultedBlock.Parent = SecondResultedBlock.Parent = this;
	
	return true;
}

bool FUnknownBlock::BlockDivision(const FRoomsDivisionConstraints& DivisionCst, FUnknownBlock &FirstResultedBlock, FUnknownBlock& SecondResultedBlock)
{
	//Basic checks
	if(DivideDecision != DivideMethod::DIVISION)
		return false;

	//Make split
	if(DivideAlongX)
	{
		const int WallAxis = FMath::RandRange(DivisionCst.ABSMinimalSide, Size.X - DivisionCst.ABSMinimalSide);

		//Setup first block
		FirstResultedBlock.DivideAlongX	= false;
		FirstResultedBlock.Size	= FVectorGrid(WallAxis, Size.Y);
		FirstResultedBlock.GlobalPosition = GlobalPosition;

		//Setup second block (highest X location)
		SecondResultedBlock.DivideAlongX = false;
		SecondResultedBlock.Size = FVectorGrid(Size.X - WallAxis, Size.Y);
		SecondResultedBlock.GlobalPosition = GlobalPosition + FVectorGrid(WallAxis, 0);
	}
	else
	{
		const int WallAxis = FMath::RandRange(DivisionCst.ABSMinimalSide, Size.Y - DivisionCst.ABSMinimalSide);

		//Setup first block
		FirstResultedBlock.DivideAlongX = true;
		FirstResultedBlock.Size = FVectorGrid(Size.X, WallAxis);
		FirstResultedBlock.GlobalPosition = GlobalPosition;

		//Setup the returned block (highest Y location)
		SecondResultedBlock.DivideAlongX = true;
		SecondResultedBlock.Size = FVectorGrid(Size.X, Size.Y - WallAxis);
		SecondResultedBlock.GlobalPosition = GlobalPosition + FVectorGrid(0, WallAxis);
	}

	//Redundant affectations
	SecondResultedBlock.DivideDecision	= FirstResultedBlock.DivideDecision = DivideMethod::ERROR;
	SecondResultedBlock.NoMoreSplit		= FirstResultedBlock.NoMoreSplit	= true;
	SecondResultedBlock.Level			= FirstResultedBlock.Level			= Level;

	//Link setup
	Child1 = &FirstResultedBlock;
	Child2 = &SecondResultedBlock;
	FirstResultedBlock.Parent = SecondResultedBlock.Parent = this;

	return true;
}

void FUnknownBlock::TransformToRoom(FRoomBlock& CreatedRoom)
{
	CreatedRoom.RoomType = "";
	CreatedRoom.Level = Level;
	CreatedRoom.Size = Size;
	CreatedRoom.GlobalPosition = GlobalPosition;
	Room = &CreatedRoom;
}

void FUnknownBlock::ComputeRealSizeRecursive(const FBuildingConstraint &BuildingCst, const FRoomsDivisionConstraints &RoomDivisionCst)
{
	check(DivideDecision != DivideMethod::ERROR);
	if(Child1 == nullptr && Child2 == nullptr || DivideDecision == DivideMethod::NO_DIVIDE) //End case
	{
		RealSize.X = Size.X * BuildingCst.GridSnapLength + 2 * BuildingCst.WallWidth;
		RealSize.Y = Size.Y * BuildingCst.GridSnapLength + 2 * BuildingCst.WallWidth;
		Room->RealSize = RealSize;
		return;
	}

	Child1->ComputeRealSizeRecursive(BuildingCst, RoomDivisionCst);
	Child2->ComputeRealSizeRecursive(BuildingCst, RoomDivisionCst);

	//Basic checks
	auto CheckSize = [&] (const FVectorGrid &Vector, const FVector2D &Real) -> bool {
		return Vector.X * BuildingCst.GridSnapLength <= Real.X
			&& Vector.Y * BuildingCst.GridSnapLength <= Real.Y;
	};
	check(CheckSize(Child1->Size, Child1->RealSize));
	check(CheckSize(Child2->Size, Child2->RealSize));

	if(DivideAlongX)
	{
		RealSize.X = Child1->RealSize.X + Child2->RealSize.X;
		RealSize.Y = FMath::Max(Child1->RealSize.Y, Child2->RealSize.Y);

		if(DivideDecision == DivideMethod::SPLIT) //Adds hall width
		{
			RealSize.X += RoomDivisionCst.HallWidth * BuildingCst.GridSnapLength;
			
			HallBlock->RealSize.X = RoomDivisionCst.HallWidth * BuildingCst.GridSnapLength;
			HallBlock->RealSize.Y = RealSize.Y - BuildingCst.WallWidth; //A hall must always pass through one wall (but not the other)
		}
		else if(DivideDecision == DivideMethod::DIVISION) //Removes collapsed walls (only one middle wall)
			RealSize.X -= BuildingCst.WallWidth;
	}
	else
	{
		RealSize.X = FMath::Max(Child1->RealSize.X, Child2->RealSize.X);
		RealSize.Y = Child1->RealSize.Y + Child2->RealSize.Y + RoomDivisionCst.HallWidth * BuildingCst.GridSnapLength;

		if(DivideDecision == DivideMethod::SPLIT) //Adds hall width
		{
			RealSize.Y += RoomDivisionCst.HallWidth * BuildingCst.GridSnapLength;
			
			HallBlock->RealSize.X = RealSize.X - BuildingCst.WallWidth; //A hall must always pass through one wall (but not the other)
			HallBlock->RealSize.Y = RoomDivisionCst.HallWidth * BuildingCst.GridSnapLength;
		}
		else if(DivideDecision == DivideMethod::DIVISION) //Removes collapsed walls (only one middle wall)
			RealSize.Y -= BuildingCst.WallWidth;
	}
}

void FUnknownBlock::ComputeRealOffsetRecursive(const FBuildingConstraint& BuildingCst, const FRoomsDivisionConstraints& RoomDivisionCst, const bool IsHighestAxe) const
{
	assert(DivideDecision != DivideMethod::ERROR);
	if(Child1 == nullptr && Child2 == nullptr || DivideDecision == DivideMethod::NO_DIVIDE) //End case
	{
		Room->RealOffset = RealOffset;
		return;
	}
	
	if(DivideAlongX)
	{
		Child1->RealOffset.X = RealOffset.X; //Lowest X position
		Child2->RealOffset.X = RealOffset.X + Child1->RealSize.X; //It includes 3 walls
		Child1->RealOffset.Y = RealOffset.Y + (RealSize.Y - Child1->RealSize.Y) / 2;
		Child2->RealOffset.Y = RealOffset.Y + (RealSize.Y - Child2->RealSize.Y) / 2;

		if(DivideDecision == DivideMethod::SPLIT) //Adds hall width
		{
			Child2->RealOffset.X  += RoomDivisionCst.HallWidth * BuildingCst.GridSnapLength;

			HallBlock->RealOffset.X = RealOffset.X + Child1->RealSize.X - BuildingCst.WallWidth;
			HallBlock->RealOffset.Y = RealOffset.Y - (IsHighestAxe ? BuildingCst.WallWidth : 0.f);
		}
		else if(DivideDecision == DivideMethod::DIVISION) //Removes collapsed walls (only one middle wall)
			Child2->RealOffset.X  -= BuildingCst.WallWidth;
	}
	else
	{
		Child1->RealOffset.Y = RealOffset.Y; //Lowest Y position
		Child2->RealOffset.Y = RealOffset.Y + Child1->RealSize.Y;//It includes 3 walls
		Child1->RealOffset.X = RealOffset.X + (RealSize.X - Child1->RealSize.X) / 2;
		Child2->RealOffset.X = RealOffset.X + (RealSize.X - Child2->RealSize.X) / 2;

		if(DivideDecision == DivideMethod::SPLIT) //Adds hall width
		{
			Child2->RealOffset.Y  += RoomDivisionCst.HallWidth * BuildingCst.GridSnapLength;

			HallBlock->RealOffset.X = RealOffset.X - (IsHighestAxe ? BuildingCst.WallWidth : 0.f);
			HallBlock->RealOffset.Y = RealOffset.Y + Child1->RealSize.Y - BuildingCst.WallWidth;
		}
		else if(DivideDecision == DivideMethod::DIVISION) //Removes collapsed walls (only one middle wall)
			Child2->RealOffset.Y  -= BuildingCst.WallWidth;
	}

	Child1->ComputeRealOffsetRecursive(BuildingCst, RoomDivisionCst, false);
	Child2->ComputeRealOffsetRecursive(BuildingCst, RoomDivisionCst, true);
}

FLevelOrganisation::FLevelOrganisation()
{
	InitialBlocks.Init(nullptr, BlockPositionsSize);
	InitialHalls.Init(nullptr, HallPositionsSize);
}

void FLevelOrganisation::SetUnknownBlock(EInitialBlockPositions Index, FUnknownBlock* Block)
{
	if(InitialBlocks.IsValidIndex(Index)) InitialBlocks[Index] = Block;
}

void FLevelOrganisation::SetHallBlock(EInitialHallPositions Index, FHallBlock* Hall)
{
	if(InitialHalls.IsValidIndex(Index)) InitialHalls[Index] = Hall;
}

FLevelOrganisation::EInitialBlockPositions FLevelOrganisation::GetUnknownBlockPosition(FUnknownBlock *Block) const
{
	int Index;
	if(InitialBlocks.Find(Block, Index)) return static_cast<EInitialBlockPositions>(Index);
	return BlockPositionsSize;
}

FLevelOrganisation::EInitialHallPositions FLevelOrganisation::GetHallBlockPosition(FHallBlock *Hall) const
{
	int Index;
	if(InitialHalls.Find(Hall, Index)) return static_cast<EInitialHallPositions>(Index);
	return HallPositionsSize;
}

void FLevelOrganisation::Empty(bool bDelete)
{
	for(uint8 i = 0; i < BlockPositionsSize; ++i)
	{
		if (bDelete) delete InitialBlocks[i];
		InitialBlocks[i] = nullptr;
	}
	
	for(uint8 i = 0; i < HallPositionsSize; ++i)
	{
		if (bDelete) delete InitialHalls[i];
		InitialHalls[i] = nullptr;
	}
}

int FLevelOrganisation::InitialHallArea() const
{
	return	(InitialHalls[LowCorridor]	? InitialHalls[LowCorridor]->Size.Area()	: 0)
		+	(InitialHalls[HighCorridor] ? InitialHalls[HighCorridor]->Size.Area()	: 0)
		+	(InitialHalls[LowMargin]	? InitialHalls[LowMargin]->Size.Area()		: 0)
		+	(InitialHalls[HighMargin]	? InitialHalls[HighMargin]->Size.Area()		: 0);
}

void FLevelOrganisation::ComputeBasicRealData(const FBuildingConstraint& BuildingCst, const FRoomsDivisionConstraints& RoomDivisionCst)
{
	check(InitialHalls[LowCorridor] && InitialBlocks[LowWing] || InitialHalls[HighCorridor] && InitialBlocks[HighWing]);
	check(InitialHalls[Stairs]);
	const bool AlongX = (InitialHalls[LowCorridor] && InitialHalls[LowCorridor]->GlobalPosition.Y == 0) || (InitialHalls[HighCorridor] && InitialHalls[HighCorridor]->GlobalPosition.Y == 0);
	RealOffset = FVector2D::UnitVector * BuildingCst.WallWidth;

	for(const auto Block : InitialBlocks)
		if (Block != nullptr) Block->ComputeRealSizeRecursive(BuildingCst, RoomDivisionCst);

	//Alternate value for Wing when they aren't not present take in account the fact that the corresponding corridor doesn't exist neither in this case.
	if(AlongX)
	{		
		//This' size calculus
		RealSize.X =
			  (InitialBlocks[LowWing]		? InitialBlocks[LowWing]->RealSize.X						: 0.f)
			+ (InitialBlocks[HighWing]		? InitialBlocks[HighWing]->RealSize.X						: 0.f)
			+ (InitialHalls[LowCorridor]	? RoomDivisionCst.HallWidth * BuildingCst.GridSnapLength	: 0.f)
			+ (InitialHalls[HighCorridor]	? RoomDivisionCst.HallWidth * BuildingCst.GridSnapLength	: 0.f)
			+ FMath::Max(
				InitialBlocks[LowApartment]	? InitialBlocks[LowApartment]->RealSize.X	: 0.f,
				InitialBlocks[HighApartment]	? InitialBlocks[HighApartment]->RealSize.X	: 0.f
			);

		RealSize.Y = FMath::Max3(
				InitialBlocks[LowWing]	? InitialBlocks[LowWing]->RealSize.Y	: 0.f,
				InitialBlocks[HighWing]	? InitialBlocks[HighWing]->RealSize.Y	: 0.f,
				InitialHalls[Stairs]->Size.Y * BuildingCst.GridSnapLength
					+ (InitialBlocks[LowApartment]	? InitialBlocks[LowApartment]->RealSize.Y	: 0.f)
					+ (InitialBlocks[HighApartment] ? InitialBlocks[HighApartment]->RealSize.Y	: 0.f)
			);

		//Corridors size
		if(InitialHalls[HighCorridor])
		{
			InitialHalls[HighCorridor]->RealSize.X = RoomDivisionCst.HallWidth * BuildingCst.GridSnapLength;
			InitialHalls[HighCorridor]->RealSize.Y = RealSize.Y - 2 * BuildingCst.WallWidth;
		}
		if(InitialHalls[LowCorridor])
		{
			InitialHalls[LowCorridor]->RealSize.X = RoomDivisionCst.HallWidth * BuildingCst.GridSnapLength;
			InitialHalls[LowCorridor]->RealSize.Y = RealSize.Y - 2 * BuildingCst.WallWidth;
		}

		//Margin and Stairs size
		InitialHalls[Stairs]->RealSize = InitialHalls[Stairs]->Size * BuildingCst.GridSnapLength;		
		if(InitialHalls[LowMargin]) InitialHalls[LowMargin]->RealSize = InitialHalls[LowMargin]->Size * BuildingCst.GridSnapLength;
		if(InitialHalls[HighMargin]) InitialHalls[HighMargin]->RealSize = InitialHalls[HighMargin]->Size * BuildingCst.GridSnapLength;
		
		const int MarginNumber = (InitialHalls[HighMargin] ? 1 : 0) + (InitialHalls[LowMargin] ? 1 : 0);
		const float AddMarginX = RealSize.X
			- (InitialBlocks[LowWing]		? InitialBlocks[LowWing]->RealSize.X		: BuildingCst.WallWidth)
			- (InitialBlocks[HighWing]		? InitialBlocks[HighWing]->RealSize.X		: BuildingCst.WallWidth)
			- (InitialHalls[LowCorridor]	? InitialHalls[LowCorridor]->RealSize.X		: 0.f)
			- (InitialHalls[HighCorridor]	? InitialHalls[HighCorridor]->RealSize.X	: 0.f)
			- (InitialHalls[LowMargin]		? InitialHalls[LowMargin]->RealSize.X		: 0.f)
			- (InitialHalls[HighMargin]		? InitialHalls[HighMargin]->RealSize.X		: 0.f)
			- InitialHalls[Stairs]->RealSize.X;
		
		const float AddMarginY = RealSize.Y
			- (InitialBlocks[LowApartment]	? InitialBlocks[LowApartment]->RealSize.Y	: BuildingCst.WallWidth)
			- (InitialBlocks[HighApartment] ? InitialBlocks[HighApartment]->RealSize.Y	: BuildingCst.WallWidth)
			- InitialHalls[Stairs]->RealSize.Y;

		InitialHalls[Stairs]->RealSize.Y += AddMarginY;
		if(MarginNumber == 0) InitialHalls[Stairs]->RealSize.X += AddMarginX; //Actually increase size instead of increasing offset
		
		if(InitialHalls[LowMargin])
		{
			InitialHalls[LowMargin]->RealSize.Y += AddMarginY;
			InitialHalls[LowMargin]->RealSize.X += AddMarginX / MarginNumber;
		}
		if(InitialHalls[HighMargin])
		{
			InitialHalls[HighMargin]->RealSize.Y += AddMarginY;
			InitialHalls[HighMargin]->RealSize.X += AddMarginX / MarginNumber;
		}

		//Stairs offset (actually not exactly centered if the stair hall need to be extended)
		InitialHalls[Stairs]->RealOffset.X =
			  (InitialBlocks[LowWing]		? InitialBlocks[LowWing]->RealSize.X	: BuildingCst.WallWidth)
			+ (InitialHalls[LowCorridor]	? InitialHalls[LowCorridor]->RealSize.X	: 0.f)
			+ (InitialHalls[LowMargin]		? InitialHalls[LowMargin]->RealSize.X	: 0.f); //Includes the additional wall

		InitialHalls[Stairs]->RealOffset.Y = InitialBlocks[LowApartment] ? InitialBlocks[LowApartment]->RealSize.Y : BuildingCst.WallWidth;
	}
	else
	{
		//This' size calculus
		RealSize.Y = (InitialBlocks[LowWing] ? InitialBlocks[LowWing]->RealSize.Y : 0.f)
			+ (InitialBlocks[HighWing] ? InitialBlocks[HighWing]->RealSize.Y : 0.f)
			+ (InitialHalls[LowCorridor] ? RoomDivisionCst.HallWidth * BuildingCst.GridSnapLength : 0.f)
			+ (InitialHalls[HighCorridor] ? RoomDivisionCst.HallWidth * BuildingCst.GridSnapLength : 0.f)
			+ FMath::Max(
				InitialBlocks[LowApartment] ? InitialBlocks[LowApartment]->RealSize.Y : 0.f,
				InitialBlocks[HighApartment] ? InitialBlocks[HighApartment]->RealSize.Y : 0.f
			);

		RealSize.X = FMath::Max3(
				InitialBlocks[LowWing] ? InitialBlocks[LowWing]->RealSize.X : 0.f,
				InitialBlocks[HighWing] ? InitialBlocks[HighWing]->RealSize.X : 0.f,
				InitialHalls[Stairs]->RealSize.X
					+ (InitialBlocks[LowApartment] ? InitialBlocks[LowApartment]->RealSize.X : 0.f)
					+ (InitialBlocks[HighApartment] ? InitialBlocks[HighApartment]->RealSize.X : 0.f)
			);

		//Corridors size
		if(InitialHalls[HighCorridor])
		{
			InitialHalls[HighCorridor]->RealSize.Y = RoomDivisionCst.HallWidth * BuildingCst.GridSnapLength;
			InitialHalls[HighCorridor]->RealSize.X = RealSize.Y - 2 * BuildingCst.WallWidth;
		}
		if(InitialHalls[LowCorridor])
		{
			InitialHalls[LowCorridor]->RealSize.Y = RoomDivisionCst.HallWidth * BuildingCst.GridSnapLength;
			InitialHalls[LowCorridor]->RealSize.X = RealSize.Y - 2 * BuildingCst.WallWidth;
		}

		//Margin and Stairs size
		InitialHalls[Stairs]->RealSize = InitialHalls[Stairs]->Size * BuildingCst.GridSnapLength;		
		if(InitialHalls[LowMargin]) InitialHalls[LowMargin]->RealSize = InitialHalls[LowMargin]->Size * BuildingCst.GridSnapLength;
		if(InitialHalls[HighMargin]) InitialHalls[HighMargin]->RealSize = InitialHalls[HighMargin]->Size * BuildingCst.GridSnapLength;
		
		const int MarginNumber = (InitialHalls[HighMargin] ? 1 : 0) + (InitialHalls[LowMargin] ? 1 : 0);
		const float AddMarginY = RealSize.Y
			- (InitialBlocks[LowWing]		? InitialBlocks[LowWing]->RealSize.Y		: BuildingCst.WallWidth)
			- (InitialBlocks[HighWing]		? InitialBlocks[HighWing]->RealSize.Y		: BuildingCst.WallWidth)
			- (InitialHalls[LowCorridor]	? InitialHalls[LowCorridor]->RealSize.Y		: 0.f)
			- (InitialHalls[HighCorridor]	? InitialHalls[HighCorridor]->RealSize.Y	: 0.f)
			- (InitialHalls[LowMargin]		? InitialHalls[LowMargin]->RealSize.Y		: 0.f)
			- (InitialHalls[HighMargin]		? InitialHalls[HighMargin]->RealSize.Y		: 0.f)
			- InitialHalls[Stairs]->RealSize.Y;
		
		const float AddMarginX = RealSize.X
			- (InitialBlocks[LowApartment]	? InitialBlocks[LowApartment]->RealSize.X	: BuildingCst.WallWidth)
			- (InitialBlocks[HighApartment] ? InitialBlocks[HighApartment]->RealSize.X	: BuildingCst.WallWidth)
			- InitialHalls[Stairs]->RealSize.X;

		InitialHalls[Stairs]->RealSize.X += AddMarginX;
		if(MarginNumber == 0) InitialHalls[Stairs]->RealSize.Y += AddMarginY;
		
		if(InitialHalls[LowMargin])
		{
			InitialHalls[LowMargin]->RealSize.X += AddMarginX;
			InitialHalls[LowMargin]->RealSize.Y += AddMarginY / MarginNumber;
		}
		if(InitialHalls[HighMargin])
		{
			InitialHalls[HighMargin]->RealSize.X += AddMarginX;
			InitialHalls[HighMargin]->RealSize.Y += AddMarginY / MarginNumber;
		}

		//Stairs offset (actually not exactly centered if the stair hall need to be extended)
		InitialHalls[Stairs]->RealOffset.X = InitialBlocks[LowApartment] ? InitialBlocks[LowApartment]->RealSize.X : BuildingCst.WallWidth;

		InitialHalls[Stairs]->RealOffset.Y = (InitialBlocks[LowWing] ? InitialBlocks[LowWing]->RealSize.Y : BuildingCst.WallWidth)
			+ (InitialHalls[LowCorridor] ? RoomDivisionCst.HallWidth * BuildingCst.GridSnapLength + BuildingCst.WallWidth: 0.f)
			+ (InitialHalls[LowMargin] ? InitialHalls[LowMargin]->RealSize.Y : 0.f);
	}
	
	//Don't launch recursive offset because before we should calculate all levels
}

void FLevelOrganisation::ComputeAllRealData(const FVector2D& LevelOffset, const FBuildingConstraint& BuildingCst, const FRoomsDivisionConstraints& RoomDivisionCst)
{
	check(InitialHalls[LowCorridor] && InitialBlocks[LowWing] || InitialHalls[HighCorridor] && InitialBlocks[HighWing]);
	check(InitialHalls[Stairs]);
	const bool AlongX = (InitialHalls[LowCorridor] && InitialHalls[LowCorridor]->GlobalPosition.Y == 0) || (InitialHalls[HighCorridor] && InitialHalls[HighCorridor]->GlobalPosition.Y == 0);
	RealOffset += LevelOffset;//WallWidth + Space for stairs
	
	if(AlongX)
	{
		//Corridors and wings offset (use this offset)
		if(InitialBlocks[LowWing])
		{
			InitialBlocks[LowWing]->RealOffset = RealOffset;
			InitialBlocks[LowWing]->RealOffset.Y += (RealSize.Y - InitialBlocks[LowWing]->RealSize.Y) / 2.f;
			InitialHalls[LowCorridor]->RealOffset.X = LevelOffset.X + InitialBlocks[LowWing]->Size.X;
			InitialHalls[LowCorridor]->RealOffset.Y = RealOffset.Y;
		}

		if(InitialBlocks[HighWing])
		{
			InitialBlocks[HighWing]->RealOffset = RealOffset;
			InitialBlocks[HighWing]->RealOffset.X += RealSize.X - InitialBlocks[HighWing]->RealSize.X;
			InitialBlocks[HighWing]->RealOffset.Y += (RealSize.Y - InitialBlocks[LowWing]->RealSize.Y) / 2.f;
			InitialHalls[HighCorridor]->RealOffset.X = InitialBlocks[HighWing]->RealOffset.X
				- BuildingCst.WallWidth
				- InitialHalls[HighCorridor]->RealSize.X;
			InitialHalls[HighCorridor]->RealOffset.Y = RealOffset.Y;
		}

		//Margin and stairs offset (use stairs offset)
		InitialHalls[Stairs]->RealOffset += LevelOffset;

		if(InitialHalls[LowMargin])
		{
			InitialHalls[LowMargin]->RealOffset = InitialHalls[Stairs]->RealOffset;
			InitialHalls[LowMargin]->RealOffset.X -= InitialBlocks[LowMargin]->RealSize.X;
		}

		if(InitialHalls[HighMargin])
		{
			InitialHalls[HighMargin]->RealOffset = InitialHalls[Stairs]->RealOffset;
			InitialHalls[HighMargin]->RealOffset.X += InitialBlocks[Stairs]->RealSize.X;
		}

		//Apartments offset
		const float CenterWidth = RealSize.X
			- (InitialBlocks[LowWing]		? InitialBlocks[LowWing]->RealSize.X		: 0.f)
			- (InitialBlocks[HighWing]		? InitialBlocks[HighWing]->RealSize.X		: 0.f)
			- (InitialHalls[LowCorridor]	? InitialHalls[LowCorridor]->RealSize.X		: 0.f)
			- (InitialHalls[HighCorridor]	? InitialHalls[HighCorridor]->RealSize.X	: 0.f);

		if(InitialBlocks[LowApartment])
		{
			InitialBlocks[LowApartment]->RealOffset.Y = RealOffset.Y;
			
			InitialBlocks[LowApartment]->RealOffset.X = RealOffset.X
				+ (InitialBlocks[LowWing]		? InitialBlocks[LowWing]->RealOffset.X		: 0.f)
				+ (InitialHalls[LowCorridor]	? InitialHalls[LowCorridor]->RealOffset.X	: 0.f);
			InitialBlocks[LowApartment]->RealOffset.X += (CenterWidth - InitialBlocks[LowApartment]->RealSize.X) / 2.f;
		}

		if(InitialBlocks[HighApartment])
		{
			InitialBlocks[HighApartment]->RealOffset.Y = RealOffset.Y + RealSize.Y
				- InitialBlocks[HighApartment]->RealSize.Y;
			
			InitialBlocks[HighApartment]->RealOffset.X = RealOffset.X
				+ (InitialBlocks[LowWing]		? InitialBlocks[LowWing]->RealOffset.X		: 0.f)
				+ (InitialHalls[LowCorridor]	? InitialHalls[LowCorridor]->RealOffset.X	: 0.f);
			InitialBlocks[HighApartment]->RealOffset.X += (CenterWidth - InitialBlocks[HighApartment]->RealSize.X) / 2.f;
		}
	}
	else
	{
		//Corridors and wings offset (use this offset)
		if(InitialBlocks[LowWing])
		{
			InitialBlocks[LowWing]->RealOffset = RealOffset;
			InitialBlocks[LowWing]->RealOffset.X += (RealSize.X - InitialBlocks[LowWing]->RealSize.X) / 2.f;
			
			InitialHalls[LowCorridor]->RealOffset.Y = LevelOffset.Y + InitialBlocks[LowWing]->Size.Y;
			InitialHalls[LowCorridor]->RealOffset.X = RealOffset.X;
		}

		if(InitialBlocks[HighWing])
		{
			InitialBlocks[HighWing]->RealOffset = RealOffset;
			InitialBlocks[HighWing]->RealOffset.Y += RealSize.Y - InitialBlocks[HighWing]->RealSize.Y;
			InitialBlocks[HighWing]->RealOffset.X += (RealSize.X - InitialBlocks[LowWing]->RealSize.X) / 2.f;
			
			InitialHalls[HighCorridor]->RealOffset.Y = InitialBlocks[HighWing]->RealOffset.Y
				- BuildingCst.WallWidth
				- InitialHalls[HighCorridor]->RealSize.Y;
			InitialHalls[HighCorridor]->RealOffset.X = RealOffset.X;
		}

		//Margin and stairs offset (use stairs offset)
		InitialHalls[Stairs]->RealOffset += LevelOffset;

		if(InitialHalls[LowMargin])
		{
			InitialHalls[LowMargin]->RealOffset = InitialHalls[Stairs]->RealOffset;
			InitialHalls[LowMargin]->RealOffset.Y -= InitialBlocks[LowMargin]->RealSize.Y;
		}

		if(InitialHalls[HighMargin])
		{
			InitialHalls[HighMargin]->RealOffset = InitialHalls[Stairs]->RealOffset;
			InitialHalls[HighMargin]->RealOffset.Y += InitialBlocks[Stairs]->RealSize.Y;
		}

		//Apartments offset
		const float CenterWidth = RealSize.Y
			- (InitialBlocks[LowWing]		? InitialBlocks[LowWing]->RealSize.Y		: 0.f)
			- (InitialBlocks[HighWing]		? InitialBlocks[HighWing]->RealSize.Y		: 0.f)
			- (InitialHalls[LowCorridor]	? InitialHalls[LowCorridor]->RealSize.Y		: 0.f)
			- (InitialHalls[HighCorridor]	? InitialHalls[HighCorridor]->RealSize.Y	: 0.f);

		if(InitialBlocks[LowApartment])
		{
			InitialBlocks[LowApartment]->RealOffset.X = RealOffset.X;
			
			InitialBlocks[LowApartment]->RealOffset.Y = RealOffset.Y
				+ (InitialBlocks[LowWing]		? InitialBlocks[LowWing]->RealOffset.Y		: 0.f)
				+ (InitialHalls[LowCorridor]	? InitialHalls[LowCorridor]->RealOffset.Y	: 0.f);
			InitialBlocks[LowApartment]->RealOffset.Y += (CenterWidth - InitialBlocks[LowApartment]->RealSize.Y) / 2.f;
		}

		if(InitialBlocks[HighApartment])
		{
			InitialBlocks[HighApartment]->RealOffset.X = RealOffset.X + RealSize.X
				- InitialBlocks[HighApartment]->RealSize.X;
			
			InitialBlocks[HighApartment]->RealOffset.Y = RealOffset.Y
				+ (InitialBlocks[LowWing]		? InitialBlocks[LowWing]->RealOffset.Y		: 0.f)
				+ (InitialHalls[LowCorridor]	? InitialHalls[LowCorridor]->RealOffset.Y	: 0.f);
			InitialBlocks[HighApartment]->RealOffset.Y += (CenterWidth - InitialBlocks[HighApartment]->RealSize.Y) / 2.f;
		}		
	}

	for(int i = 0; i < BlockPositionsSize; ++i)
	{
		const bool High = i == HighApartment || i == HighWing;
		if (InitialBlocks[i] != nullptr)
			InitialBlocks[i]->ComputeRealOffsetRecursive(BuildingCst, RoomDivisionCst, High);
	}
}

const FVector2D& FLevelOrganisation::GetStairsRealOffset() const
{
	check(InitialHalls[Stairs] != nullptr);
	return InitialHalls[Stairs]->GetRealOffset();
}

const TArray<FUnknownBlock*>& FLevelOrganisation::GetBlockList() const
{
	return InitialBlocks;
}

const TArray<FHallBlock*>& FLevelOrganisation::GetHallList() const
{
	return InitialHalls;
}

//Copyright

#include "HGInternalStruct.h"

#include "HomeGenerator.h"

#define CONSTRUCT_GRID(X,Y) check(X > 0 && Y > 0) Grid.SetNum(X); for (int i = 0; i < X; ++i) { Grid[i].SetNum(X); }

FRoomCell::FRoomCell() : Type(ERoomCellType::EMPTY), FurnitureDependencyMarker(0) {}

uint32 FRoomCell::IndexToMarker(uint8 FurnitureIndex)
{
	if(FurnitureIndex > 32 || FurnitureIndex == 0)
		return 0;

	int Buffer = 1;
	for (int i = 0; i < FurnitureIndex - 1; ++i)
		Buffer *= 2;

	return Buffer;
}

FFurnitureRect::FFurnitureRect() : Rotation(EFurnitureRotation::ROT0) {}

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
		case EDependencyPlace::EDGEL:
			if(DVirtualLeft != 0)
				return false;
			break;
		
		case EDependencyPlace::EDGER:
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
	: ParentMain(_MainParent), ParentSecond(_SecondParent), OpeningSide(_OpeningSide), Placed(false), GlobalPosition(-1, -1), DoorAsset(_DoorAsset) {}

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

FRoomBlock::FRoomBlock() : Level(0) {}

FRoomBlock::FRoomBlock(const FVectorGrid& _Size, const FVectorGrid& _GlobalPosition, int _Level) : Size(_Size), GlobalPosition(_GlobalPosition), Level(_Level) {}

FVector FRoomBlock::GenerateRoomOffset(const FBuildingConstraint& BuildData) const
{
	return FVector(
		GlobalPosition.X * BuildData.GridSnapLength,
		GlobalPosition.Y * BuildData.GridSnapLength,
		Level * (BuildData.FloorHeight + BuildData.FloorWidth)
	);
}

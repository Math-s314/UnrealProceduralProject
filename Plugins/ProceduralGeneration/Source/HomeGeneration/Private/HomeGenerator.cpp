// Fill out your copyright notice in the Description page of Project Settings.


#include "HomeGenerator.h"
#include "Engine/StaticMeshActor.h"
#include <assert.h>

FRoomsDivisionConstraints::FRoomsDivisionConstraints()
	: HallWidth(0), MaxHallRatio(0.4f), MinSideCoef(2.f), StopSplitCoef(2.f), SufficientChunkCoef(2.f), OverDivideProba(0.5f),
	  BasicMinimalSide(0), ABSMinimalSide(0), StopSplitSide(0), SufficientSide(0) {}

int FRoom::GetMinimalSide() const
{
	return MinRoomSide;
}

// Sets default values
AHomeGenerator::AHomeGenerator()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AHomeGenerator::BeginPlay()
{
	Super::BeginPlay();
	
}

void AHomeGenerator::GenerateRangeArray(TArray<int32>& InArray, int32 Start, int32 Stop)
{
	InArray.Empty();

	for (int i  = Start; i < Stop; ++i)
		InArray.Add(i);
}

void AHomeGenerator::GenerateRangeArray(TArray<int32>& InArray, int32 Stop)
{
	GenerateRangeArray(InArray, 0, Stop);
}

void AHomeGenerator::DefineRooms()
{
	TArray<FHallBlock> InitialHalls;
	TArray<FUnknownBlock> InitialBlocks;
	StairsPositioning(InitialBlocks, InitialHalls);

	for (int i = 0; i < Levels; ++i)
	{
		HallBlocks.Push(TArray<FHallBlock>());
		RoomBlocks.Push(TArray<FRoomBlock>());
		DivideSurface(i, InitialBlocks, InitialHalls);
	}

	AllocateSurface();
	CompleteHallSurface();
}

void AHomeGenerator::StairsPositioning(TArray<FUnknownBlock> &InitialBlocks, TArray<FHallBlock> &InitialHalls)
{
	FRoomGrid LevelGrid(BuildingSize);
	
	//Possible positions
	TArray<int> PositionX;
	TArray<int> PositionY;
	TArray<EFurnitureRotation> Rotations = {EFurnitureRotation::ROT0, EFurnitureRotation::ROT90, EFurnitureRotation::ROT180, EFurnitureRotation::ROT270};
	GenerateRangeArray(PositionX, LevelGrid.GetSizeX());
	GenerateRangeArray(PositionY, LevelGrid.GetSizeY());

	//Define needed general element for positioning verification
	const FFurnitureConstraint &FinalConstraints = SelectedStair->bOverrideConstraint ? SelectedStair->ConstraintsOverride : Stairs.DefaultConstraints;
	const auto IsCenterAvailable = [&] (int GridSize, int Size) -> bool {
		return ( Size < RoomsDivisionConstraints.ABSMinimalSide + 2 * RoomsDivisionConstraints.HallWidth ) ?
			GridSize >= 3 * RoomsDivisionConstraints.ABSMinimalSide + 2 * RoomsDivisionConstraints.HallWidth
		:
			GridSize >= 2 * RoomsDivisionConstraints.ABSMinimalSide + Size;
	};
	const auto IsInCenter = [&] (int Coordinate, int GridSize, int Size) -> bool { return RoomsDivisionConstraints.ABSMinimalSide < Coordinate && Coordinate <= GridSize - (Size + RoomsDivisionConstraints.ABSMinimalSide); };
	const auto IsNHCenterAvailable = [&] (int GridSize, int Size) -> bool { return GridSize >= 2 * RoomsDivisionConstraints.ABSMinimalSide + Size; }; // No hall
	
	bool PositionFound = false;
	for(const int X : PositionX)
	{
		for(const int Y : PositionY)
		{
			for(const auto Rotation : Rotations)
			{
				
				FFurnitureRect FinalRect(Rotation, FVectorGrid(X, Y), SelectedStair->GridSize);

				//Reset if previous operation failed
				InitialBlocks.Empty();
				InitialHalls.Empty();

				//Checks if the rect respects stairs constraints
				bool IsStairPlaceable = true;
				{
					//Check lambdas
					const FVectorGrid RotatedSize = FinalRect.WillRotationInvertSize() ? FVectorGrid(FinalRect.Size.Y, FinalRect.Size.X) : FinalRect.Size;
					const auto IsInXCenter = [&] () -> bool { return IsInCenter(X, LevelGrid.GetSizeX(), RotatedSize.X); };
					const auto IsInYCenter = [&] () -> bool  { return IsInCenter(Y, LevelGrid.GetSizeY(), RotatedSize.Y); };
					const auto IsXCenterAvailable = [&] () -> bool { return IsCenterAvailable(LevelGrid.GetSizeX(), RotatedSize.X); };
					const auto IsYCenterAvailable = [&] () -> bool  { return IsCenterAvailable( LevelGrid.GetSizeY(), RotatedSize.Y); };
					const auto IsNHXCenterAvailable = [&] () -> bool { return IsNHCenterAvailable(LevelGrid.GetSizeX(), RotatedSize.X); };
					const auto IsNHYCenterAvailable = [&] () -> bool  { return IsNHCenterAvailable( LevelGrid.GetSizeY(), RotatedSize.Y); };

					//TODO : The code in the center case could replace all other cases (just if we check X > 0 for all X calculated value)
					//We could so split the part check if possible and spawn the hall/blocks
					//Case where it is in center of the room
					if(IsInXCenter() && IsInYCenter())
					{
						if(RotatedSize.X >= RotatedSize.Y)
						{
							if(!IsXCenterAvailable() || !IsNHYCenterAvailable())
								IsStairPlaceable = false;

							const int FHAxis = FMath::RandRange(RoomsDivisionConstraints.ABSMinimalSide, FMath::Min(FinalRect.Position.X, LevelGrid.GetSizeX() - 2 * (RoomsDivisionConstraints.HallWidth + RoomsDivisionConstraints.ABSMinimalSide)));
							const int SHAxis = FMath::RandRange(FMath::Max(FHAxis + RoomsDivisionConstraints.HallWidth + RoomsDivisionConstraints.ABSMinimalSide, FinalRect.Position.X + RotatedSize.X - RoomsDivisionConstraints.HallWidth), LevelGrid.GetSizeX() - (RoomsDivisionConstraints.HallWidth + RoomsDivisionConstraints.ABSMinimalSide));
							const int FHSpace = FinalRect.Position.X - (FHAxis + RoomsDivisionConstraints.HallWidth); //No need of min or max, because it is already implied by the def of the axis value
							const int SHSpace = SHAxis - (FinalRect.Position.X + RotatedSize.X);

							InitialHalls.Push(FHallBlock(
								FVectorGrid(RoomsDivisionConstraints.HallWidth, LevelGrid.GetSizeY()),
								FVectorGrid(FHAxis, 0),
								0
							));

							InitialHalls.Push(FHallBlock(
								FVectorGrid(RoomsDivisionConstraints.HallWidth, LevelGrid.GetSizeY()),
								FVectorGrid(SHAxis, 0),
								0
							));

							if(FHSpace > 0)
								InitialHalls.Push(FHallBlock(
									FVectorGrid(FHSpace, RotatedSize.Y),
									FVectorGrid(FHAxis + RoomsDivisionConstraints.HallWidth, FinalRect.Position.Y),
									0
								));

							if(SHSpace > 0)
								InitialHalls.Push(FHallBlock(
									FVectorGrid(SHSpace, RotatedSize.Y),
									FVectorGrid(FinalRect.Position.X + RotatedSize.X, FinalRect.Position.Y),
									0
								));

							InitialBlocks.Push(FUnknownBlock(
								FVectorGrid(FHAxis, LevelGrid.GetSizeY()),
								FVectorGrid(0, 0),
								0,
								false
							));

							InitialBlocks.Push(FUnknownBlock(
								FVectorGrid(LevelGrid.GetSizeX() - (SHAxis + RoomsDivisionConstraints.HallWidth), LevelGrid.GetSizeY()),
								FVectorGrid(SHAxis + RoomsDivisionConstraints.HallWidth, 0),
								0,
								false
							));

							InitialBlocks.Push(FUnknownBlock(
								FVectorGrid(SHAxis - (FHAxis + RoomsDivisionConstraints.HallWidth), FinalRect.Position.Y),
								FVectorGrid(SHAxis + RoomsDivisionConstraints.HallWidth, 0),
								0,
								false
							));

							InitialBlocks.Push(FUnknownBlock(
								FVectorGrid(SHAxis - (FHAxis + RoomsDivisionConstraints.HallWidth), LevelGrid.GetSizeY() - (FinalRect.Position.Y + RotatedSize.Y)),
								FVectorGrid(FHAxis + RoomsDivisionConstraints.HallWidth, FinalRect.Position.Y + RotatedSize.Y),
								0,
								false
							));
						}
						else
						{
							if(!IsYCenterAvailable() || !IsNHXCenterAvailable())
								IsStairPlaceable = false;

							const int FHAxis = FMath::RandRange(RoomsDivisionConstraints.ABSMinimalSide, FMath::Min(FinalRect.Position.Y, LevelGrid.GetSizeY() - 2 * (RoomsDivisionConstraints.HallWidth + RoomsDivisionConstraints.ABSMinimalSide)));
							const int SHAxis = FMath::RandRange(FMath::Max(FHAxis + RoomsDivisionConstraints.HallWidth + RoomsDivisionConstraints.ABSMinimalSide, FinalRect.Position.Y + RotatedSize.Y - RoomsDivisionConstraints.HallWidth), LevelGrid.GetSizeY() - (RoomsDivisionConstraints.HallWidth + RoomsDivisionConstraints.ABSMinimalSide));
							const int FHSpace = FinalRect.Position.Y - (FHAxis + RoomsDivisionConstraints.HallWidth); //No need of min or max, because it is already implied by the def of the axis value
							const int SHSpace = SHAxis - (FinalRect.Position.Y + RotatedSize.Y);

							InitialHalls.Push(FHallBlock(
								FVectorGrid(LevelGrid.GetSizeX(), RoomsDivisionConstraints.HallWidth),
								FVectorGrid(0, FHAxis),
								0
							));

							InitialHalls.Push(FHallBlock(
								FVectorGrid(LevelGrid.GetSizeX(), RoomsDivisionConstraints.HallWidth),
								FVectorGrid(0, SHAxis),
								0
							));

							if(FHSpace > 0)
								InitialHalls.Push(FHallBlock(
									FVectorGrid(RotatedSize.X, FHSpace),
									FVectorGrid(FinalRect.Position.X, FHAxis + RoomsDivisionConstraints.HallWidth),
									0
								));

							if(SHSpace > 0)
								InitialHalls.Push(FHallBlock(
									FVectorGrid(RotatedSize.X, SHSpace),
									FVectorGrid(FinalRect.Position.X, FinalRect.Position.Y + RotatedSize.Y),
									0
								));

							InitialBlocks.Push(FUnknownBlock(
								FVectorGrid(LevelGrid.GetSizeX(), FHAxis),
								FVectorGrid(0, 0),
								0,
								true
							));

							InitialBlocks.Push(FUnknownBlock(
								FVectorGrid(LevelGrid.GetSizeX(), LevelGrid.GetSizeY() - (SHAxis + RoomsDivisionConstraints.HallWidth)),
								FVectorGrid(0, SHAxis + RoomsDivisionConstraints.HallWidth),
								0,
								true
							));

							InitialBlocks.Push(FUnknownBlock(
								FVectorGrid(FinalRect.Position.X, SHAxis - (FHAxis + RoomsDivisionConstraints.HallWidth)),
								FVectorGrid(0, SHAxis + RoomsDivisionConstraints.HallWidth),
								0,
								true
							));

							InitialBlocks.Push(FUnknownBlock(
								FVectorGrid(LevelGrid.GetSizeX() - (FinalRect.Position.X + RotatedSize.X), SHAxis - (FHAxis + RoomsDivisionConstraints.HallWidth)),
								FVectorGrid(FinalRect.Position.X + RotatedSize.X, FHAxis + RoomsDivisionConstraints.HallWidth),
								0,
								true
							));
						}
					}
				
					//Case where it is in center along a X wall
					else if((LevelGrid.IsAlongXDownWall(FinalRect) || LevelGrid.IsAlongXUpWall(FinalRect)) && IsInYCenter())
					{
						if(RotatedSize.X < RotatedSize.Y /*To force placement with one hall*/ || !IsYCenterAvailable() || !IsNHXCenterAvailable())
							IsStairPlaceable = false;
						
						else if(LevelGrid.IsAlongXUpWall(FinalRect))
						{
							const int Space = FMath::Max(-RoomsDivisionConstraints.HallWidth, FinalRect.Position.X - LevelGrid.GetSizeX() + RoomsDivisionConstraints.ABSMinimalSide);
							InitialHalls.Push(FHallBlock(
								FVectorGrid(RoomsDivisionConstraints.HallWidth, LevelGrid.GetSizeY()),
								FVectorGrid(FinalRect.Position.X  - Space - RoomsDivisionConstraints.HallWidth, 0),
								0
							));

							if(Space > 0)
								InitialHalls.Push(FHallBlock(
									FVectorGrid(Space, RotatedSize.Y),
									FVectorGrid(FinalRect.Position.X  - Space, FinalRect.Position.Y),
									0
								));

							InitialBlocks.Push(FUnknownBlock(
								FVectorGrid(FinalRect.Position.X  - Space - RoomsDivisionConstraints.HallWidth, LevelGrid.GetSizeY()),
								FVectorGrid(0, 0),
								0,
								false
							));

							InitialBlocks.Push(FUnknownBlock(
								FVectorGrid(RotatedSize.X  + Space, FinalRect.Position.Y),
								FVectorGrid(FinalRect.Position.X  - Space, 0),
								0,
								false
							));

							InitialBlocks.Push(FUnknownBlock(
								FVectorGrid(RotatedSize.X  + Space, LevelGrid.GetSizeY() - (FinalRect.Position.Y + RotatedSize.Y)),
								FVectorGrid(FinalRect.Position.X  - Space, FinalRect.Position.Y + RotatedSize.Y),
								0,
								false
							));
						}
						else
						{
							const int Space = FMath::Max(-RoomsDivisionConstraints.HallWidth, RoomsDivisionConstraints.ABSMinimalSide - RotatedSize.X);
							InitialHalls.Push(FHallBlock(
								FVectorGrid(RoomsDivisionConstraints.HallWidth, LevelGrid.GetSizeY()),
								FVectorGrid( RotatedSize.X + Space, 0),
								0
							));

							if(Space > 0)
								InitialHalls.Push(FHallBlock(
									FVectorGrid(Space, RotatedSize.Y),
									FVectorGrid(RotatedSize.X, FinalRect.Position.Y),
									0
								));

							InitialBlocks.Push(FUnknownBlock(
								FVectorGrid(LevelGrid.GetSizeX() - (RotatedSize.X + Space + RoomsDivisionConstraints.HallWidth), LevelGrid.GetSizeY()),
								FVectorGrid(RotatedSize.X + Space + RoomsDivisionConstraints.HallWidth, 0),
								0,
								false
							));

							InitialBlocks.Push(FUnknownBlock(
								FVectorGrid(RotatedSize.X  + Space, FinalRect.Position.X),
								FVectorGrid(0, 0),
								0,
								false
							));

							InitialBlocks.Push(FUnknownBlock(
								FVectorGrid(RotatedSize.X  + Space, LevelGrid.GetSizeY() - (FinalRect.Position.Y + RotatedSize.Y)),
								FVectorGrid(0, FinalRect.Position.Y + RotatedSize.Y),
								0,
								false
							));
						}
					}
				
					//Case where it is in center along a Y wall
					else if((LevelGrid.IsAlongYDownWall(FinalRect) || LevelGrid.IsAlongYUpWall(FinalRect)) && IsInXCenter())
					{
						if(RotatedSize.Y < RotatedSize.X || !IsXCenterAvailable() || !IsNHYCenterAvailable())
							IsStairPlaceable = false;

						if(LevelGrid.IsAlongYUpWall(FinalRect))
						{
							const int Space = FMath::Max(-RoomsDivisionConstraints.HallWidth, FinalRect.Position.Y - LevelGrid.GetSizeY() + RoomsDivisionConstraints.ABSMinimalSide);
							InitialHalls.Push(FHallBlock(
								 FVectorGrid(LevelGrid.GetSizeX(), RoomsDivisionConstraints.HallWidth),
								 FVectorGrid(0, FinalRect.Position.Y  - Space - RoomsDivisionConstraints.HallWidth),
								 0
							 ));

							if(Space > 0)
						 		InitialHalls.Push(FHallBlock(
									 FVectorGrid(RotatedSize.X, Space),
									 FVectorGrid(FinalRect.Position.X, FinalRect.Position.Y  - Space),
									 0
								));

							InitialBlocks.Push(FUnknownBlock(
								FVectorGrid(LevelGrid.GetSizeX(), FinalRect.Position.Y  - Space - RoomsDivisionConstraints.HallWidth),
								FVectorGrid(0, 0),
								0,
								true
							));

							InitialBlocks.Push(FUnknownBlock(
								FVectorGrid(FinalRect.Position.X, RotatedSize.Y  + Space),
								FVectorGrid(0, FinalRect.Position.Y  - Space),
								0,
								true
							));
							
							InitialBlocks.Push(FUnknownBlock(
								FVectorGrid(LevelGrid.GetSizeX() - (FinalRect.Position.X + RotatedSize.X), RotatedSize.Y  + Space),
								FVectorGrid(FinalRect.Position.X + RotatedSize.X, FinalRect.Position.Y  - Space),
								0,
								true
							));
						}
						else
						{
							const int Space = FMath::Max(-RoomsDivisionConstraints.HallWidth, RoomsDivisionConstraints.ABSMinimalSide - RotatedSize.Y);
							InitialHalls.Push(FHallBlock(
								FVectorGrid(LevelGrid.GetSizeX(), RoomsDivisionConstraints.HallWidth),
								FVectorGrid(0, RotatedSize.Y + Space),
								0
							));

							if(Space > 0)
								InitialHalls.Push(FHallBlock(
									FVectorGrid(RotatedSize.X, Space),
									FVectorGrid(FinalRect.Position.X, RotatedSize.Y),
									0
								));

							InitialBlocks.Push(FUnknownBlock(
								FVectorGrid(LevelGrid.GetSizeX(),LevelGrid.GetSizeY() - (RotatedSize.Y + Space + RoomsDivisionConstraints.HallWidth)),
								FVectorGrid(0, RotatedSize.Y + Space + RoomsDivisionConstraints.HallWidth),
								0,
								true
							));

							InitialBlocks.Push(FUnknownBlock(
								FVectorGrid(FinalRect.Position.X, RotatedSize.Y  + Space),
								FVectorGrid(0, 0),
								0,
								true
							));

							InitialBlocks.Push(FUnknownBlock(
								FVectorGrid(LevelGrid.GetSizeX() - (FinalRect.Position.X + RotatedSize.X), RotatedSize.Y  + Space),
								FVectorGrid(FinalRect.Position.X + RotatedSize.X, 0),
								0,
								true
							));
						}
					}
				
					//Case it is in a corner (always ok, if the building step has successfully done its task)
					else if(LevelGrid.IsInAnyCorner(FinalRect))
					{
						if(RotatedSize.X >= RotatedSize.Y)
						{
							if(LevelGrid.IsAlongXUpWall(FinalRect))
							{
								const int Space = FMath::Max(-RoomsDivisionConstraints.HallWidth, FinalRect.Position.X - LevelGrid.GetSizeX() + RoomsDivisionConstraints.ABSMinimalSide);
								InitialHalls.Push(FHallBlock(
									FVectorGrid(RoomsDivisionConstraints.HallWidth, LevelGrid.GetSizeY()),
									FVectorGrid(FinalRect.Position.X  - Space - RoomsDivisionConstraints.HallWidth, 0),
									0
								));

								if(Space > 0)
									InitialHalls.Push(FHallBlock(
										FVectorGrid(Space, RotatedSize.Y),
										FVectorGrid(FinalRect.Position.X  - Space, FinalRect.Position.Y),
										0
									));

								InitialBlocks.Push(FUnknownBlock(
									FVectorGrid(FinalRect.Position.X  - Space - RoomsDivisionConstraints.HallWidth, LevelGrid.GetSizeY()),
									FVectorGrid(0, 0),
									0,
									false
								));

								InitialBlocks.Push(FUnknownBlock(
									FVectorGrid(RotatedSize.X  + Space, LevelGrid.GetSizeY() - RotatedSize.Y),
									FVectorGrid(FinalRect.Position.X  - Space, 0),
									0,
									false
								));

								if(LevelGrid.IsAlongYDownWall(FinalRect))
									InitialBlocks[1].GlobalPosition.Y = RotatedSize.Y;
							}
							else //Along XDown so Position.X = 0
							{
								const int Space = FMath::Max(-RoomsDivisionConstraints.HallWidth, RoomsDivisionConstraints.ABSMinimalSide - RotatedSize.X);
								InitialHalls.Push(FHallBlock(
									FVectorGrid(RoomsDivisionConstraints.HallWidth, LevelGrid.GetSizeY()),
									FVectorGrid( RotatedSize.X + Space, 0),
									0
								));

								if(Space > 0)
									InitialHalls.Push(FHallBlock(
										FVectorGrid(Space, RotatedSize.Y),
										FVectorGrid(RotatedSize.X, FinalRect.Position.Y),
										0
									));

								InitialBlocks.Push(FUnknownBlock(
									FVectorGrid(LevelGrid.GetSizeX() - (RotatedSize.X + Space + RoomsDivisionConstraints.HallWidth), LevelGrid.GetSizeY()),
									FVectorGrid(RotatedSize.X + Space + RoomsDivisionConstraints.HallWidth, 0),
									0,
									false
								));

								InitialBlocks.Push(FUnknownBlock(
									FVectorGrid(RotatedSize.X  + Space, LevelGrid.GetSizeY() - RotatedSize.Y),
									FVectorGrid(0, 0),
									0,
									false
								));

								if(LevelGrid.IsAlongYDownWall(FinalRect))
									InitialBlocks[1].GlobalPosition.Y = RotatedSize.Y;
							}
						}
						else
						{
							if(LevelGrid.IsAlongYUpWall(FinalRect))
							{
								const int Space = FMath::Max(-RoomsDivisionConstraints.HallWidth, FinalRect.Position.Y - LevelGrid.GetSizeY() + RoomsDivisionConstraints.ABSMinimalSide);
								InitialHalls.Push(FHallBlock(
									FVectorGrid(LevelGrid.GetSizeX(), RoomsDivisionConstraints.HallWidth),
									FVectorGrid(0, FinalRect.Position.Y  - Space - RoomsDivisionConstraints.HallWidth),
									0
								));

								if(Space > 0)
									InitialHalls.Push(FHallBlock(
										FVectorGrid(RotatedSize.X, Space),
										FVectorGrid(FinalRect.Position.X, FinalRect.Position.Y  - Space),
										0
									));

								InitialBlocks.Push(FUnknownBlock(
									FVectorGrid(LevelGrid.GetSizeX(), FinalRect.Position.Y  - Space - RoomsDivisionConstraints.HallWidth),
									FVectorGrid(0, 0),
									0,
									true
								));

								InitialBlocks.Push(FUnknownBlock(
									FVectorGrid(LevelGrid.GetSizeX() - RotatedSize.X, RotatedSize.Y  + Space),
									FVectorGrid(0, FinalRect.Position.Y  - Space),
									0,
									true
								));

								if(LevelGrid.IsAlongXDownWall(FinalRect))
									InitialBlocks[1].GlobalPosition.X = RotatedSize.X;
							}
							else
							{
								const int Space = FMath::Max(-RoomsDivisionConstraints.HallWidth, RoomsDivisionConstraints.ABSMinimalSide - RotatedSize.Y);
								InitialHalls.Push(FHallBlock(
									FVectorGrid(LevelGrid.GetSizeX(), RoomsDivisionConstraints.HallWidth),
									FVectorGrid(0, RotatedSize.Y + Space),
									0
								));

								if(Space > 0)
									InitialHalls.Push(FHallBlock(
										FVectorGrid(RotatedSize.X, Space),
										FVectorGrid(FinalRect.Position.X, RotatedSize.Y),
										0
									));

								InitialBlocks.Push(FUnknownBlock(
									FVectorGrid(LevelGrid.GetSizeX(),LevelGrid.GetSizeY() - (RotatedSize.Y + Space + RoomsDivisionConstraints.HallWidth)),
									FVectorGrid(0, RotatedSize.Y + Space + RoomsDivisionConstraints.HallWidth),
									0,
									true
								));

								InitialBlocks.Push(FUnknownBlock(
									FVectorGrid(LevelGrid.GetSizeX() - RotatedSize.X, RotatedSize.Y  + Space),
									FVectorGrid(0, 0),
									0,
									true
								));

								if(LevelGrid.IsAlongXDownWall(FinalRect))
									InitialBlocks[1].GlobalPosition.X = RotatedSize.X;
							}
						}
					}

					//If none of above cases, it means the rect isn't in a valid position
					else
						IsStairPlaceable = false;
				}
				
				if(IsStairPlaceable)
					PositionFound = LevelGrid.MarkFurnitureAtPosition(FinalRect, FinalConstraints);
				if(PositionFound) break;
			}

			if(PositionFound) break;
		}

		if(PositionFound) break;
	}

	assert(PositionFound);//If no position found for the stairs, the process exit.
}

void AHomeGenerator::DivideSurface(int Level , const TArray<FUnknownBlock> &InitialBlocks, const TArray<FHallBlock> &InitialHalls)
{
	//Basic checks
	assert(HallBlocks.IsValidIndex(Level) && RoomBlocks.IsValidIndex(Level));

	//BSP's storage initialisation
	TDoubleLinkedList<FUnknownBlock> ToDivide;
	for (auto Block : InitialBlocks)
	{
		Block.Level = Level;
		ToDivide.AddHead(Block);		
	}

	for (auto Hall : InitialHalls)
	{
		Hall.Level = Level;
		HallBlocks[Level].Push(Hall);
	}
	
	//Divide the generated blocks
	while (ToDivide.Num() != 0)
	{
		TDoubleLinkedList<FUnknownBlock>::TDoubleLinkedListNode *ExHead;
		switch (ToDivide.GetHead()->GetValue().ShouldDivide(RoomsDivisionConstraints))
		{
			//Create a new room and delete the bloc from the list.
			case FUnknownBlock::DivideMethod::NO_DIVIDE:
				RoomBlocks[Level].Push(FRoomBlock());
				ToDivide.GetHead()->GetValue().TransformToRoom(RoomBlocks[Level].Last());
				ToDivide.RemoveNode(ToDivide.GetHead());
				break;

			//Divide the block and place the generated block at the list's end
			//Create a hall
			case FUnknownBlock::DivideMethod::SPLIT:
				HallBlocks[Level].Push(FHallBlock());
			
				ExHead = ToDivide.GetHead();
				ToDivide.RemoveNode(ExHead, false);
				ToDivide.AddTail(ExHead);
				ToDivide.AddTail(FUnknownBlock());

				ExHead->GetValue().BlockSplit(RoomsDivisionConstraints, ToDivide.GetTail()->GetValue(), HallBlocks[Level].Last());
				break;			

			//Divide the block and place the generated block at the list's end 
			case FUnknownBlock::DivideMethod::DIVISION:
				ExHead = ToDivide.GetHead();
				ToDivide.RemoveNode(ExHead, false);
				ToDivide.AddTail(ExHead);
				ToDivide.AddTail(FUnknownBlock());

				ExHead->GetValue().BlockDivision(RoomsDivisionConstraints, ToDivide.GetTail()->GetValue());
				break;
			
			//Trouble in structure : exit.
			case FUnknownBlock::DivideMethod::ERROR:
			default:
				assert(false);
				return;
		}
	}
}

void AHomeGenerator::AllocateSurface()
{
	assert(HallBlocks.Num() == Level && RoomBlocks.Num() == Level);

	//Sort all rooms of the building
	TArray<FRoomBlock *> SortedRoomBlocks;
	int NeededPlace = 0;
	for (int i = 0; i < RoomBlocks.Num(); ++i) NeededPlace += RoomBlocks[i].Num(); //To avoid reallocation each time we add something
	SortedRoomBlocks.Reserve(NeededPlace);
	
	for (int i = 0; i < RoomBlocks.Num(); ++i)
		for (int j = 0; j < RoomBlocks[i].Num(); ++j)
			SortedRoomBlocks.Push(&RoomBlocks[i][j]);
	SortedRoomBlocks.Sort();

	//For the next part we suppose that `Rooms`' map has already been sorted accordingly to its MinimalSide property
	const int Diff  = NormalRoomQuantity - SortedRoomBlocks.Num();
	if(0 < Diff)
	{
		TArray<FName> RoomsToAvoid;
		RoomsToAvoid.Reserve(Diff);
		
		for (int i = 0; i < Diff; ++i)
		{
			TPair<FName, int> MaxPresence;
			TPair<FName, int> MinRest;
			MaxPresence.Value = 0;
			MinRest.Value = 1;

			for (const auto &Room : Rooms)
			{
				const float DesiredNumber = Room.Value.NumPerHab * Inhabitants;
				const float Rest = DesiredNumber - FMath::Floor(DesiredNumber);
				
				if(DesiredNumber > MaxPresence.Value)
				{
					MaxPresence.Key = Room.Key;
					MaxPresence.Value = DesiredNumber;
				}
				else if(DesiredNumber == MaxPresence.Value)
				{
					if(RoomsToAvoid.Contains(MaxPresence.Key) && !RoomsToAvoid.Contains(Room.Key))
					{
						MaxPresence.Key = Room.Key;
						MaxPresence.Value = DesiredNumber;
					}
				}

				if(Rest < MinRest.Value)
				{
					MinRest.Key = Room.Key;
					MinRest.Value = Rest;
				}
				else if(Rest == MinRest.Value)
				{
					if(RoomsToAvoid.Contains(MinRest.Key) && !RoomsToAvoid.Contains(Room.Key))
					{
						MinRest.Key = Room.Key;
						MinRest.Value = Rest;
					}
				}
			}

			if(MaxPresence.Value > 1)
				RoomsToAvoid.Push(MaxPresence.Key);
			else
				RoomsToAvoid.Push(MinRest.Key);
		}

		int RoomIndex = 0;
		for (const auto &Room : Rooms)
		{
			const int DesiredNumber = (Room.Value.NumPerHab * Inhabitants) - RoomsToAvoid.RemoveAll([&Room](const FName &A) {return A == Room.Key;});
			for (int i = 0; i < DesiredNumber; ++i)
			{
				SortedRoomBlocks[RoomIndex]->RoomType = Room.Key;
				++RoomIndex;
			}
		}
	}
	else
	{
		TArray<FName> RoomsToAdd;
		RoomsToAdd.Reserve(-Diff);
		
		for (int i = 0; i < -Diff; ++i)
		{
			TPair<FName, int> MinPresence;
			TPair<FName, int> MaxRest;
			MinPresence.Value = 0;
			MaxRest.Value = 1;

			for (const auto &Room : Rooms)
			{
				const float DesiredNumber = Room.Value.NumPerHab * Inhabitants;
				const float Rest = DesiredNumber - FMath::Floor(DesiredNumber);

				if(Rest > MaxRest.Value)
				{
					MaxRest.Key = Room.Key;
					MaxRest.Value = Rest;
				}
				else if(Rest == MaxRest.Value)
				{
					if(RoomsToAdd.Contains(MaxRest.Key) && !RoomsToAdd.Contains(Room.Key))
					{
						MaxRest.Key = Room.Key;
						MaxRest.Value = Rest;
					}
				}

				if(DesiredNumber < MinPresence.Value)
				{
					MinPresence.Key = Room.Key;
					MinPresence.Value = DesiredNumber;
				}
				else if(DesiredNumber == MinPresence.Value)
				{
					if(RoomsToAdd.Contains(MinPresence.Key) && !RoomsToAdd.Contains(Room.Key))
					{
						MinPresence.Key = Room.Key;
						MinPresence.Value = DesiredNumber;
					}
				}
			}

			if(MaxRest.Value > 0.5)
				RoomsToAdd.Push(MaxRest.Key);
			else
				RoomsToAdd.Push(MinPresence.Key);
		}

		int RoomIndex = 0;
		for (const auto &Room : Rooms)
		{
			const int DesiredNumber = (Room.Value.NumPerHab * Inhabitants) + RoomsToAdd.RemoveAll([&Room](const FName &A) {return A == Room.Key;});
			for (int i = 0; i < DesiredNumber; ++i)
			{
				SortedRoomBlocks[RoomIndex]->RoomType = Room.Key;
				++RoomIndex;
			}
		}
	}
}

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

void AHomeGenerator::GenerateRoom(const FName& RoomType, const FRoomBlock& RoomBlock)
{
	FRoomGrid RoomGrid(RoomBlock.Size);
	GenerateRoomDoors(RoomType, RoomBlock, RoomGrid);
	GenerateFurniture(RoomType, RoomBlock.GenerateRoomOffset(BuildingConstraints), RoomGrid);
	//GenerateDecoration(RoomType, ...)
}

void AHomeGenerator::GenerateRoomDoors(const FName& RoomType, const FRoomBlock& RoomBlock, FRoomGrid& RoomGrid)
{
	for(FDoorBlock* DoorBlock : RoomBlock.ConnectedDoors)
	{
		if (!DoorBlock->IsPlaced())
		{
			//Define a random possible position
			if (!DoorBlock->IsPositionValid())
			{
				const FRoomBlock *OtherRoom = DoorBlock->ObtainOppositeParent(RoomBlock);
				const FVectorGrid MarginSize = DoorBlock->GenerateLocalMarginSize(RoomBlock, Doors.DefaultConstraints.Margin);

				//Inclusive limits for the door
				FVectorGrid PositionMin = OtherRoom != nullptr ? FVectorGrid::Max(FVectorGrid(0,0), OtherRoom->GlobalPosition - RoomBlock.GlobalPosition) : FVectorGrid(0,0);
				FVectorGrid PositionMax = OtherRoom != nullptr ? FVectorGrid::Min(RoomBlock.Size , OtherRoom->Size + OtherRoom->GlobalPosition - RoomBlock.GlobalPosition) : RoomBlock.Size;
				PositionMax -= MarginSize;

				//Depending on its opening wall, it adjusts
				switch(DoorBlock->RoomWallAxe(RoomBlock))
				{
					case EGenerationAxe::X_UP: PositionMax.X = PositionMin.X = RoomBlock.Size.X; break;
					case EGenerationAxe::X_DOWN: PositionMax.X = PositionMin.X = 0; break;
					
					case EGenerationAxe::Y_UP: PositionMax.Y = PositionMin.Y = RoomBlock.Size.Y; break;
					case EGenerationAxe::Y_DOWN: PositionMax.Y = PositionMin.Y = 0; break;
					default : assert(false);
				}

				DoorBlock->SaveLocalPosition(FVectorGrid::Random(PositionMin, PositionMax), RoomBlock);
			}

			//Place the door
			PlaceMeshInWorld(DoorBlock->GetMeshAsset(), DoorBlock->GenerateLocalFurnitureRect(RoomBlock), RoomBlock.GenerateRoomOffset(BuildingConstraints));
		}
	}
}

void AHomeGenerator::GenerateFurniture(const FName& RoomType, const FVector& RoomOrigin, FRoomGrid& RoomGrid)
{
	check(RoomGrid.GetSizeX() > 0 && RoomGrid.GetSizeY() > 0)
	const FRoom * const Room = Rooms.Find(RoomType);
	check(Room->GetMinimalSide() <= RoomGrid.GetSizeX() &&  Room->GetMinimalSide() <= RoomGrid.GetSizeY())

	//Possible positions
	TArray<int> PositionX;
	TArray<int> PositionY;
	TArray<EFurnitureRotation> Rotations = {EFurnitureRotation::ROT0, EFurnitureRotation::ROT90, EFurnitureRotation::ROT180, EFurnitureRotation::ROT270};
	GenerateRangeArray(PositionX, RoomGrid.GetSizeX());
	GenerateRangeArray(PositionY, RoomGrid.GetSizeY());

	//Shuffle everything here to allow more random generation (useless to update on each mesh)
	ShuffleArray(PositionX);
	ShuffleArray(PositionY);
	ShuffleArray(Rotations);

	//Dependencies management
	TArray<FDependencyBuffer> FurnitureWithDep;

	//First furniture placement
	for(const auto &_FurnitureType : Room->Furniture)
	{
		FFurniture * const _Furniture = Furniture.Find(_FurnitureType);
		//Checks on the found structure (just skip if there are some errors)
		if(_Furniture == nullptr)
			continue;
		
		//Shuffle everything here to allow more random generation (useless to update on each mesh)
		ShuffleArray(_Furniture->Mesh);
		ShuffleArray(PositionX);
		ShuffleArray(PositionY);
		ShuffleArray(Rotations);

		//Find already known values
		const uint8 DependencyIndex = _Furniture->Dependencies.Num() > 0 ? FurnitureWithDep.Num() + 1 : 0;
		bool MeshFounded = false;
		
		for(const UFurnitureMeshAsset* _Mesh : _Furniture->Mesh)
		{
			//Checks on the found structure (just skip if there are some errors)
			if(_Mesh == nullptr || !IsValid(_Mesh->ActorClass) && !IsValid(_Mesh->Mesh))
				continue;

			//Define needed value
			const FFurnitureConstraint &FinalConstraints = _Mesh->bOverrideConstraint ? _Mesh->ConstraintsOverride : _Furniture->DefaultConstraints;
			
			for(const int X : PositionX)
			{
				for(const int Y : PositionY)
				{
					for(const auto Rotation : Rotations)
					{
						FFurnitureRect FinalRect(Rotation, FVectorGrid(X, Y), _Mesh->GridSize);
						MeshFounded = RoomGrid.MarkFurnitureAtPosition(FinalRect, FinalConstraints, DependencyIndex);
						if(MeshFounded)
						{
							AActor * SpawnedActor = PlaceMeshInWorld(_Mesh, FinalRect, RoomOrigin);
							if(DependencyIndex)
								FurnitureWithDep.Push(FDependencyBuffer(_Furniture->Dependencies, FinalRect));

							break;
						}
					}

					if(MeshFounded)
						break;
				}

				if(MeshFounded)
					break;
			}

			if(MeshFounded)
				break;
		}		
	}

	//Dependency placement
	for(uint8 i = 0; i < FurnitureWithDep.Num(); ++i)
	{
		for(const FFurnitureDependency &_Dependency : FurnitureWithDep[i].Dependencies)
		{
			FFurniture * const _Furniture = Furniture.Find(_Dependency.FurnitureType);
			//Checks on the found structure (just skip if there are some errors)
			if(_Furniture == nullptr)
				continue;
		
			//Shuffle everything here to allow more random generation (useless to update on each mesh)
			ShuffleArray(_Furniture->Mesh);
			ShuffleArray(PositionX);
			ShuffleArray(PositionY);
			ShuffleArray(Rotations);
			
			bool MeshFounded = false;
		
			for(const UFurnitureMeshAsset* _Mesh : _Furniture->Mesh)
			{
				//Checks on the found structure (just skip if there are some errors)
				if(_Mesh == nullptr || !IsValid(_Mesh->ActorClass) && !IsValid(_Mesh->Mesh))
					continue;

				//Define needed value
				const FFurnitureConstraint &FinalConstraints = _Mesh->bOverrideConstraint ? _Mesh->ConstraintsOverride : _Furniture->DefaultConstraints;
			
				for(const int X : PositionX)
				{
					for(const int Y : PositionY)
					{
						for(const auto Rotation : Rotations)
						{
							FFurnitureRect FinalRect(Rotation, FVectorGrid(X, Y), _Mesh->GridSize);
							MeshFounded = RoomGrid.MarkDependencyAtPosition(FinalRect, FurnitureWithDep[i].ParentPosition, FinalConstraints, _Dependency, i + 1);
							if(MeshFounded)
							{
								AActor *SpawnedActor = PlaceMeshInWorld(_Mesh, FinalRect, RoomOrigin);
								break;
							}
						}

						if(MeshFounded)
							break;
					}

					if(MeshFounded)
						break;
				}

				if(MeshFounded)
					break;
			}		
		}
	}
}

AActor* AHomeGenerator::PlaceMeshInWorld(const UFurnitureMeshAsset* MeshAsset, const FFurnitureRect& FurnitureRect, const FVector& RoomOffset)
{
	//Primary check
	if(MeshAsset == nullptr)
		return nullptr;

	//Handle position and rotation
	FTransform ToSpawnTransform(RoomOffset + FurnitureRect.Position.ToVector(BuildingConstraints.GridSnapLength));
	switch (FurnitureRect.Rotation)
	{
		case EFurnitureRotation::ROT0:
			ToSpawnTransform.AddToTranslation(-MeshAsset->BoundsOrigin);
			break;
		case EFurnitureRotation::ROT90:
			ToSpawnTransform.AddToTranslation(-FVector(MeshAsset->BoundsOrigin.Y, MeshAsset->BoundsOrigin.X, MeshAsset->BoundsOrigin.Z));
			ToSpawnTransform.SetRotation(FRotator(0.f, 90.f, 0.f).Quaternion());
			break;
		case EFurnitureRotation::ROT180: 
			ToSpawnTransform.AddToTranslation(MeshAsset->BoundsOrigin);
			ToSpawnTransform.SetRotation(FRotator(0.f, 180.f, 0.f).Quaternion());
			break;
		case EFurnitureRotation::ROT270:
			ToSpawnTransform.AddToTranslation(FVector(MeshAsset->BoundsOrigin.Y, MeshAsset->BoundsOrigin.X, MeshAsset->BoundsOrigin.Z));
			ToSpawnTransform.SetRotation(FRotator(0.f, 270.f, 0.f).Quaternion());
			break;
	}

	//Handle component creation
	AActor *SpawnedActor;
	if(IsValid(MeshAsset->ActorClass))
	{
		SpawnedActor = GetWorld()->SpawnActor(MeshAsset->ActorClass.Get(), &ToSpawnTransform);
		
		if(MeshAsset->ActorClass.Get()->ImplementsInterface(UFurnitureMeshInt::StaticClass()) && IsValid(MeshAsset->Mesh))
			IFurnitureMeshInt::Execute_GetStaticMeshComponent(SpawnedActor)->SetStaticMesh(MeshAsset->Mesh);
	}
	else
	{
		SpawnedActor = GetWorld()->SpawnActor(AStaticMeshActor::StaticClass(), &ToSpawnTransform);
		const AStaticMeshActor * const StaticActor = Cast<AStaticMeshActor>(SpawnedActor);
		StaticActor->GetStaticMeshComponent()->SetStaticMesh(MeshAsset->Mesh);
	}

	//Finally attach to the generator
	SpawnedActor->AttachToActor(this, FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));

	return SpawnedActor;
}

// Called every frame
void AHomeGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


// Fill out your copyright notice in the Description page of Project Settings.


#include "HomeGenerator.h"
#include "Engine/StaticMeshActor.h"

void FRoomsDivisionConstraints::CalculateAllSides(const int _BasicMinimalSide, const int _BasicAverageSide, const int _BasicMaximalSide)
{
	ABSMinimalSide = FMath::CeilToInt(MinSideCoef * _BasicMinimalSide);
	StopSplitSide = FMath::CeilToInt(StopSplitCoef * _BasicAverageSide);
	SufficientSide = FMath::CeilToInt(SufficientChunkCoef * _BasicMaximalSide);
}

int FRoom::CalculateMinimalSide(TMap<FName, FFurniture> &FurnitureMapping)
{
	int NeededArea = 0;
	for(const auto FurnitureType : Furniture)
	{
		FFurniture &_Furniture = FurnitureMapping[FurnitureType];
		NeededArea += _Furniture.GetAverageArea();
	}
	MinRoomSide = FMath::CeilToInt(FMath::Sqrt(NeededArea));

	return MinRoomSide;
}

int FRoom::GetMinimalSide() const
{
	return MinRoomSide;
}

int FFurniture::GetAverageArea()
{
	if(AverageArea == 0) //If not already calculated, calculate it
	{
		int Sum = 0;
		for(const UFurnitureMeshAsset * const MeshObj : Mesh)
			Sum += MeshObj->GetArea(*this);

		// ENH: Floor/Ceil is important there ?
		AverageArea =  Sum / Mesh.Num();
	}

	return AverageArea;
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

void AHomeGenerator::ComputeSides()
{
	int MinimalSide = INT_MAX;
	int MaximalSide = 0;
	int AverageSide = 0;
	BuildingConstraints.NormalRoomQuantity = 0;
	
	for(auto Room : Rooms)
	{
		const int Quantity = FMath::CeilToInt(Room.Value.NumPerHab * Inhabitants);
		BuildingConstraints.NormalRoomQuantity += Quantity;
		
		if(Room.Value.CalculateMinimalSide(Furniture) > 0 && Quantity > 0)
		{
			if(Room.Value.GetMinimalSide() < MinimalSide)
				MinimalSide = Room.Value.GetMinimalSide();
			if(Room.Value.GetMinimalSide() > MaximalSide)
				MaximalSide = Room.Value.GetMinimalSide();
			
			AverageSide += Quantity * Room.Value.GetMinimalSide();
		}
	}
	
	AverageSide /= BuildingConstraints.NormalRoomQuantity;
	RoomsDivisionConstraints.CalculateAllSides(MinimalSide, AverageSide, MaximalSide);
	BuildingConstraints.AverageSide = AverageSide;

	//Croissant order
	auto PredicateRooms = [&] (const FRoom &A, const FRoom &B) { return A.GetMinimalSide() < B.GetMinimalSide(); };
	Rooms.ValueSort(PredicateRooms);
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

void AHomeGenerator::DefineBuilding()
{
	//
	//Chooses for each special furniture a mesh
	check(Doors.Mesh.Num() > 0 && Stairs.Mesh.Num() > 0 && Windows.Mesh.Num() > 0)
	SelectedDoor = Doors.Mesh[FMath::RandRange(0, Doors.Mesh.Num() - 1)];
	SelectedStair = Stairs.Mesh[FMath::RandRange(0, Stairs.Mesh.Num() - 1)];
	SelectedWindow = Windows.Mesh[FMath::RandRange(0, Windows.Mesh.Num() - 1)];

	//
	//Calculates building's dimensions
	
	//Basic steps
	const int MinimalSideMin = FMath::Max(BuildingConstraints.MinSideFloorLength, RoomsDivisionConstraints.ABSMinimalSide + SelectedStair->GridSize.MinSide());
	const int MinimalSideMax = FMath::Max(BuildingConstraints.MinSideFloorLength, RoomsDivisionConstraints.ABSMinimalSide + FMath::Max(RoomsDivisionConstraints.ABSMinimalSide + RoomsDivisionConstraints.HallWidth, SelectedStair->GridSize.MaxSide()));
	
	const int AreaPerStage = FMath::RandRange(
		FMath::Max(MinimalSideMin * MinimalSideMax,FMath::CeilToInt(SelectedStair->GetArea(Stairs) / (1 - RoomsDivisionConstraints.MaxHallRatio))),
		FMath::Max(MinimalSideMin * MinimalSideMax, FMath::Square(BuildingConstraints.MaxSideFloorLength)) //ENH:What should we do if set to 0
	);
	const float Intermediate = static_cast<float>(AreaPerStage) * (1 - RoomsDivisionConstraints.MaxHallRatio) - static_cast<float>(SelectedStair->GetArea(Stairs));

	//Level's calculation
	const int LevelMin = FMath::CeilToInt(BuildingConstraints.NormalRoomQuantity * FMath::Square<float>(FMath::Max<int>(
		BuildingConstraints.AverageSide,
		RoomsDivisionConstraints.ABSMinimalSide
	)) / Intermediate);
	const int LevelMax = FMath::Max(LevelMin,
		FMath::Min(
			FMath::CeilToInt((BuildingConstraints.NormalRoomQuantity + Rooms.Num()) * FMath::Square<float>(RoomsDivisionConstraints.SufficientSide) / Intermediate),
			BuildingConstraints.MaxFloorsNumber > 0 ? BuildingConstraints.MaxFloorsNumber : INT_MAX
		)
	);
	BuildingConstraints.Levels = FMath::RandRange(LevelMin, LevelMax);

	//Building size calculation
	if(FMath::RandBool()) //X side -> min side
	{
		BuildingConstraints.BuildingSize.X = FMath::RandRange(MinimalSideMin, AreaPerStage / MinimalSideMax);
		BuildingConstraints.BuildingSize.Y = FMath::CeilToInt(AreaPerStage / BuildingConstraints.BuildingSize.X);
	}
	else
	{
		BuildingConstraints.BuildingSize.Y = FMath::RandRange(MinimalSideMin, AreaPerStage / MinimalSideMax);
		BuildingConstraints.BuildingSize.X = FMath::CeilToInt(AreaPerStage / BuildingConstraints.BuildingSize.Y);
	}	

	//
	//Positions the main door and prepare the division for the first floor (the only affected by this door)
	//ENH : We will see that later actually the enter will be a hole
}

void AHomeGenerator::DefineRooms()
{
	FLevelOrganisation InitialOrganisation; //Stores initial B/H on the heap
	TArray<FLevelOrganisation> LevelsOrganisation;
	TDoubleLinkedList<FUnknownBlock> NodesToDelete;
	
	StairsPositioning(InitialOrganisation);
	LevelsOrganisation.Init(InitialOrganisation, BuildingConstraints.Levels);
	
	for (int i = 0; i < BuildingConstraints.Levels; ++i)
	{
		HallBlocks.Push(TArray<FHallBlock>());
		RoomBlocks.Push(TArray<FRoomBlock>());
		DivideSurface(i, LevelsOrganisation[i], NodesToDelete);
	}
	InitialOrganisation.Empty();//InitialOrganisation isn't valid anymore

	ComputeWallEffect(LevelsOrganisation);
	NodesToDelete.Empty();//All level organisations aren't valid anymore
	
	AllocateSurface();
	CompleteHallSurface();
}

void AHomeGenerator::StairsPositioning(FLevelOrganisation &InitialOrganisation)
{
	FRoomGrid LevelGrid(BuildingConstraints.BuildingSize);
	
	//Possible positions
	TArray<int> PositionX;
	TArray<int> PositionY;
	TArray<EFurnitureRotation> Rotations = {EFurnitureRotation::ROT0, EFurnitureRotation::ROT90, EFurnitureRotation::ROT180, EFurnitureRotation::ROT270};
	GenerateRangeArray(PositionX, LevelGrid.GetSizeX());
	GenerateRangeArray(PositionY, LevelGrid.GetSizeY());

	//Shuffle everything here to allow more random generation
	ShuffleArray(PositionX);
	ShuffleArray(PositionY);
	ShuffleArray(Rotations);

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
				InitialOrganisation.Empty();

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

					InitialOrganisation.SetHallBlock(FLevelOrganisation::Stairs, new FHallBlock(
						RotatedSize,
						FinalRect.Position,
						0
					));

					//ENH : The code in the center case could replace all other cases (just if we check X > 0 for all X calculated value)			
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

							InitialOrganisation.SetHallBlock(
								FLevelOrganisation::LowCorridor,
								new FHallBlock(
									FVectorGrid(RoomsDivisionConstraints.HallWidth, LevelGrid.GetSizeY()),
									FVectorGrid(FHAxis, 0),
									0
							));

							InitialOrganisation.SetHallBlock(
								FLevelOrganisation::HighCorridor,
								new FHallBlock(
									FVectorGrid(RoomsDivisionConstraints.HallWidth, LevelGrid.GetSizeY()),
									FVectorGrid(SHAxis, 0),
									0
							));

							if(FHSpace > 0)
								InitialOrganisation.SetHallBlock(
									FLevelOrganisation::LowMargin,
									new FHallBlock(
										FVectorGrid(FHSpace, RotatedSize.Y),
										FVectorGrid(FHAxis + RoomsDivisionConstraints.HallWidth, FinalRect.Position.Y),
										0
								));

							if(SHSpace > 0)
								InitialOrganisation.SetHallBlock(
									FLevelOrganisation::HighMargin,
									new FHallBlock(
										FVectorGrid(SHSpace, RotatedSize.Y),
										FVectorGrid(FinalRect.Position.X + RotatedSize.X, FinalRect.Position.Y),
										0
								));

							InitialOrganisation.SetUnknownBlock(
								FLevelOrganisation::LowWing,
								new FUnknownBlock(
									FVectorGrid(FHAxis, LevelGrid.GetSizeY()),
									FVectorGrid(0, 0),
									0,
									false,
									static_cast<uint8>(EGenerationAxe::X_UP),
									EGenerationAxe::X_UP
							));

							InitialOrganisation.SetUnknownBlock(
								FLevelOrganisation::HighWing,
								new FUnknownBlock(
								FVectorGrid(LevelGrid.GetSizeX() - (SHAxis + RoomsDivisionConstraints.HallWidth), LevelGrid.GetSizeY()),
								FVectorGrid(SHAxis + RoomsDivisionConstraints.HallWidth, 0),
								0,
								false,
								static_cast<uint8>(EGenerationAxe::X_DOWN),
								EGenerationAxe::X_DOWN
							));

							InitialOrganisation.SetUnknownBlock(
								FLevelOrganisation::LowApartment,
								new FUnknownBlock(
									FVectorGrid(SHAxis - (FHAxis + RoomsDivisionConstraints.HallWidth), FinalRect.Position.Y),
									FVectorGrid(SHAxis + RoomsDivisionConstraints.HallWidth, 0),
									0,
									false,
									EGenerationAxe::X_DOWN | EGenerationAxe::X_UP | EGenerationAxe::Y_UP,
									EGenerationAxe::X_DOWN
							));

							InitialOrganisation.SetUnknownBlock(
								FLevelOrganisation::HighApartment,
								new FUnknownBlock(
									FVectorGrid(SHAxis - (FHAxis + RoomsDivisionConstraints.HallWidth), LevelGrid.GetSizeY() - (FinalRect.Position.Y + RotatedSize.Y)),
									FVectorGrid(FHAxis + RoomsDivisionConstraints.HallWidth, FinalRect.Position.Y + RotatedSize.Y),
									0,
									false,
									EGenerationAxe::X_DOWN | EGenerationAxe::X_UP | EGenerationAxe::Y_DOWN,
									EGenerationAxe::X_UP
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

							InitialOrganisation.SetHallBlock(
								FLevelOrganisation::LowCorridor,
								new FHallBlock(
									FVectorGrid(LevelGrid.GetSizeX(), RoomsDivisionConstraints.HallWidth),
									FVectorGrid(0, FHAxis),
									0
							));

							InitialOrganisation.SetHallBlock(
								FLevelOrganisation::HighCorridor,
								new FHallBlock(
									FVectorGrid(LevelGrid.GetSizeX(), RoomsDivisionConstraints.HallWidth),
									FVectorGrid(0, SHAxis),
									0
							));

							if(FHSpace > 0)
								InitialOrganisation.SetHallBlock(
								FLevelOrganisation::LowMargin,
									new FHallBlock(
										FVectorGrid(RotatedSize.X, FHSpace),
										FVectorGrid(FinalRect.Position.X, FHAxis + RoomsDivisionConstraints.HallWidth),
										0
								));

							if(SHSpace > 0)
								InitialOrganisation.SetHallBlock(
								FLevelOrganisation::HighMargin,
									new FHallBlock(
										FVectorGrid(RotatedSize.X, SHSpace),
										FVectorGrid(FinalRect.Position.X, FinalRect.Position.Y + RotatedSize.Y),
										0
								));

							InitialOrganisation.SetUnknownBlock(
							FLevelOrganisation::LowWing,
								new FUnknownBlock(
									FVectorGrid(LevelGrid.GetSizeX(), FHAxis),
									FVectorGrid(0, 0),
									0,
									true,
									static_cast<uint8>(EGenerationAxe::Y_UP),
									EGenerationAxe::Y_UP
							));

							InitialOrganisation.SetUnknownBlock(
								FLevelOrganisation::HighWing,
								new FUnknownBlock(
									FVectorGrid(LevelGrid.GetSizeX(), LevelGrid.GetSizeY() - (SHAxis + RoomsDivisionConstraints.HallWidth)),
									FVectorGrid(0, SHAxis + RoomsDivisionConstraints.HallWidth),
									0,
									true,
									static_cast<uint8>(EGenerationAxe::Y_DOWN),
									EGenerationAxe::Y_DOWN
							));

							InitialOrganisation.SetUnknownBlock(
								FLevelOrganisation::LowApartment,
								new FUnknownBlock(
									FVectorGrid(FinalRect.Position.X, SHAxis - (FHAxis + RoomsDivisionConstraints.HallWidth)),
									FVectorGrid(0, SHAxis + RoomsDivisionConstraints.HallWidth),
									0,
									true,
									EGenerationAxe::Y_DOWN | EGenerationAxe::Y_UP | EGenerationAxe::X_UP,
									EGenerationAxe::Y_DOWN
							));

							InitialOrganisation.SetUnknownBlock(
								FLevelOrganisation::HighApartment,
								new FUnknownBlock(
									FVectorGrid(LevelGrid.GetSizeX() - (FinalRect.Position.X + RotatedSize.X), SHAxis - (FHAxis + RoomsDivisionConstraints.HallWidth)),
									FVectorGrid(FinalRect.Position.X + RotatedSize.X, FHAxis + RoomsDivisionConstraints.HallWidth),
									0,
									true,
									EGenerationAxe::Y_DOWN | EGenerationAxe::Y_UP | EGenerationAxe::X_DOWN,
									EGenerationAxe::Y_UP
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
							InitialOrganisation.SetHallBlock(
								FLevelOrganisation::LowCorridor,
									new FHallBlock(
										FVectorGrid(RoomsDivisionConstraints.HallWidth, LevelGrid.GetSizeY()),
										FVectorGrid(FinalRect.Position.X  - Space - RoomsDivisionConstraints.HallWidth, 0),
										0
							));

							if(Space > 0)
								InitialOrganisation.SetHallBlock(
									FLevelOrganisation::LowMargin,
									new FHallBlock(
										FVectorGrid(Space, RotatedSize.Y),
										FVectorGrid(FinalRect.Position.X  - Space, FinalRect.Position.Y),
										0
								));

							InitialOrganisation.SetUnknownBlock(
								FLevelOrganisation::LowWing,
								new FUnknownBlock(
									FVectorGrid(FinalRect.Position.X  - Space - RoomsDivisionConstraints.HallWidth, LevelGrid.GetSizeY()),
									FVectorGrid(0, 0),
									0,
									false,
									static_cast<uint8>(EGenerationAxe::X_UP),
									EGenerationAxe::X_UP
							));
							
							InitialOrganisation.SetUnknownBlock(
								FLevelOrganisation::LowApartment,
								new FUnknownBlock(
									FVectorGrid(RotatedSize.X  + Space, FinalRect.Position.Y),
									FVectorGrid(FinalRect.Position.X  - Space, 0),
									0,
									false,
									EGenerationAxe::X_DOWN | EGenerationAxe::Y_UP,
									EGenerationAxe::X_DOWN
							));

							InitialOrganisation.SetUnknownBlock(
								FLevelOrganisation::HighApartment,
								new FUnknownBlock(
									FVectorGrid(RotatedSize.X  + Space, LevelGrid.GetSizeY() - (FinalRect.Position.Y + RotatedSize.Y)),
									FVectorGrid(FinalRect.Position.X  - Space, FinalRect.Position.Y + RotatedSize.Y),
									0,
									false,
									EGenerationAxe::X_DOWN | EGenerationAxe::Y_DOWN,
									EGenerationAxe::X_DOWN
							));
						}
						else
						{
							const int Space = FMath::Max(-RoomsDivisionConstraints.HallWidth, RoomsDivisionConstraints.ABSMinimalSide - RotatedSize.X);
							InitialOrganisation.SetHallBlock(
								FLevelOrganisation::HighCorridor,
								new FHallBlock(
									FVectorGrid(RoomsDivisionConstraints.HallWidth, LevelGrid.GetSizeY()),
									FVectorGrid( RotatedSize.X + Space, 0),
									0
							));

							if(Space > 0)
								InitialOrganisation.SetHallBlock(
									FLevelOrganisation::HighMargin,
									new FHallBlock(
										FVectorGrid(Space, RotatedSize.Y),
										FVectorGrid(RotatedSize.X, FinalRect.Position.Y),
										0
								));

							InitialOrganisation.SetUnknownBlock(
								FLevelOrganisation::HighWing,
								new FUnknownBlock(
									FVectorGrid(LevelGrid.GetSizeX() - (RotatedSize.X + Space + RoomsDivisionConstraints.HallWidth), LevelGrid.GetSizeY()),
									FVectorGrid(RotatedSize.X + Space + RoomsDivisionConstraints.HallWidth, 0),
									0,
									false,
									static_cast<uint8>(EGenerationAxe::X_DOWN),
									EGenerationAxe::X_DOWN
							));

							InitialOrganisation.SetUnknownBlock(
								FLevelOrganisation::LowApartment,
								new FUnknownBlock(
									FVectorGrid(RotatedSize.X  + Space, FinalRect.Position.X),
									FVectorGrid(0, 0),
									0,
									false,
									EGenerationAxe::X_UP | EGenerationAxe::Y_UP,
									EGenerationAxe::X_UP
							));

							InitialOrganisation.SetUnknownBlock(
								FLevelOrganisation::HighApartment,
								new FUnknownBlock(
									FVectorGrid(RotatedSize.X  + Space, LevelGrid.GetSizeY() - (FinalRect.Position.Y + RotatedSize.Y)),
									FVectorGrid(0, FinalRect.Position.Y + RotatedSize.Y),
									0,
									false,
									EGenerationAxe::X_UP | EGenerationAxe::Y_DOWN,
									EGenerationAxe::X_UP
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
							InitialOrganisation.SetHallBlock(
								FLevelOrganisation::LowCorridor,
								new FHallBlock(
									 FVectorGrid(LevelGrid.GetSizeX(), RoomsDivisionConstraints.HallWidth),
									 FVectorGrid(0, FinalRect.Position.Y  - Space - RoomsDivisionConstraints.HallWidth),
									 0
							 ));

							if(Space > 0)
						 		InitialOrganisation.SetHallBlock(
								FLevelOrganisation::LowMargin,
									new FHallBlock(
										 FVectorGrid(RotatedSize.X, Space),
										 FVectorGrid(FinalRect.Position.X, FinalRect.Position.Y  - Space),
										 0
								));

							InitialOrganisation.SetUnknownBlock(
								FLevelOrganisation::LowWing,
								new FUnknownBlock(
									FVectorGrid(LevelGrid.GetSizeX(), FinalRect.Position.Y  - Space - RoomsDivisionConstraints.HallWidth),
									FVectorGrid(0, 0),
									0,
									true,
									static_cast<uint8>(EGenerationAxe::Y_UP),
									EGenerationAxe::Y_UP
							));

							InitialOrganisation.SetUnknownBlock(
								FLevelOrganisation::LowApartment,
								new FUnknownBlock(
									FVectorGrid(FinalRect.Position.X, RotatedSize.Y  + Space),
									FVectorGrid(0, FinalRect.Position.Y  - Space),
									0,
									true,
									EGenerationAxe::Y_DOWN | EGenerationAxe::X_UP,
									EGenerationAxe::Y_DOWN
							));
							
							InitialOrganisation.SetUnknownBlock(
								FLevelOrganisation::HighApartment,
								new FUnknownBlock(
									FVectorGrid(LevelGrid.GetSizeX() - (FinalRect.Position.X + RotatedSize.X), RotatedSize.Y  + Space),
									FVectorGrid(FinalRect.Position.X + RotatedSize.X, FinalRect.Position.Y  - Space),
									0,
									true,
									EGenerationAxe::Y_DOWN | EGenerationAxe::X_DOWN,
									EGenerationAxe::Y_DOWN
							));
						}
						else
						{
							const int Space = FMath::Max(-RoomsDivisionConstraints.HallWidth, RoomsDivisionConstraints.ABSMinimalSide - RotatedSize.Y);
							InitialOrganisation.SetHallBlock(
								FLevelOrganisation::HighCorridor,
								new FHallBlock(
									FVectorGrid(LevelGrid.GetSizeX(), RoomsDivisionConstraints.HallWidth),
									FVectorGrid(0, RotatedSize.Y + Space),
									0
							));

							if(Space > 0)
								InitialOrganisation.SetHallBlock(
									FLevelOrganisation::HighMargin,
									new FHallBlock(
										FVectorGrid(RotatedSize.X, Space),
										FVectorGrid(FinalRect.Position.X, RotatedSize.Y),
										0
								));

							InitialOrganisation.SetUnknownBlock(
								FLevelOrganisation::HighWing,
								new FUnknownBlock(
									FVectorGrid(LevelGrid.GetSizeX(),LevelGrid.GetSizeY() - (RotatedSize.Y + Space + RoomsDivisionConstraints.HallWidth)),
									FVectorGrid(0, RotatedSize.Y + Space + RoomsDivisionConstraints.HallWidth),
									0,
									true,
									static_cast<uint8>(EGenerationAxe::Y_DOWN),
									EGenerationAxe::Y_DOWN
							));

							InitialOrganisation.SetUnknownBlock(
								FLevelOrganisation::LowApartment,
								new FUnknownBlock(
									FVectorGrid(FinalRect.Position.X, RotatedSize.Y  + Space),
									FVectorGrid(0, 0),
									0,
									true,
									EGenerationAxe::Y_UP | EGenerationAxe::X_UP,
									EGenerationAxe::Y_UP
							));

							InitialOrganisation.SetUnknownBlock(
								FLevelOrganisation::HighApartment,
								new FUnknownBlock(
									FVectorGrid(LevelGrid.GetSizeX() - (FinalRect.Position.X + RotatedSize.X), RotatedSize.Y  + Space),
									FVectorGrid(FinalRect.Position.X + RotatedSize.X, 0),
									0,
									true,
									EGenerationAxe::Y_UP | EGenerationAxe::X_DOWN,
									EGenerationAxe::Y_UP
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
								InitialOrganisation.SetHallBlock(
									FLevelOrganisation::LowCorridor,
									new FHallBlock(
										FVectorGrid(RoomsDivisionConstraints.HallWidth, LevelGrid.GetSizeY()),
										FVectorGrid(FinalRect.Position.X  - Space - RoomsDivisionConstraints.HallWidth, 0),
										0
								));

								if(Space > 0)
									InitialOrganisation.SetHallBlock(
										FLevelOrganisation::LowMargin,
										new FHallBlock(
											FVectorGrid(Space, RotatedSize.Y),
											FVectorGrid(FinalRect.Position.X  - Space, FinalRect.Position.Y),
											0
									));

								InitialOrganisation.SetUnknownBlock(
									FLevelOrganisation::LowWing,
									new FUnknownBlock(
										FVectorGrid(FinalRect.Position.X  - Space - RoomsDivisionConstraints.HallWidth, LevelGrid.GetSizeY()),
										FVectorGrid(0, 0),
										0,
										false,
										static_cast<uint8>(EGenerationAxe::X_UP),
										EGenerationAxe::X_UP
								));

								if(LevelGrid.IsAlongYDownWall(FinalRect))
									InitialOrganisation.SetUnknownBlock(
										FLevelOrganisation::HighApartment,
										new FUnknownBlock(
											FVectorGrid(RotatedSize.X  + Space, LevelGrid.GetSizeY() - RotatedSize.Y),
											FVectorGrid(FinalRect.Position.X  - Space,  RotatedSize.Y),
											0,
											false,
											EGenerationAxe::X_DOWN | EGenerationAxe::Y_DOWN,
											EGenerationAxe::X_DOWN
									));
								else
									InitialOrganisation.SetUnknownBlock(
										FLevelOrganisation::LowApartment,
										new FUnknownBlock(
											FVectorGrid(RotatedSize.X  + Space, LevelGrid.GetSizeY() - RotatedSize.Y),
											FVectorGrid(FinalRect.Position.X  - Space, 0),
											0,
											false,
											EGenerationAxe::X_DOWN | EGenerationAxe::Y_UP,
											EGenerationAxe::X_DOWN
									));
							}
							else //Along XDown so Position.X = 0
							{
								const int Space = FMath::Max(-RoomsDivisionConstraints.HallWidth, RoomsDivisionConstraints.ABSMinimalSide - RotatedSize.X);
								InitialOrganisation.SetHallBlock(
									FLevelOrganisation::HighCorridor,
									new FHallBlock(
										FVectorGrid(RoomsDivisionConstraints.HallWidth, LevelGrid.GetSizeY()),
										FVectorGrid( RotatedSize.X + Space, 0),
										0
								));

								if(Space > 0)
									InitialOrganisation.SetHallBlock(
										FLevelOrganisation::HighMargin,
										new FHallBlock(
											FVectorGrid(Space, RotatedSize.Y),
											FVectorGrid(RotatedSize.X, FinalRect.Position.Y),
											0
									));

								InitialOrganisation.SetUnknownBlock(
									FLevelOrganisation::HighWing,
									new FUnknownBlock(
										FVectorGrid(LevelGrid.GetSizeX() - (RotatedSize.X + Space + RoomsDivisionConstraints.HallWidth), LevelGrid.GetSizeY()),
										FVectorGrid(RotatedSize.X + Space + RoomsDivisionConstraints.HallWidth, 0),
										0,
										false,
										static_cast<uint8>(EGenerationAxe::X_DOWN),
										EGenerationAxe::X_DOWN
								));

								if(LevelGrid.IsAlongYDownWall(FinalRect))
									InitialOrganisation.SetUnknownBlock(
										FLevelOrganisation::HighApartment,
										new FUnknownBlock(
											FVectorGrid(RotatedSize.X  + Space, LevelGrid.GetSizeY() - RotatedSize.Y),
											FVectorGrid(0, RotatedSize.Y),
											0,
											false,
											EGenerationAxe::X_UP | EGenerationAxe::Y_DOWN,
											EGenerationAxe::X_UP
									));
								else
									InitialOrganisation.SetUnknownBlock(
										FLevelOrganisation::LowApartment,
										new FUnknownBlock(
											FVectorGrid(RotatedSize.X  + Space, LevelGrid.GetSizeY() - RotatedSize.Y),
											FVectorGrid(0, 0),
											0,
											false,
											EGenerationAxe::X_UP | EGenerationAxe::Y_UP,
											EGenerationAxe::X_UP
									));
							}
						}
						else
						{
							if(LevelGrid.IsAlongYUpWall(FinalRect))
							{
								const int Space = FMath::Max(-RoomsDivisionConstraints.HallWidth, FinalRect.Position.Y - LevelGrid.GetSizeY() + RoomsDivisionConstraints.ABSMinimalSide);
								InitialOrganisation.SetHallBlock(
									FLevelOrganisation::LowCorridor,
									new FHallBlock(
										FVectorGrid(LevelGrid.GetSizeX(), RoomsDivisionConstraints.HallWidth),
										FVectorGrid(0, FinalRect.Position.Y  - Space - RoomsDivisionConstraints.HallWidth),
										0
								));

								if(Space > 0)
									InitialOrganisation.SetHallBlock(
										FLevelOrganisation::LowMargin,
										new FHallBlock(
											FVectorGrid(RotatedSize.X, Space),
											FVectorGrid(FinalRect.Position.X, FinalRect.Position.Y  - Space),
											0
									));

								InitialOrganisation.SetUnknownBlock(
									FLevelOrganisation::LowWing,
									new FUnknownBlock(
										FVectorGrid(LevelGrid.GetSizeX(), FinalRect.Position.Y  - Space - RoomsDivisionConstraints.HallWidth),
										FVectorGrid(0, 0),
										0,
										true,
										static_cast<uint8>(EGenerationAxe::Y_UP),
										EGenerationAxe::Y_UP
								));
								
								if(LevelGrid.IsAlongXDownWall(FinalRect))
									InitialOrganisation.SetUnknownBlock(
										FLevelOrganisation::HighApartment,
										new FUnknownBlock(
											FVectorGrid(LevelGrid.GetSizeX() - RotatedSize.X, RotatedSize.Y  + Space),
											FVectorGrid(RotatedSize.X, FinalRect.Position.Y  - Space),
											0,
											true,
											EGenerationAxe::Y_DOWN | EGenerationAxe::X_DOWN,
											EGenerationAxe::Y_DOWN
									));
								else
									InitialOrganisation.SetUnknownBlock(
										FLevelOrganisation::LowApartment,
										new FUnknownBlock(
											FVectorGrid(LevelGrid.GetSizeX() - RotatedSize.X, RotatedSize.Y  + Space),
											FVectorGrid(0, FinalRect.Position.Y  - Space),
											0,
											true,
											EGenerationAxe::Y_DOWN | EGenerationAxe::X_UP,
											EGenerationAxe::Y_DOWN
									));
							}
							else
							{
								const int Space = FMath::Max(-RoomsDivisionConstraints.HallWidth, RoomsDivisionConstraints.ABSMinimalSide - RotatedSize.Y);
								InitialOrganisation.SetHallBlock(
									FLevelOrganisation::HighCorridor,
									new FHallBlock(
										FVectorGrid(LevelGrid.GetSizeX(), RoomsDivisionConstraints.HallWidth),
										FVectorGrid(0, RotatedSize.Y + Space),
										0
								));

								if(Space > 0)
									InitialOrganisation.SetHallBlock(
										FLevelOrganisation::HighMargin,
										new FHallBlock(
											FVectorGrid(RotatedSize.X, Space),
											FVectorGrid(FinalRect.Position.X, RotatedSize.Y),
											0
									));

								InitialOrganisation.SetUnknownBlock(
									FLevelOrganisation::HighWing,
									new FUnknownBlock(
										FVectorGrid(LevelGrid.GetSizeX(),LevelGrid.GetSizeY() - (RotatedSize.Y + Space + RoomsDivisionConstraints.HallWidth)),
										FVectorGrid(0, RotatedSize.Y + Space + RoomsDivisionConstraints.HallWidth),
										0,
										true,
										static_cast<uint8>(EGenerationAxe::Y_DOWN),
										EGenerationAxe::Y_DOWN
								));

								if(LevelGrid.IsAlongXDownWall(FinalRect))
									InitialOrganisation.SetUnknownBlock(
										FLevelOrganisation::HighApartment,
										new FUnknownBlock(
											FVectorGrid(LevelGrid.GetSizeX() - RotatedSize.X, RotatedSize.Y  + Space),
											FVectorGrid(RotatedSize.X, 0),
											0,
											true,
											EGenerationAxe::Y_UP | EGenerationAxe::X_DOWN,
											EGenerationAxe::Y_UP
									));
								else
									InitialOrganisation.SetUnknownBlock(
										FLevelOrganisation::LowApartment,
										new FUnknownBlock(
											FVectorGrid(LevelGrid.GetSizeX() - RotatedSize.X, RotatedSize.Y  + Space),
											FVectorGrid(0, 0),
											0,
											true,
											EGenerationAxe::Y_UP | EGenerationAxe::X_UP,
											EGenerationAxe::Y_UP
									));
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
	//If no position was found for the stairs, the process exit.
	check(PositionFound);
}

void AHomeGenerator::DivideSurface(const int Level, FLevelOrganisation &LevelOrganisation, TDoubleLinkedList<FUnknownBlock> &NodesToDelete)
{
	check(HallBlocks.IsValidIndex(Level) && RoomBlocks.IsValidIndex(Level))

	//BSP's storage initialisation
	FLevelDivisionData LevelDivisionData(BuildingConstraints.BuildingSize.Area(), LevelOrganisation.InitialHallArea());
	TArray<FUnknownBlock *> FinalBlocks;
	
	TDoubleLinkedList<FUnknownBlock> ToDivide;
	for (uint8 i = 0; i < FLevelOrganisation::BlockPositionsSize; ++i) 
	{
		ToDivide.AddHead(*LevelOrganisation.GetBlockList()[i]);
		ToDivide.GetHead()->GetValue().Level = Level;

		// Replaces the pointer to the initial block
		LevelOrganisation.SetUnknownBlock(static_cast<FLevelOrganisation::EInitialBlockPositions>(i), &ToDivide.GetHead()->GetValue());
	}

	for (uint8 i = 0; i < FLevelOrganisation::HallPositionsSize; ++i) 
	{
		HallBlocks[Level].Push(*LevelOrganisation.GetHallList()[i]);
		HallBlocks[Level].Last().Level = Level;

		// Replaces the pointer to the initial block
		LevelOrganisation.SetHallBlock(static_cast<FLevelOrganisation::EInitialHallPositions>(i), &HallBlocks[Level].Last());
	}
	
	//Divide the generated blocks
	//When a block is generated it generate two new blocks (added to the list) before being retrieved from the list (its node is pointed by the array NodeToDelete)
	while (ToDivide.Num() != 0)
	{
		//Removes actual node.
		TDoubleLinkedList<FUnknownBlock>::TDoubleLinkedListNode * const ExHead = ToDivide.GetHead();
		TDoubleLinkedList<FUnknownBlock>::TDoubleLinkedListNode *TailBuffer;
		ToDivide.RemoveNode(ExHead, false);
		NodesToDelete.AddHead(ExHead);
		
		switch (ToDivide.GetHead()->GetValue().ShouldDivide(RoomsDivisionConstraints, LevelDivisionData))
		{
			//Creates a new room.
			case FUnknownBlock::DivideMethod::NO_DIVIDE:
				RoomBlocks[Level].Push(FRoomBlock());
				ExHead->GetValue().TransformToRoom(RoomBlocks[Level].Last());
				FinalBlocks.Push(&ExHead->GetValue());
				break;

			//Divides the block and places generated blocks at the list's end
			//Creates a hall.
			case FUnknownBlock::DivideMethod::SPLIT:
				HallBlocks[Level].Push(FHallBlock());
				ToDivide.AddTail(FUnknownBlock());
				TailBuffer = ToDivide.GetTail();
				ToDivide.AddTail(FUnknownBlock());
			
				ExHead->GetValue().BlockSplit(RoomsDivisionConstraints, TailBuffer->GetValue(), ToDivide.GetTail()->GetValue(), HallBlocks[Level].Last());
				break;			

			//Divides the block and places generated blocks at the list's end.
			case FUnknownBlock::DivideMethod::DIVISION:
				ToDivide.AddTail(FUnknownBlock());
				TailBuffer = ToDivide.GetTail();
				ToDivide.AddTail(FUnknownBlock());
				ExHead->GetValue().BlockDivision(RoomsDivisionConstraints, TailBuffer->GetValue(), ToDivide.GetTail()->GetValue());
				break;
			
			//Trouble in structure : exit.
			case FUnknownBlock::DivideMethod::ERROR:
			default:
				check(false);
				return;
		}
	}

	for(auto *FinalBlock : FinalBlocks)
		FinalBlock->ConnectDoors(SelectedDoor);
}

void AHomeGenerator::ComputeWallEffect(TArray<FLevelOrganisation>& LevelsOrganisation)
{
	check(LevelsOrganisation.Num() == BuildingConstraints.Levels)
	TArray<FVector2D> NeededOffsets;
	NeededOffsets.Reserve(LevelsOrganisation.Num());
	
	//Compute all basic data recursively
	for (int i = 0; i < LevelsOrganisation.Num(); ++i)
	{
		LevelsOrganisation[i].ComputeBasicRealData(BuildingConstraints, RoomsDivisionConstraints);
		NeededOffsets.Push(LevelsOrganisation[i].GetStairsRealOffset()); //First stores stairs offset of each lvl
	}

	//Find maximal stairs' offset
	FVector2D MaxOffset = FVector2D::ZeroVector;
	for (const auto &Offset : NeededOffsets)
	{
		if(Offset.X > MaxOffset.X)
			MaxOffset.X = Offset.X;
		if(Offset.Y > MaxOffset.Y)
			MaxOffset.Y = Offset.Y;
	}

	//Finally compute all internal real data by aligning all stairs
	for (int i = 0; i < NeededOffsets.Num(); ++i) //Then stores the offset to add to each lvl
		LevelsOrganisation[i].ComputeAllRealData(MaxOffset - NeededOffsets[i], BuildingConstraints, RoomsDivisionConstraints);
}

void AHomeGenerator::AllocateSurface()
{
	check(HallBlocks.Num() == BuildingConstraints.Levels && RoomBlocks.Num() == BuildingConstraints.Levels);

	//Sort all rooms of the building
	TArray<FRoomBlock *> SortedRoomBlocks;
	int NeededPlace = 0;
	for (int i = 0; i < RoomBlocks.Num(); ++i) NeededPlace += RoomBlocks[i].Num(); //To avoid reallocation each time we add something
	SortedRoomBlocks.Reserve(NeededPlace);
	
	for (int i = 0; i < RoomBlocks.Num(); ++i)
		for (int j = 0; j < RoomBlocks[i].Num(); ++j)
			SortedRoomBlocks.Push(&RoomBlocks[i][j]);
	SortedRoomBlocks.Sort(); //Croissant order : we define first the smallest ones

	//For the next part we suppose that `Rooms`' map has already been sorted accordingly to its MinimalSide property
	const int Diff  = BuildingConstraints.NormalRoomQuantity - SortedRoomBlocks.Num();
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
				//Here we use Floor to check the decimal part : lower one means more rounded when floored
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
	else if(0 > Diff)
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
				//Here we use Floor to check the decimal part : bigger one means less rounded when floored
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
					default : check(false);
				}

				DoorBlock->SaveLocalPosition(FVectorGrid::Random(PositionMin, PositionMax), RoomBlock);
			}

			//Place the door
			PlaceMeshInWorld(DoorBlock->GetMeshAsset(), DoorBlock->GenerateLocalFurnitureRect(RoomBlock), RoomBlock.GenerateRoomOffset(BuildingConstraints));
			DoorBlock->MarkAsPlaced();
		}

		//Marks the grid, for the future furniture placement
		RoomGrid.MarkDoorAtPosition(DoorBlock->GenerateLocalFurnitureRect(RoomBlock));
	}
}

void AHomeGenerator::GenerateFurniture(const FName& RoomType, const FVector& RoomOrigin, FRoomGrid& RoomGrid)
{
	const FRoom * const Room = Rooms.Find(RoomType);
	check(RoomGrid.GetSizeX() > 0 && RoomGrid.GetSizeY() > 0)
	check(Room->GetMinimalSide() <= RoomGrid.GetSizeX() &&  Room->GetMinimalSide() <= RoomGrid.GetSizeY())

	//Possible positions
	TArray<int> PositionX;
	TArray<int> PositionY;
	TArray<EFurnitureRotation> Rotations = {EFurnitureRotation::ROT0, EFurnitureRotation::ROT90, EFurnitureRotation::ROT180, EFurnitureRotation::ROT270};
	GenerateRangeArray(PositionX, RoomGrid.GetSizeX());
	GenerateRangeArray(PositionY, RoomGrid.GetSizeY());

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


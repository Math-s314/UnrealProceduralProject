// Fill out your copyright notice in the Description page of Project Settings.


#include "HomeGenerator.h"
#include "Engine/StaticMeshActor.h"
#include <assert.h>

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


// Fill out your copyright notice in the Description page of Project Settings.


#include "FurnitureMeshAsset.h"
#include "HomeGenerator.h"

bool FWallInteractionStruct::GetFirstAttractedWall(EGenerationAxe& First) const
{
	if(XUp == EWallAxeInteraction::ATTRACT)
	{
		First = EGenerationAxe::X_UP;
		return true;
	}

	if(XDown == EWallAxeInteraction::ATTRACT)
	{
		First = EGenerationAxe::X_DOWN;
		return true;
	}

	if(YUp == EWallAxeInteraction::ATTRACT)
	{
		First = EGenerationAxe::Y_UP;
		return true;
	}

	if(YDown == EWallAxeInteraction::ATTRACT)
	{
		First = EGenerationAxe::Y_DOWN;
		return true;
	}

	return false;
}

bool FWallInteractionStruct::GetSecondAttractedWall(EGenerationAxe& Second) const
{
	EGenerationAxe First = EGenerationAxe::X_UP;
	if(!GetFirstAttractedWall(First))
		return false;

	switch (First)
	{
		case EGenerationAxe::X_UP:
			if(XDown == EWallAxeInteraction::ATTRACT)
			{
				Second = EGenerationAxe::X_DOWN;
				return true;
			}
		
		case EGenerationAxe::X_DOWN:
			if(YUp == EWallAxeInteraction::ATTRACT)
			{
				Second = EGenerationAxe::Y_UP;
				return true;
			}
		
		case EGenerationAxe::Y_UP:
			if(YDown == EWallAxeInteraction::ATTRACT)
			{
				Second = EGenerationAxe::Y_DOWN;
				return true;
			}
		
		default:
			return false;
	}
}

int UFurnitureMeshAsset::GetArea(const FFurniture& CorrespondingFurniture) const
{
	const FFurnitureConstraint &Constraints = bOverrideConstraint ? ConstraintsOverride : CorrespondingFurniture.DefaultConstraints;
	return (GridSize.X + Constraints.Margin.XDown + Constraints.Margin.XUp) * (GridSize.Y + Constraints.Margin.YDown + Constraints.Margin.YUp);
}

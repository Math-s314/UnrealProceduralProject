// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProceduralProjectGameMode.h"
#include "ProceduralProjectCharacter.h"
#include "UObject/ConstructorHelpers.h"

AProceduralProjectGameMode::AProceduralProjectGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "HierarchicalPathFindGameMode.h"
#include "HierarchicalPathFindCharacter.h"
#include "UObject/ConstructorHelpers.h"

AHierarchicalPathFindGameMode::AHierarchicalPathFindGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

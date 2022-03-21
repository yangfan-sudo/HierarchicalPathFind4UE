// Fill out your copyright notice in the Description page of Project Settings.


#include "HPFPlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Runtime/Engine/Classes/Components/DecalComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "HPFAIController.h"

AHPFPlayerController::AHPFPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void AHPFPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	InputComponent->BindAction("SetDestination", IE_Pressed, this, &AHPFPlayerController::OnSetDestinationPressed);
	InputComponent->BindAction("SetDestination", IE_Released, this, &AHPFPlayerController::OnSetDestinationReleased);

	// support touch devices 
	InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AHPFPlayerController::MoveToTouchLocation);
	InputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AHPFPlayerController::MoveToTouchLocation);
}

void AHPFPlayerController::MoveToTouchLocation(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	FVector2D ScreenSpaceLocation(Location);

	// Trace to see what is under the touch location
	FHitResult HitResult;
	GetHitResultAtScreenPosition(ScreenSpaceLocation, CurrentClickTraceChannel, true, HitResult);
	if (HitResult.bBlockingHit)
	{
		// We hit something, move there
		SetNewMoveDestination(HitResult.ImpactPoint);
	}
}

void AHPFPlayerController::SetNewMoveDestination(const FVector DestLocation)
{
	APawn* const MyPawn = GetPawn();
	if (MyPawn)
	{
		//float const Distance = FVector::Dist(DestLocation, MyPawn->GetActorLocation());

		//// We need to issue move command only if far enough in order for walk animation to play correctly
		//if ((Distance > 120.0f))
		//{
		//	//UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, DestLocation);
		//}

		TArray<AActor*> HPFAIControllers;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHPFAIController::StaticClass(), HPFAIControllers);
		for (int i=0;i< HPFAIControllers.Num();i++)
		{
			AHPFAIController* TmpControll = Cast<AHPFAIController>(HPFAIControllers[i]);
			TmpControll->SetGoalLocation(DestLocation);
		}
	}
}

void AHPFPlayerController::OnSetDestinationPressed()
{
	// set flag to keep updating destination until released
	// Trace to see what is under the mouse cursor
	
}

void AHPFPlayerController::OnSetDestinationReleased()
{
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, false, Hit);

	if (Hit.bBlockingHit)
	{
		// We hit something, move there
		SetNewMoveDestination(Hit.ImpactPoint);
	}
}

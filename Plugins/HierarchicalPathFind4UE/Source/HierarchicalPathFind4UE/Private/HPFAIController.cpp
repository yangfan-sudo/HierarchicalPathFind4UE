// Fill out your copyright notice in the Description page of Project Settings.


#include "HPFAIController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "HierarchicalPathImplement.h"

void AHPFAIController::SetGoalLocation(const FVector& GoalLocation)
{
	if (!GetPawn())
	{
		return;
	}
	if (FVector::Distance(GoalLocation, GetPawn()->GetActorLocation()) < HPFDistance)
	{
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, GoalLocation);
	}
	else
	{
		ToGoalPathPoints.Empty();
		if (HierarchicalPathImplement::GetHerarchicalPath(GetWorld(), GetPawn()->GetActorLocation(), GoalLocation, GetNavAgentPropertiesRef(), ToGoalPathPoints))
		{
			ContinueMove();
		}
	}

}

void AHPFAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	if (Result.Code == EPathFollowingResult::Success)
	{
		ContinueMove();
	}
	else
	{
		ToGoalPathPoints.Empty();
	}
}

void AHPFAIController::ContinueMove()
{
	if (ToGoalPathPoints.Num() > 0)
	{
		FVector TargetLocation = ToGoalPathPoints[0];
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, TargetLocation);
		ToGoalPathPoints.RemoveAt(0);
	}
}
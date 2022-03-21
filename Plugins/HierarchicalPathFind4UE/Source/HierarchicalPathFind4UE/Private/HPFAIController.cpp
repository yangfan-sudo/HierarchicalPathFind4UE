// Fill out your copyright notice in the Description page of Project Settings.


#include "HPFAIController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "HierarchicalPathImplement.h"
#include "DrawDebugHelpers.h"

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
			for (int i=0;i< ToGoalPathPoints.Num();i++)
			{
				DrawDebugSphere(GetWorld(), ToGoalPathPoints[i], 300, 8, FColor::Red, true);
			}
			
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
	Super::OnMoveCompleted(RequestID, Result);
}

void AHPFAIController::ContinueMove()
{
	if (ToGoalPathPoints.Num() > 0)
	{
		FVector TargetLocation = ToGoalPathPoints[0];
		ToGoalPathPoints.Remove(TargetLocation);
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, TargetLocation);	
	}
}
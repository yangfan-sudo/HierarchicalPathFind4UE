// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "HPFAIController.generated.h"

/**
 * 
 */
UCLASS()
class HIERARCHICALPATHFIND4UE_API AHPFAIController : public AAIController
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	float HPFDistance = 100000;

	TArray<FVector> ToGoalPathPoints;

	void SetGoalLocation(const FVector& GoalLocation);

	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	void ContinueMove();
};

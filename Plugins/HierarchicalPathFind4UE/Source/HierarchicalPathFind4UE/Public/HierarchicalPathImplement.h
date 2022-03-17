// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Detour/DetourStatus.h"
#include "Detour/DetourNavMesh.h"


class ARecastNavMesh;
struct dtQueryResult;
class UNavigationSystemV1;

/**
 * 
 */
class HIERARCHICALPATHFIND4UE_API HierarchicalPathImplement
{
public:
	HierarchicalPathImplement();
	~HierarchicalPathImplement();

	static bool GetHerarchicalPath(UWorld* CurrentWorld, const FVector& StartPoint, const FVector& EndPoint, FNavAgentProperties NavAgentProperties, TArray<FVector>& PathPoints);
private:
	static dtStatus getClusterPath(const dtNavMesh* m_nav, dtPolyRef startRef, dtPolyRef endRef, TArray<dtPolyRef>& result);

	static bool ProjectPointToNavmesh(UNavigationSystemV1* NavSys, FNavAgentProperties NavAgentProperties,
		const FVector& ProjectLocation, FVector& OutLocation);
};

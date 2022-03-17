// Fill out your copyright notice in the Description page of Project Settings.


#include "HierarchicalPathImplement.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"
#include "NavMesh/RecastHelpers.h"
#include "Detour/DetourNavMeshQuery.h"
#include "Detour/DetourAssert.h"
#include "Detour/DetourAlloc.h"
#include "NavigationData.h"
#include "AI/Navigation/NavQueryFilter.h"
#include "HierarchicalNodePool.h"
#include "Detour/DetourNode.h"



static const float DEFAULT_HEURISTIC_SCALE = 0.999f; // Search heuristic scale.
static const int maxNodes = 1024;

HierarchicalPathImplement::HierarchicalPathImplement()
{

}

HierarchicalPathImplement::~HierarchicalPathImplement()
{

}

bool HierarchicalPathImplement::GetHerarchicalPath(UWorld* CurrentWorld, const FVector& StartPoint, const FVector& EndPoint,
	FNavAgentProperties NavAgentProperties, TArray<FVector>& PathPoints)
{
	if (CurrentWorld == nullptr)
	{
		return false;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(CurrentWorld);

	if (!NavSys)
	{
		return false;
	}

	ANavigationData* NavData = NavSys->GetNavDataForProps(NavAgentProperties);
	if (NavData == nullptr)
	{
		return false;
	}

	ARecastNavMesh* RecastNavData = Cast<ARecastNavMesh>(NavData);
	if (!RecastNavData) return false;

	FSharedConstNavQueryFilter QueryFilter = NavData->GetDefaultQueryFilter();

	if (QueryFilter == NULL)
	{
		UE_LOG(LogTemp, Error, TEXT("FPImplRecastNavMesh::FindPath failed due to passed filter having NULL implementation!"));
		return false;
	}

	FVector DefaultQueryExtent = NavData->GetConfig().DefaultQueryExtent;
	dtPolyRef tStartPoly = RecastNavData->FindNearestPoly(StartPoint, DefaultQueryExtent);
	dtPolyRef tEndPoly = RecastNavData->FindNearestPoly(EndPoint, DefaultQueryExtent);
	if (tStartPoly == tEndPoly)
	{
		return false;
	}
	const dtNavMesh* Navmesh = RecastNavData->GetRecastMesh();

	TArray<dtPolyRef> PointsOut;
	getClusterPath(Navmesh, tStartPoly, tEndPoly, PointsOut);
	PathPoints.Add(StartPoint);
	for (int i = 0;i < PointsOut.Num();i++)
	{
		dtClusterRef clusterRef = PointsOut[i];
		const dtMeshTile* bestTile = Navmesh->getTileByRef(clusterRef);
		const unsigned int bestClusterIdx = Navmesh->decodeClusterIdCluster(clusterRef);
		const dtCluster* bestCluster = &bestTile->clusters[bestClusterIdx];

		FVector TmpCenter = Recast2UnrealPoint(bestCluster->center);

		FVector ProjectedLocation;
		if (ProjectPointToNavmesh(NavSys, NavAgentProperties, TmpCenter, ProjectedLocation))
		{
			PathPoints.Add(ProjectedLocation);
		}
		else
		{
			PathPoints.Add(TmpCenter);
		}
	}
	PathPoints.Add(EndPoint);

	return true;
}

bool HierarchicalPathImplement::ProjectPointToNavmesh(UNavigationSystemV1* NavSys, FNavAgentProperties NavAgentProperties,
	const FVector& ProjectLocation, FVector& OutLocation)
{
	if (!NavSys)
	{
		return false;
	}

	ANavigationData* NavData = NavSys->GetNavDataForProps(NavAgentProperties);
	if (NavData == nullptr)
	{
		return false;
	}

	ARecastNavMesh* RecastNavData = Cast<ARecastNavMesh>(NavData);
	if (!RecastNavData) return false;

	FNavLocation TmpLocation;
	FVector ProjectExtent = RecastNavData->GetConfig().DefaultQueryExtent;
	if (NavSys && NavSys->ProjectPointToNavigation(ProjectLocation, TmpLocation, ProjectExtent, &NavAgentProperties))
	{
		OutLocation = TmpLocation;
		//DrawDebugSphere(GetWorld(), TempGoalLocation, 40, 4, FColor::Red, false, 5);
		return true;
	}
	OutLocation = ProjectLocation;
	return false;

}

dtStatus HierarchicalPathImplement::getClusterPath(const dtNavMesh* m_nav, dtPolyRef startRef, dtPolyRef endRef, TArray<dtPolyRef>& result)
{
	hierarchicalNodePool* m_nodePool = new (dtAlloc(sizeof(hierarchicalNodePool), DT_ALLOC_PERM)) hierarchicalNodePool(maxNodes, dtNextPow2(maxNodes / 4));		///< Pointer to node pool.
	hierarchicalNodeQueue* m_openList = new (dtAlloc(sizeof(hierarchicalNodeQueue), DT_ALLOC_PERM)) hierarchicalNodeQueue(maxNodes);		///< Pointer to open list queue.

	const dtMeshTile* startTile = m_nav->getTileByRef(startRef);
	const dtMeshTile* endTile = m_nav->getTileByRef(endRef);
	const unsigned int startPolyIdx = m_nav->decodePolyIdPoly(startRef);
	const unsigned int endPolyIdx = m_nav->decodePolyIdPoly(endRef);

	int m_queryNodes = 0;
	const int loopLimit = m_nodePool->getMaxRuntimeNodes() + 1;
	result.Empty();

	if (startTile == 0 || endTile == 0 ||
		startTile->polyClusters == 0 || endTile->polyClusters == 0 ||
		startPolyIdx >= (unsigned int)startTile->header->offMeshBase ||
		endPolyIdx >= (unsigned int)endTile->header->offMeshBase)
	{
		// this means most probably the hierarchical graph has not been build at all
		return DT_FAILURE | DT_INVALID_PARAM;
	}

	const unsigned int startIdx = startTile->polyClusters[startPolyIdx];
	const unsigned int endIdx = endTile->polyClusters[endPolyIdx];
	const dtCluster& startCluster = startTile->clusters[startIdx];
	const dtCluster& endCluster = endTile->clusters[endIdx];

	const dtClusterRef startCRef = m_nav->getClusterRefBase(startTile) | (dtClusterRef)startIdx;
	const dtClusterRef endCRef = m_nav->getClusterRefBase(endTile) | (dtClusterRef)endIdx;
	if (startCRef == endCRef)
	{
		result.Add(startCRef);
		return DT_SUCCESS;
	}

	m_nodePool->clear();
	m_openList->clear();

	dtNode* startNode = m_nodePool->getNode(startCRef);
	dtVcopy(startNode->pos, startCluster.center);
	startNode->pidx = 0;
	startNode->cost = 0;
	startNode->total = dtVdist(startCluster.center, endCluster.center) * DEFAULT_HEURISTIC_SCALE;
	startNode->id = startCRef;
	startNode->flags = DT_NODE_OPEN;
	m_openList->push(startNode);
	m_queryNodes++;

	dtNode* lastBestNode = startNode;
	float lastBestNodeCost = startNode->total;

	dtStatus status = DT_FAILURE;
	while (!m_openList->empty())
	{
		// Remove node from open list and put it in closed list.
		dtNode* bestNode = m_openList->pop();
		bestNode->flags &= ~DT_NODE_OPEN;
		bestNode->flags |= DT_NODE_CLOSED;

		// Reached the goal, stop searching.
		if (bestNode->id == endCRef)
		{
			lastBestNode = bestNode;
			break;
		}

		// Get current cluster
		const dtClusterRef bestRef = bestNode->id;
		const dtMeshTile* bestTile = m_nav->getTileByRef(bestRef);
		const unsigned int bestClusterIdx = m_nav->decodeClusterIdCluster(bestRef);
		const dtCluster* bestCluster = &bestTile->clusters[bestClusterIdx];

		// Get parent ref
		const dtClusterRef parentRef = (bestNode->pidx) ? m_nodePool->getNodeAtIdx(bestNode->pidx)->id : 0;

		// Iterate through links
		unsigned int i = bestCluster->firstLink;
		while (i != DT_NULL_LINK)
		{
			// don't update link, cost is not important
			const dtClusterLink& link = m_nav->getClusterLink(bestTile, i);
			i = link.next;

			const dtClusterRef& neighbourRef = link.ref;

			// do not expand back to where we came from.
			if (!neighbourRef || neighbourRef == parentRef)
				continue;

			// Check backtracking
			if ((link.flags & DT_CLINK_VALID_FWD) == 0)
				continue;

			// Get neighbour poly and tile.
			// The API input has been cheked already, skip checking internal data.
			const dtMeshTile* neighbourTile = m_nav->getTileByRef(neighbourRef);
			const dtCluster* neighbourCluster = &neighbourTile->clusters[m_nav->decodeClusterIdCluster(neighbourRef)];

			dtNode* neighbourNode = m_nodePool->getNode(neighbourRef);
			if (!neighbourNode)
			{
				status |= DT_OUT_OF_NODES;
				continue;
			}

			// If the node is visited the first time, calculate node position.
			if (neighbourNode->flags == 0)
			{
				dtVcopy(neighbourNode->pos, neighbourCluster->center);
			}

			// Calculate cost and heuristic.
			const float cost = bestNode->cost;
			const float heuristic = (neighbourRef != endCRef) ? dtVdist(neighbourNode->pos, endCluster.center) * DEFAULT_HEURISTIC_SCALE : 0.0f;
			const float total = cost + heuristic;

			// The node is already in open list and the new result is worse, skip.
			if ((neighbourNode->flags & DT_NODE_OPEN) && total >= neighbourNode->total)
				continue;
			// The node is already visited and process, and the new result is worse, skip.
			if ((neighbourNode->flags & DT_NODE_CLOSED) && total >= neighbourNode->total)
				continue;

			// Add or update the node.
			neighbourNode->pidx = m_nodePool->getNodeIdx(bestNode);
			neighbourNode->id = neighbourRef;
			neighbourNode->flags = (neighbourNode->flags & ~DT_NODE_CLOSED);
			neighbourNode->cost = cost;
			neighbourNode->total = total;

			if (neighbourNode->flags & DT_NODE_OPEN)
			{
				// Already in open, update node location.
				m_openList->modify(neighbourNode);
			}
			else
			{
				// Put the node in open list.
				neighbourNode->flags |= DT_NODE_OPEN;
				m_openList->push(neighbourNode);
				m_queryNodes++;
			}

			// Update nearest node to target so far.
			if (heuristic < lastBestNodeCost)
			{
				lastBestNodeCost = heuristic;
				lastBestNode = neighbourNode;
			}
		}
	}

	if (lastBestNode->id == endCRef)
		status = DT_SUCCESS;


	// Reverse the path.
	dtNode* prev = 0;
	dtNode* node = lastBestNode;
	int n = 1;
	do
	{
		dtNode* next = m_nodePool->getNodeAtIdx(node->pidx);
		node->pidx = m_nodePool->getNodeIdx(prev);
		prev = node;
		node = next;
	} while (node && ++n < loopLimit);

	if (n >= loopLimit)
	{
		return DT_FAILURE | DT_INVALID_CYCLE_PATH;
	}

	// Store path
	float prevCost = 0.0f;

	node = prev;
	check(node);
	do
	{
		result.Add(node->id);
		prevCost = node->cost;

		node = m_nodePool->getNodeAtIdx(node->pidx);
	} while (node && --n > 0);

	m_nodePool->clear();
	m_openList->clear();
	//result.Add(endRef);
	return status;
}
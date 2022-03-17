// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Detour/DetourNode.h"
#include "NavMesh/RecastNavMesh.h"

typedef unsigned short dtNodeIndex;

class HIERARCHICALPATHFIND4UE_API hierarchicalNodePool
{
public:
	hierarchicalNodePool(int maxNodes, int hashSize);
	~hierarchicalNodePool();
	inline void operator=(const hierarchicalNodePool&) {}
	void clear();
	dtNode* getNode(dtPolyRef id);
	dtNode* findNode(dtPolyRef id);

	inline unsigned int getNodeIdx(const dtNode* node) const
	{
		if (!node) return 0;
		return (unsigned int)(node - m_nodes) + 1;
	}

	inline dtNode* getNodeAtIdx(unsigned int idx)
	{
		if (!idx) return 0;
		return &m_nodes[idx - 1];
	}

	inline const dtNode* getNodeAtIdx(unsigned int idx) const
	{
		if (!idx) return 0;
		return &m_nodes[idx - 1];
	}

	inline int getMemUsed() const
	{
		return sizeof(*this) +
			sizeof(dtNode) * m_maxNodes +
			sizeof(dtNodeIndex) * m_maxNodes +
			sizeof(dtNodeIndex) * m_hashSize;
	}

	inline int getMaxNodes() const { return m_maxNodes; }
	//@UE4 BEGIN
	// If using a shared query instance it's possible that m_maxNodes is greater
	// than pool size requested by callee. There's no point in reallocating the
	// pool so we artificially limit the number of available nodes
	inline int getMaxRuntimeNodes() const { return m_maxRuntimeNodes; }
	//@UE4 END
	inline int getNodeCount() const { return m_nodeCount; }

	inline int getHashSize() const { return m_hashSize; }
	inline dtNodeIndex getFirst(int bucket) const { return m_first[bucket]; }
	inline dtNodeIndex getNext(int i) const { return m_next[i]; }

	//@UE4 BEGIN
	// overrides m_maxNodes for runtime purposes
	inline void setMaxRuntimeNodes(const int newMaxRuntimeNodes) { m_maxRuntimeNodes = newMaxRuntimeNodes; }
	//@UE4 END

private:

	dtNode* m_nodes;
	dtNodeIndex* m_first;
	dtNodeIndex* m_next;
	const int m_maxNodes;
	const int m_hashSize;
	//@UE4 BEGIN
	int m_maxRuntimeNodes;
	//@UE4 END
	int m_nodeCount;
};

class HIERARCHICALPATHFIND4UE_API hierarchicalNodeQueue
{
public:
	hierarchicalNodeQueue(int n);
	~hierarchicalNodeQueue();
	inline void operator=(hierarchicalNodeQueue&) {}

	inline void clear()
	{
		m_size = 0;
	}

	inline dtNode* top()
	{
		return m_heap[0];
	}

	inline dtNode* pop()
	{
		dtNode* result = m_heap[0];
		m_size--;
		trickleDown(0, m_heap[m_size]);
		return result;
	}

	inline void push(dtNode* node)
	{
		m_size++;
		bubbleUp(m_size - 1, node);
	}

	inline void modify(dtNode* node)
	{
		for (int i = 0; i < m_size; ++i)
		{
			if (m_heap[i] == node)
			{
				bubbleUp(i, node);
				return;
			}
		}
	}

	inline bool empty() const { return m_size == 0; }

	inline int getMemUsed() const
	{
		return sizeof(*this) +
			sizeof(dtNode*) * (m_capacity + 1);
	}

	inline int getCapacity() const { return m_capacity; }

private:
	void bubbleUp(int i, dtNode* node);
	void trickleDown(int i, dtNode* node);

	dtNode** m_heap;
	const int m_capacity;
	int m_size;
};

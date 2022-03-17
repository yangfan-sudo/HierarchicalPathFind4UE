// Fill out your copyright notice in the Description page of Project Settings.


#include "HierarchicalNodePool.h"
#include "Detour/DetourCommon.h"
#include "Detour/DetourAssert.h"


inline unsigned int dtHashRef(dtPolyRef a)
{
	a += ~(a << 31);
	a ^= (a >> 20);
	a += (a << 6);
	a ^= (a >> 12);
	a += ~(a << 22);
	a ^= (a >> 32);
	return (unsigned int)a;
}


//////////////////////////////////////////////////////////////////////////////////////////
hierarchicalNodePool::hierarchicalNodePool(int maxNodes, int hashSize) :
	m_nodes(0),
	m_first(0),
	m_next(0),
	m_maxNodes(maxNodes),
	m_hashSize(hashSize),
	//@UE4 BEGIN
	m_maxRuntimeNodes(maxNodes),
	//@UE4 END
	m_nodeCount(0)
{
	dtAssert(dtNextPow2(m_hashSize) == (unsigned int)m_hashSize);
	dtAssert(m_maxNodes > 0);

	m_nodes = (dtNode*)dtAlloc(sizeof(dtNode) * m_maxNodes, DT_ALLOC_PERM);
	m_next = (dtNodeIndex*)dtAlloc(sizeof(dtNodeIndex) * m_maxNodes, DT_ALLOC_PERM);
	m_first = (dtNodeIndex*)dtAlloc(sizeof(dtNodeIndex) * hashSize, DT_ALLOC_PERM);

	dtAssert(m_nodes);
	dtAssert(m_next);
	dtAssert(m_first);

	memset(m_first, 0xff, sizeof(dtNodeIndex) * m_hashSize);
	memset(m_next, 0xff, sizeof(dtNodeIndex) * m_maxNodes);
}

hierarchicalNodePool::~hierarchicalNodePool()
{
	dtFree(m_nodes);
	dtFree(m_next);
	dtFree(m_first);
}

void hierarchicalNodePool::clear()
{
	memset(m_first, 0xff, sizeof(dtNodeIndex) * m_hashSize);
	m_nodeCount = 0;
}

dtNode* hierarchicalNodePool::findNode(dtPolyRef id)
{
	unsigned int bucket = dtHashRef(id) & (m_hashSize - 1);
	dtNodeIndex i = m_first[bucket];
	while (i != DT_NULL_IDX)
	{
		if (m_nodes[i].id == id)
			return &m_nodes[i];
		i = m_next[i];
	}
	return 0;
}

dtNode* hierarchicalNodePool::getNode(dtPolyRef id)
{
	unsigned int bucket = dtHashRef(id) & (m_hashSize - 1);
	dtNodeIndex i = m_first[bucket];
	dtNode* node = 0;
	while (i != DT_NULL_IDX)
	{
		if (m_nodes[i].id == id)
			return &m_nodes[i];
		i = m_next[i];
	}

	//@UE4 BEGIN
	if (m_nodeCount >= getMaxRuntimeNodes())
		//@UE4 END
		return 0;

	i = (dtNodeIndex)m_nodeCount;
	m_nodeCount++;

	// Init node
	node = &m_nodes[i];
	node->pidx = 0;
	node->cost = 0;
	node->total = 0;
	node->id = id;
	node->flags = 0;

	m_next[i] = m_first[bucket];
	m_first[bucket] = i;

	return node;
}


//////////////////////////////////////////////////////////////////////////////////////////
hierarchicalNodeQueue::hierarchicalNodeQueue(int n) :
	m_heap(0),
	m_capacity(n),
	m_size(0)
{
	dtAssert(m_capacity > 0);

	m_heap = (dtNode**)dtAlloc(sizeof(dtNode*) * (m_capacity + 1), DT_ALLOC_PERM);
	dtAssert(m_heap);
}

hierarchicalNodeQueue::~hierarchicalNodeQueue()
{
	dtFree(m_heap);
}

void hierarchicalNodeQueue::bubbleUp(int i, dtNode* node)
{
	int parent = (i - 1) / 2;
	// note: (index > 0) means there is a parent
	while ((i > 0) && (m_heap[parent]->total > node->total))
	{
		m_heap[i] = m_heap[parent];
		i = parent;
		parent = (i - 1) / 2;
	}
	m_heap[i] = node;
}

void hierarchicalNodeQueue::trickleDown(int i, dtNode* node)
{
	int child = (i * 2) + 1;
	while (child < m_size)
	{
		if (((child + 1) < m_size) &&
			(m_heap[child]->total > m_heap[child + 1]->total))
		{
			child++;
		}
		m_heap[i] = m_heap[child];
		i = child;
		child = (i * 2) + 1;
	}
	bubbleUp(i, node);
}

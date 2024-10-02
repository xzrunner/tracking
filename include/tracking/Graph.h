#pragma once

#include "OpType.h"

#include <vector>
#include <unordered_map>
#include <memory>

namespace tracking
{

class Node;
class OpNode;
class RegNode;

class Graph
{
public:
	Graph() {}
	~Graph();

	void AddOp(OpType type, const std::vector<int>& inputs, 
		const std::vector<int>& outputs);

	auto& GetAllOpNodes() const { return m_op_nodes; }
	auto& GetAllRegNodes() const { return m_reg_nodes; }

private:
	Node* NewNode(OpType type);
	Node* NewNode(int reg);

	static bool IsOutputNew(OpType type);

private:
	std::vector<OpNode*> m_op_nodes;
	std::vector<RegNode*> m_reg_nodes;

	std::unordered_map<int, Node*> m_registers;

}; // Graph

}
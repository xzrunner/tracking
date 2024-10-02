#include "tracking/Graph.h"
#include "tracking/OpNode.h"
#include "tracking/RegNode.h"

#include <assert.h>

namespace tracking
{

Graph::~Graph()
{
	for (auto node : m_op_nodes) {
		delete node;
	}
	for (auto node : m_reg_nodes) {
		delete node;
	}
}

void Graph::AddOp(OpType type, const std::vector<int>& inputs, 
	              const std::vector<int>& outputs)
{
	auto op_node = NewNode(type);

	for (auto reg : inputs)
	{
		Node* reg_node = nullptr;

		auto itr = m_registers.find(reg);
		if (itr == m_registers.end()) {
			reg_node = NewNode(reg);
		} else {
			reg_node = itr->second;
		}

		reg_node->Connect(op_node);
	}

	if (IsOutputNew(type))
	{
		for (auto reg : outputs)
		{
			Node* reg_node = NewNode(reg);

			auto itr = m_registers.find(reg);
			if (itr != m_registers.end()) {
				itr->second = reg_node;
			}

			op_node->Connect(reg_node);
		}
	}
	else
	{
		for (auto reg : outputs)
		{
			auto itr = m_registers.find(reg);
			assert(itr != m_registers.end());
			op_node->Connect(itr->second);
		}
	}
}

Node* Graph::NewNode(OpType type)
{
	OpNode* node = new OpNode(type);
	m_op_nodes.push_back(node);
	return node;
}

Node* Graph::NewNode(int reg)
{
	RegNode* node = new RegNode(reg);
	m_reg_nodes.push_back(node);
	m_registers.insert({ reg, node });
	return node;
}

bool Graph::IsOutputNew(OpType type)
{
	if (type == OpType::DRIVE) {
		return false;
	} else {
		return true;
	}
}

}
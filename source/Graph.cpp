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
	              const std::vector<int>& outputs, Node* op_node)
{
	bool merge_output = false;

	//// merge outputs
	//if (can_merge_output(type))
	//{
	//	auto node = QueryOpNode(type, inputs);
	//	if (node)
	//	{
	//		op_node = node;
	//		merge_output = true;
	//	}
	//}

	if (!merge_output)
	{
		if (!op_node) {
			op_node = NewNode(type);
		}

		for (auto reg : inputs)
		{
			if (!is_output_create(type) && std::find(outputs.begin(), outputs.end(), reg) != outputs.end()) {
				continue;
			}

			Node* reg_node = nullptr;

			auto itr = m_registers.find(reg);
			if (itr == m_registers.end()) {
				reg_node = NewNode(reg);
			} else {
				reg_node = itr->second;
			}

			reg_node->Connect(op_node);
		}
	}

	if (is_output_create(type))
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
			Node* reg_node = nullptr;

			auto itr = m_registers.find(reg);
			if (itr == m_registers.end()) {
				reg_node = NewNode(reg);
			} else {
				reg_node = itr->second;
			}

			op_node->Connect(reg_node);
		}
	}
}

void Graph::AddMergeSplitOp(const std::vector<std::pair<float, int>> inputs, int output)
{
	//assert(inputs.size() > 1);

	Node* merge_node = NewNode(OpType::MERGE);
	for (auto& input : inputs)
	{
		Node* input_node = nullptr;
		auto itr = m_registers.find(input.second);
		if (itr == m_registers.end()) {
			input_node = NewNode(input.second);
		} else {
			input_node = itr->second;
		}

		if (input.first < 1.0f)
		{
			Node* split_node = static_cast<RegNode*>(input_node)->QueryOpNode(false, OpType::SPLIT);
			if (!split_node)
			{
				split_node = NewNode(OpType::SPLIT);
				input_node->Connect(split_node);
			}
			assert(split_node);
			split_node->Connect(merge_node);
		}
		else
		{
			input_node->Connect(merge_node);
		}
	}

	Node* output_node = NewNode(output);
	auto itr = m_registers.find(output);
	if (itr != m_registers.end()) {
		itr->second = output_node;
	}
	merge_node->Connect(output_node);
}

Node* Graph::QueryRegNode(int id) const
{
	auto itr = m_registers.find(id);
	return itr == m_registers.end() ? nullptr : itr->second;
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

Node* Graph::QueryOpNode(OpType type, const std::vector<int>& inputs) const
{
	if (inputs.empty()) {
		return nullptr;
	}

	auto itr = m_registers.find(inputs[0]);
	if (itr == m_registers.end()) {
		return nullptr;
	}

	Node* op_node = static_cast<RegNode*>(itr->second)->QueryOpNode(false, type);
	if (!op_node) {
		return nullptr;
	}

	if (op_node->GetInputs().size() != inputs.size()) {
		return nullptr;
	}

	for (auto r : op_node->GetInputs())
	{
		bool find = false;
		for (auto i : inputs) 
		{
			if (static_cast<RegNode*>(r)->GetId() == i) {
				find = true;
				break;
			}
		}
		if (!find) {
			return nullptr;
		}
	}

	return op_node;
}

}
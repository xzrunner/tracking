#pragma once

#include "OpType.h"

#include <vector>

namespace tracking
{

class Node
{
public:
	Node(bool is_op);

	void Connect(Node* to);
	void Disconnect(Node* to);

	auto& GetInputs() const { return m_inputs; }
	auto& GetOutputs() const { return m_outputs; }

	bool IsOp() const { return m_is_op; }

private:
	std::vector<Node*> m_inputs, m_outputs;

	bool m_is_op = true;

}; // Node

}
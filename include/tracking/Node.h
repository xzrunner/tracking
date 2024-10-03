#pragma once

#include "OpType.h"

#include <vector>

namespace tracking
{

class Node
{
public:
	Node() {}

	void Connect(Node* to);
	void Disconnect(Node* to);

	auto& GetInputs() const { return m_inputs; }
	auto& GetOutputs() const { return m_outputs; }



private:
	std::vector<Node*> m_inputs, m_outputs;

}; // Node

}
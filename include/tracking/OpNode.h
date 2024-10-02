#pragma once

#include "Node.h"
#include "OpType.h"

namespace tracking
{

class OpNode : public Node
{
public:
	OpNode(OpType type);

	auto GetType() const { return m_type; }

private:
	OpType m_type;

}; // OpNode

}
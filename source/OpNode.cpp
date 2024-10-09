#include "tracking/OpNode.h"

namespace tracking
{

OpNode::OpNode(OpType type)
	: Node(true)
	, m_type(type)
{
}

}
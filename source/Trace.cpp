#include "tracking/Trace.h"

namespace tracking
{

Trace::Trace(RegNode* node, float weight, uint32_t type)
	: m_node(node)
	, m_weight(weight)
	, m_type(type)
{
}

}
#include "tracking/Trace.h"

namespace tracking
{

Trace::Trace(RegNode* node, bool is_evolve)
	: m_node(node)
	, m_is_evolve(is_evolve)
{
}

}
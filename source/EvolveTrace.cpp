#include "tracking/EvolveTrace.h"

namespace tracking
{

EvolveTrace::EvolveTrace(RegNode* node, float weight)
	: Trace(node, true)
	, m_weight(weight)
{
}

}
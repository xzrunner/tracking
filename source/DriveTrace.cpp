#include "tracking/DriveTrace.h"

namespace tracking
{

DriveTrace::DriveTrace(RegNode* node, uint32_t type)
	: Trace(node, false)
	, m_type(type)
{
}

}
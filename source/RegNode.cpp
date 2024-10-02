#include "tracking/RegNode.h"

namespace tracking
{

RegNode::RegNode(int reg)
	: m_id(reg)
{
}

void RegNode::ClearTraces()
{
	m_traces.clear();
}

void RegNode::CombineTraces(RegNode* parent, float weight)
{
	for (auto pt : parent->m_traces)
	{
		const float w = pt.GetWeight() * weight;
		
		bool find = false;
		for (auto& t : m_traces)
		{
			if (t.GetNode() == pt.GetNode())
			{
				t.AddWeight(w);
				find = true;
				break;
			}
		}

		if (!find) {
			m_traces.push_back(Trace(pt.GetNode(), w, pt.GetType()));
		}
	}
}

void RegNode::InitTrace()
{
	m_traces = { Trace(this, 1.0f)};
}

void RegNode::CopyTrace(RegNode* parent)
{
	for (auto t : parent->m_traces)
	{
		uint32_t type = t.GetType() | TRACE_TYPE_COPY_BIT;
		m_traces.push_back(Trace(t.GetNode(), t.GetWeight(), type));
	}
}

}
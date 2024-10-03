#include "tracking/RegNode.h"
#include "tracking/OpNode.h"
#include "tracking/Graph.h"

#include <iterator>

#include <assert.h>

namespace tracking
{

RegNode::RegNode(int reg)
	: m_id(reg)
{
}

bool RegNode::IsGraphInput() const
{
	for (auto op : GetInputs())
	{
		if (is_output_create(static_cast<OpNode*>(op)->GetType())) {
			return false;
		}
	}
	return true;
}

bool RegNode::IsGraphOutput() const
{
	if (IsGraphInput()) {
		return false;
	}
	for (auto op : GetOutputs())
	{
		if (is_derive_input(static_cast<OpNode*>(op)->GetType())) {
			return false;
		}
	}
	return true;
}

bool RegNode::IsNeedOutput() const
{
	for (auto op : GetInputs())
	{
		if (is_need_output(static_cast<OpNode*>(op)->GetType())) {
			return true;
		}
	}
	return false;
}

void RegNode::ClearTraces()
{
	m_traces.clear();
}

void RegNode::InitTrace()
{
	m_traces = { Trace(this, 1.0f) };
}

void RegNode::DeinitTrace()
{
	for (auto itr = m_traces.begin(); itr != m_traces.end(); ++itr)
	{
		if (itr->GetNode() == this)
		{
			assert(itr->GetWeight() == 1.0f);
			m_traces.erase(itr);
			break;
		}
	}
}

void RegNode::CombineTraces(RegNode* parent, float weight, RegNode* expect)
{
	for (auto pt : parent->m_traces)
	{
		if (pt.GetNode() != expect) {
			continue;
		}

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
			AddTrace(Trace(pt.GetNode(), w, pt.GetType()));
		}
	}
}

void RegNode::TransmitTrace(RegNode* parent, OpType type, RegNode* expect)
{
	TypeFlagBits flag;
	switch (type)
	{
	case OpType::COPY:
		flag = TRACE_TYPE_COPY_BIT;
		break;
	case OpType::DRIVE:
		flag = TRACE_TYPE_DRIVE_BIT;
		break;
	case OpType::DRIVE_CHANGE:
		flag = TRACE_TYPE_DRIVE_CHANGE_BIT;
		break;
	default:
		return;
	}

	for (auto t : parent->m_traces)
	{
		if (t.GetNode() == expect)
		{
			uint32_t type = t.GetType() | flag;
			AddTrace(Trace(t.GetNode(), t.GetWeight(), type));
		}
	}
}

Node* RegNode::QueryOpNode(bool input, OpType type) const
{
	auto& ops = input ? GetInputs() : GetOutputs();
	for (auto op : ops)
	{
		if (static_cast<OpNode*>(op)->GetType() == type) {
			return op;
		}
	}
	return nullptr;
}

void RegNode::AddTrace(const Trace& t)
{
	for (auto old_t : m_traces) 
	{
		if (old_t.GetNode() == t.GetNode()) {
			return;
		}
	}
	m_traces.push_back(t);
}

}
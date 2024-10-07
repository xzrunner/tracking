#include "tracking/RegNode.h"
#include "tracking/OpNode.h"
#include "tracking/Graph.h"
#include "tracking/EvolveTrace.h"
#include "tracking/DriveTrace.h"

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
		if (is_evolve_input(static_cast<OpNode*>(op)->GetType())) {
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
	m_traces = { std::make_unique<EvolveTrace>(this, 1.0f) };
}

void RegNode::DeinitTrace()
{
	for (auto itr = m_traces.begin(); itr != m_traces.end(); ++itr)
	{
		if ((*itr)->GetNode() == this && (*itr)->IsEvolve())
		{
			assert(std::static_pointer_cast<EvolveTrace>(*itr)->GetWeight() == 1.0f);
			m_traces.erase(itr);
			break;
		}
	}
}

void RegNode::TransmitEvolveTraces(RegNode* parent, float weight, RegNode* expect)
{
	for (auto pt : parent->m_traces)
	{
		if (pt->GetNode() != expect) {
			continue;
		}

		if (pt->IsEvolve())
		{
			const float w = std::static_pointer_cast<EvolveTrace>(pt)->GetWeight() * weight;
			AddEvolveTrace(pt->GetNode(), w);
		}
		else
		{
			const uint32_t t = std::static_pointer_cast<DriveTrace>(pt)->GetType();
			AddDriveTrace(pt->GetNode(), t);
		}
	}
}

void RegNode::TransmitDriveTraces(RegNode* parent, OpType type, RegNode* expect)
{
	DriveTraceTypeFlagBits flag;
	switch (type)
	{
	case OpType::COPY:
		flag = DTT_COPY_BIT;
		break;
	case OpType::DRIVE:
		flag = DTT_DRIVE_BIT;
		break;
	case OpType::DRIVE_CHANGE:
		flag = DTT_DRIVE_CHANGE_BIT;
		break;
	default:
		return;
	}

	for (auto pt : parent->m_traces)
	{
		if (pt->GetNode() != expect) {
			continue;
		}

		if (pt->IsEvolve())
		{
			AddDriveTrace(pt->GetNode(), flag);
		}
		else
		{
			const uint32_t t = std::static_pointer_cast<DriveTrace>(pt)->GetType() | flag;
			AddDriveTrace(pt->GetNode(), t);
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

void RegNode::AddEvolveTrace(RegNode* node, float weight)
{
	for (auto& t : m_traces)
	{
		if (t->GetNode() == node && t->IsEvolve())
		{
			std::static_pointer_cast<EvolveTrace>(t)->AddWeight(weight);
			return;
		}
	}
	m_traces.push_back(std::make_shared<EvolveTrace>(node, weight));
}

void RegNode::AddDriveTrace(RegNode* node, uint32_t type)
{
	for (auto& t : m_traces)
	{
		if (t->GetNode() == node && !t->IsEvolve())
		{
			std::static_pointer_cast<DriveTrace>(t)->AddType(type);
			return;
		}
	}
	m_traces.push_back(std::make_shared<DriveTrace>(node, type));
}

}
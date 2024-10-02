#pragma once

#include "Node.h"
#include "Trace.h"

namespace tracking
{

class RegNode : public Node
{
public:
	RegNode(int reg);

	void ClearTraces();
	void CombineTraces(RegNode* parent, float weight);
	auto& GetTraces() const { return m_traces; }

	void InitTrace();
	void CopyTrace(RegNode* parent);

	int GetId() const { return m_id; }

private:
	int m_id;

	std::vector<Trace> m_traces;

}; // RegNode

}
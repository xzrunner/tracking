#pragma once

#include "Node.h"
#include "Trace.h"

namespace tracking
{

class RegNode : public Node
{
public:
	RegNode(int reg);

	bool IsGraphInput() const;
	bool IsGraphOutput() const;
	bool IsNeedOutput() const;

	void ClearTraces();
	auto& GetTraces() const { return m_traces; }

	void InitTrace();
	void DeinitTrace();

	void CombineTraces(RegNode* parent, float weight, RegNode* expect);
	void TransmitTrace(RegNode* parent, OpType type, RegNode* expect);

	int GetId() const { return m_id; }

	Node* QueryOpNode(bool input, OpType type) const;

private:
	void AddTrace(const Trace& t);

private:
	int m_id;

	std::vector<Trace> m_traces;

}; // RegNode

}
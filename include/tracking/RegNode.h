#pragma once

#include "Node.h"

#include <memory>

namespace tracking
{

class Trace;

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

	void TransmitEvolveTraces(RegNode* parent, float weight, RegNode* expect);
	void TransmitDriveTraces(RegNode* parent, OpType type, RegNode* expect);

	int GetId() const { return m_id; }

	Node* QueryOpNode(bool input, OpType type) const;

private:
	void AddEvolveTrace(RegNode* node, float weight);
	void AddDriveTrace(RegNode* node, uint32_t type);

private:
	int m_id;

	std::vector<std::shared_ptr<Trace>> m_traces;

}; // RegNode

}
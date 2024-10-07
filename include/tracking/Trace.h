#pragma once

namespace tracking
{

class RegNode;

class Trace
{
public:
	Trace(RegNode* node, bool is_evolve);

	auto GetNode() const { return m_node; }

	bool IsEvolve() const { return m_is_evolve; }

private:
	RegNode* m_node = nullptr;
	
	bool m_is_evolve = true;

}; // Trace

}
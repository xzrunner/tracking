#pragma once

#include <stdint.h>

namespace tracking
{

class RegNode;

enum TypeFlagBits
{
	TRACE_TYPE_COPY_BIT = 0x0001,
	TRACE_TYPE_DRIVE_BIT = 0x0002
};

class Trace
{
public:
	Trace(RegNode* node, float weight, uint32_t type = 0);

	auto GetNode() const { return m_node; }
	auto GetWeight() const { return m_weight; }
	auto GetType() const { return m_type; }

	void AddWeight(float w) { m_weight += w; }

private:
	RegNode* m_node;
	float    m_weight;
	uint32_t m_type;

}; // Trace

}
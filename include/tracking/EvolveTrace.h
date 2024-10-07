#pragma once

#include "tracking/Trace.h"

namespace tracking
{

class EvolveTrace : public Trace
{
public:
	EvolveTrace(RegNode* node, float weight);

	float GetWeight() const { return m_weight; }

	void AddWeight(float w) { m_weight += w; }

private:
	float m_weight = 0.0f;

}; // EvolveTrace

}
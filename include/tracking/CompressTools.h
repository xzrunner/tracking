#pragma once

#include <vector>

namespace tracking
{

class RegNode;

class CompressTools
{
public:
	static void build_traces(const std::vector<tracking::RegNode*>& roots);

}; // CompressTools

}
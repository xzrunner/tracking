#pragma once

#include "OpType.h"

#include <vector>
#include <memory>

namespace tracking
{

class Graph;

class GraphSerializer
{
public:
	struct OpItem
	{
		bool operator == (const OpItem& op) const;

		OpType type;
		std::vector<int> inputs;
		std::vector<int> outputs;
	};

	static std::shared_ptr<Graph> Build(const std::vector<OpItem>& ops);
	static std::vector<OpItem> Dump(const Graph& graph);

}; // GraphSerializer

}
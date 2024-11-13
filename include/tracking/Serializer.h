#pragma once

#include "OpType.h"

#include <vector>
#include <memory>

namespace tracking
{

class Graph;

class Serializer
{
public:
	struct OpItem
	{
		bool operator == (const OpItem& op) const;

		OpType type;
		std::vector<int> inputs;
		std::vector<int> outputs;
	};

	static std::shared_ptr<Graph> Build(const std::vector<OpItem>& items);

	static std::vector<OpItem> DumpDirectly(const Graph& graph);
#ifdef USE_DERIVE_CREATE_TYPE
	static std::vector<OpItem> DumpSimplify(const Graph& graph);
#endif // USE_DERIVE_CREATE_TYPE

}; // Serializer

}
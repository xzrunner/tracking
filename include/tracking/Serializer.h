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
	enum class OutputType
	{
		CREATE,
		DELETE,

		SPLIT,
		MERGE,

		DERIVE,
		DERIVE_CREATE,

		DRIVE_CHANGE,
	};

	struct OutputItem
	{
		bool operator == (const OutputItem& op) const;

		OutputType type;
		std::vector<int> inputs;
		std::vector<int> outputs;
	};

	struct InputItem
	{
		OpType type;
		std::vector<int> inputs;
		std::vector<int> outputs;
	};

	static std::shared_ptr<Graph> Build(const std::vector<InputItem>& items);
	static std::vector<OutputItem> Dump(const Graph& graph);

}; // Serializer

}
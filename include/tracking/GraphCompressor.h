#pragma once

#include <memory>

namespace tracking
{

class Graph;

class GraphCompressor
{
public:
	static std::shared_ptr<Graph> Compress(const Graph& graph);

}; // GraphCompressor

}
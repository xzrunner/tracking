#pragma once

#include <memory>

namespace tracking
{

class Graph;

class Compressor
{
public:
	static std::shared_ptr<Graph> Compress(const Graph& graph, bool simplify = true);

}; // Compressor

}
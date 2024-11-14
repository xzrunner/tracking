#pragma once

#include <memory>

namespace tracking
{

class Graph;

class Compressor
{
public:
	enum class Strategy
	{
		EvolveAndDrive,
		OnlyEvolve,
		OnlyDrive
	};

	static std::shared_ptr<Graph> Compress(const Graph& graph, 
		Strategy strategy = Strategy::EvolveAndDrive, bool simplify = false);

}; // Compressor

}
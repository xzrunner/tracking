#include "tracking/Serializer.h"
#include "tracking/Graph.h"
#include "tracking/OpNode.h"
#include "tracking/RegNode.h"

namespace tracking
{

bool Serializer::OpItem::operator == (const OpItem& op) const
{
	return type == op.type && inputs == op.inputs && outputs == op.outputs;
}

std::shared_ptr<Graph> Serializer::Build(const std::vector<OpItem>& ops)
{
	std::shared_ptr<Graph> graph = std::make_shared<Graph>();
	for (auto op : ops) {
		graph->AddOp(op.type, op.inputs, op.outputs);
	}
	return graph;
}

std::vector<Serializer::OpItem> Serializer::Dump(const Graph& graph)
{
	std::vector<Serializer::OpItem> ops;
	for (auto op : graph.GetAllOpNodes())
	{
		std::vector<int> inputs, outputs;
		for (auto r : op->GetInputs()) {
			inputs.push_back(static_cast<tracking::RegNode*>(r)->GetId());
		}
		for (auto r : op->GetOutputs()) {
			outputs.push_back(static_cast<tracking::RegNode*>(r)->GetId());
		}
		ops.push_back({ op->GetType(), inputs, outputs });
	}
	return ops;
}

}
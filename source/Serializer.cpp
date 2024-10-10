#include "tracking/Serializer.h"
#include "tracking/Graph.h"
#include "tracking/OpNode.h"
#include "tracking/RegNode.h"

#include <set>
#include <iterator>

#include <assert.h>

namespace tracking
{

bool Serializer::OutputItem::operator == (const OutputItem& op) const
{
	return type == op.type && inputs == op.inputs && outputs == op.outputs;
}

std::shared_ptr<Graph> Serializer::Build(const std::vector<InputItem>& items)
{
	std::shared_ptr<Graph> graph = std::make_shared<Graph>();
	for (auto item : items) {
		graph->AddOp(item.type, item.inputs, item.outputs);
	}
	return graph;
}

std::vector<Serializer::OutputItem> Serializer::Dump(const Graph& graph)
{
	std::vector<Serializer::OutputItem> items;

	std::set<RegNode*> derive_inputs;
	for (auto op : graph.GetAllOpNodes())
	{
		if (op->GetType() == OpType::SPLIT)
		{
			std::vector<int> outputs;
			for (auto output : op->GetOutputs())
			{
				if (output->IsOp())
				{
					assert(static_cast<OpNode*>(output)->GetType() == OpType::MERGE);
				}
				else
				{
					outputs.push_back(static_cast<RegNode*>(output)->GetId());
				}
			}

			if (!outputs.empty())
			{
				assert(op->GetInputs().size() == 1);
				std::vector<int> inputs = { static_cast<RegNode*>(op->GetInputs()[0])->GetId() };
				items.push_back({ OutputType::SPLIT, inputs, outputs });
			}

			continue;
		}
		else if (op->GetType() == OpType::MERGE)
		{
			bool is_derive = false;
			for (auto input : op->GetInputs())
			{
				if (input->IsOp())
				{
					auto op_node = static_cast<OpNode*>(input);
					assert(op_node->GetType() == OpType::SPLIT);
					is_derive = true;
					break;
				}
			}

			if (is_derive)
			{
				std::vector<int> inputs;
				for (auto input : op->GetInputs())
				{
					if (input->IsOp())
					{
						auto op_node = static_cast<OpNode*>(input);
						assert(op_node->GetType() == OpType::SPLIT);
						assert(input->GetInputs().size() == 1 && !input->GetInputs()[0]->IsOp());
						inputs.push_back(static_cast<RegNode*>(input->GetInputs()[0])->GetId());
						derive_inputs.insert(static_cast<RegNode*>(input->GetInputs()[0]));
					}
					else
					{
						inputs.push_back(static_cast<RegNode*>(input)->GetId());
						derive_inputs.insert(static_cast<RegNode*>(input));
					}
				}

				if (!inputs.empty())
				{
					assert(op->GetOutputs().size() == 1);
					std::vector<int> outputs = { static_cast<RegNode*>(op->GetOutputs()[0])->GetId() };
					items.push_back({ OutputType::DERIVE, inputs, outputs });
				}

				continue;
			}
		}

		OutputType type;
		switch (op->GetType())
		{
		case OpType::CREATE:
			type = OutputType::CREATE;
			break;
		case OpType::DELETE:
			type = OutputType::DELETE;
			break;
		case OpType::SPLIT:
			type = OutputType::SPLIT;
			break;
		case OpType::MERGE:
			type = OutputType::MERGE;
			break;
		case OpType::DRIVE_CHANGE:
			type = OutputType::DRIVE_CHANGE;
			break;
		default:
			assert(0);
		}

		std::vector<int> inputs, outputs;
		for (auto r : op->GetInputs()) {
			inputs.push_back(static_cast<RegNode*>(r)->GetId());
		}
		for (auto r : op->GetOutputs()) {
			outputs.push_back(static_cast<RegNode*>(r)->GetId());
		}
		items.push_back({ type, inputs, outputs });
	}

	// delete
	for (auto input : derive_inputs)
	{
		bool need_delete = true;
		for (auto output : input->GetOutputs())
		{
			if (output->IsOp() && is_input_delete(static_cast<OpNode*>(output)->GetType()))
			{
				need_delete = false;
				break;
			}
		}

		if (need_delete) 
		{
			items.push_back({ OutputType::DELETE, { static_cast<RegNode*>(input)->GetId() }, {}});
		}
	}

	return items;
}

}
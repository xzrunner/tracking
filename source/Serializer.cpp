#include "tracking/Serializer.h"
#include "tracking/Graph.h"
#include "tracking/OpNode.h"
#include "tracking/RegNode.h"

#include <set>
#include <iterator>

#include <assert.h>

namespace
{

void dump_op(tracking::OpNode* op, std::vector<tracking::Serializer::OpItem>& items)
{
	std::vector<int> inputs, outputs;
	for (auto r : op->GetInputs()) {
		inputs.push_back(static_cast<tracking::RegNode*>(r)->GetId());
	}
	for (auto r : op->GetOutputs()) {
		outputs.push_back(static_cast<tracking::RegNode*>(r)->GetId());
	}
	items.push_back({ op->GetType(), inputs, outputs});
}

}

namespace tracking
{

bool Serializer::OpItem::operator == (const OpItem& op) const
{
	return type == op.type && inputs == op.inputs && outputs == op.outputs;
}

std::shared_ptr<Graph> Serializer::Build(const std::vector<OpItem>& items)
{
	std::shared_ptr<Graph> graph = std::make_shared<Graph>();
	for (auto item : items) {
		graph->AddOp(item.type, item.inputs, item.outputs);
	}
	return graph;
}

std::vector<Serializer::OpItem> Serializer::DumpDirectly(const Graph& graph)
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

#ifdef USE_DERIVE_CREATE_TYPE
std::vector<Serializer::OpItem> Serializer::DumpSimplify(const Graph& graph)
{
	std::vector<Serializer::OpItem> items;

	std::set<RegNode*> derive_inputs;
	for (auto op : graph.GetAllOpNodes())
	{
		switch (op->GetType())
		{
		case OpType::SPLIT:
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
				items.push_back({ OpType::SPLIT, inputs, outputs });
			}
		}
			break;
		case OpType::MERGE:
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
					items.push_back({ OpType::DERIVE, inputs, outputs });
				}
			}
		}
			break;
		case OpType::CREATE:
		{
			// derive_create
			assert(op->GetOutputs().size() == 1);
			auto drive_node = static_cast<RegNode*>(op->GetOutputs()[0])->QueryOpNode(true, OpType::DRIVE);
			if (drive_node)
			{
				std::vector<int> inputs, outputs;
				for (auto r : drive_node->GetInputs()) {
					inputs.push_back(static_cast<tracking::RegNode*>(r)->GetId());
				}
				for (auto r : op->GetOutputs()) {
					outputs.push_back(static_cast<tracking::RegNode*>(r)->GetId());
				}
				items.push_back({ OpType::DERIVE_CREATE, inputs, outputs });
			}
			else
			{
				dump_op(op, items);
			}
		}
			
			break;
		case OpType::DRIVE:
			break;
		default:
			dump_op(op, items);
		}
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
			items.push_back({ OpType::DELETE, { static_cast<RegNode*>(input)->GetId() }, {}});
		}
	}

	return items;
}
#endif // USE_DERIVE_CREATE_TYPE

}
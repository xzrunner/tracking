#include "tracking/Compressor.h"
#include "tracking/Graph.h"
#include "tracking/RegNode.h"
#include "tracking/OpNode.h"
#include "tracking/EvolveTrace.h"
#include "tracking/DriveTrace.h"

#include <vector>
#include <queue>
#include <map>
#include <set>

#include <assert.h>

namespace
{

void build_traces(const std::vector<tracking::RegNode*>& roots)
{
	// init root traces
	for (auto n : roots) {
		n->InitTrace();
	}

	for (auto root : roots)
	{
		std::queue<tracking::RegNode*> buf;
		buf.push(root);
		while (!buf.empty())
		{
			tracking::RegNode* reg = buf.front();
			buf.pop();

			auto& reg_outputs = reg->GetOutputs();
			if (reg_outputs.empty()) {
				continue;
			}

			for (auto op : reg_outputs)
			{
				auto& op_outputs = op->GetOutputs();
				tracking::OpType op_type = static_cast<tracking::OpNode*>(op)->GetType();
				if (op_type == tracking::OpType::SPLIT)
				{
					for (auto c : op_outputs)
					{
						const float w = 1.0f / op_outputs.size();
						static_cast<tracking::RegNode*>(c)->TransmitEvolveTraces(reg, w, root);
					}
				}
				else if (op_type == tracking::OpType::MERGE)
				{
					assert(op_outputs.size() == 1);
					static_cast<tracking::RegNode*>(op_outputs[0])->TransmitEvolveTraces(reg, 1.0f, root);
				}
				else if (tracking::need_transmit_trace(op_type))
				{
					assert(op_outputs.size() == 1);
					static_cast<tracking::RegNode*>(op_outputs[0])->TransmitDriveTraces(reg, op_type, root);
				}

				for (auto n : op_outputs) {
					buf.push(static_cast<tracking::RegNode*>(n));
				}
			}
		}
	}

	// clear init trace
	for (auto n : roots) {
		n->DeinitTrace();
	}
}

void compress_with_output(tracking::Graph& g, const std::set<int> input_ids, 
	                      const std::vector<tracking::RegNode*>& outputs, 
	                      const std::vector<tracking::RegNode*>& others)
{
	std::map<tracking::RegNode*, std::vector<tracking::RegNode*>> split_collect;
	for (auto n : outputs)
	{
		auto& traces = n->GetTraces();
		if (traces.empty())
		{
			// create
			std::vector<int> input;
			auto copy_op = n->QueryOpNode(true, tracking::OpType::COPY);
			if (copy_op)
			{
				assert(copy_op->GetInputs().size() == 1);
				input.push_back(static_cast<tracking::RegNode*>(copy_op->GetInputs()[0])->GetId());
			}
			g.AddOp(tracking::OpType::CREATE, input, { n->GetId()});
		}
		else if (traces.size() == 1)
		{
			auto& t = traces[0];
			auto t_node = t->GetNode();
			// split
			if (t->IsEvolve())
			{
				auto itr = split_collect.find(t_node);
				if (itr == split_collect.end()) {
					split_collect.insert({ t_node, {n} });
				} else {
					itr->second.push_back(n);
				}
			}
			// copy or derive_create
			else
			{
				auto copy_op = n->QueryOpNode(true, tracking::OpType::COPY);
				if (copy_op && copy_op->GetInputs()[0] == t_node) {
					g.AddOp(tracking::OpType::CREATE, { t_node->GetId() }, { n->GetId() });
				} else {
					g.AddOp(tracking::OpType::DERIVE_CREATE, { t_node->GetId() }, { n->GetId() });
				}
			}
		}
		else
		{
			// merge
			std::vector<int> inputs, inputs_only_evolve, inputs_only_drive, _outputs;

			_outputs.push_back(n->GetId());

			bool not_merge = false;
			for (auto& t : n->GetTraces())
			{
				int t_id = t->GetNode()->GetId();
				inputs.push_back(t_id);
				if (t->IsEvolve())
				{
					inputs_only_evolve.push_back(t_id);
					if (std::static_pointer_cast<tracking::EvolveTrace>(t)->GetWeight() < 1)
					{
						not_merge = true;
					}
				}
				else 
				{
					inputs_only_drive.push_back(t_id);
				}
			}

			if (!not_merge && inputs.size() == inputs_only_evolve.size()) {
				g.AddOp(tracking::OpType::MERGE, inputs, _outputs);
			} else if (input_ids.find(n->GetId()) == input_ids.end()) {
				g.AddOp(tracking::OpType::DERIVE_CREATE, inputs, _outputs);
			} else {
				g.AddOp(tracking::OpType::DERIVE, inputs_only_evolve, _outputs);
			}
		}
	}

	// split
	for (auto pair : split_collect)
	{
		std::vector<int> inputs, _outputs, outputs_only_evolve;

		inputs.push_back(pair.first->GetId());
		for (auto o : pair.second) 
		{
			_outputs.push_back(o->GetId());

			bool is_evolve = true;
			for (auto& t : o->GetTraces())
			{
				if (!t->IsEvolve())
				{
					is_evolve = false;
					break;
				}
			}
			if (is_evolve) {
				outputs_only_evolve.push_back(o->GetId());
			}
		}

		if (_outputs.size() == outputs_only_evolve.size()) {
			g.AddOp(tracking::OpType::SPLIT, inputs, _outputs);
		} else if (outputs_only_evolve.empty()) {
			g.AddOp(tracking::OpType::DERIVE_CREATE, inputs, _outputs);
		} else {
			g.AddOp(tracking::OpType::SPLIT, inputs, outputs_only_evolve);
		}
	}

	for (auto reg : others)
	{
		if (reg->QueryOpNode(false, tracking::OpType::DELETE))
		{
			g.AddOp(tracking::OpType::DELETE, { reg->GetId() }, {});
		}
		else
		{
			// drive_change
			std::vector<int> inputs;
			for (auto& t : reg->GetTraces())
			{
				if (!t->IsEvolve())
				{
					auto dt = std::static_pointer_cast<tracking::DriveTrace>(t);
					if (dt->GetType() & tracking::DTT_DRIVE_CHANGE_BIT)
					{
						inputs.push_back(t->GetNode()->GetId());
					}
				}
			}
			if (!inputs.empty()) {
				g.AddOp(tracking::OpType::DRIVE_CHANGE, inputs, { reg->GetId() });
			}
		}
	}
}

void compress_with_input(tracking::Graph& g, const std::vector<tracking::RegNode*>& inputs, 
	                     const std::set<int>& output_ids)
{
	auto is_del = [] (const tracking::RegNode* r) -> bool
	{
		for (auto op : r->GetOutputs())
		{
			if (is_input_delete(static_cast<tracking::OpNode*>(op)->GetType())) {
				return true;
			}
		}
		return false;
	};

	std::set<int> new_input_del;
	for (auto r : g.GetAllRegNodes())
	{
		if (!r->IsGraphInput()) {
			continue;
		}
		if (is_del(r)) {
			new_input_del.insert(r->GetId());
		}
	}

	for (auto r : inputs)
	{
		if (output_ids.find(r->GetId()) != output_ids.end()) {
			continue;
		}
		if (is_del(r) && new_input_del.find(r->GetId()) == new_input_del.end()) {
			g.AddOp(tracking::OpType::DELETE, { r->GetId() }, {});
		}
	}
}

}

namespace tracking
{

std::shared_ptr<Graph> Compressor::Compress(const Graph& src)
{
	std::vector<RegNode*> input, output, need_output;
	std::set<int> input_ids, output_ids;
	for (auto n : src.GetAllRegNodes())
	{
		if (n->IsGraphOutput()) 
		{
			output.push_back(n);
			output_ids.insert(n->GetId());
		}
		else
		{
			if (n->IsNeedOutput()) {
				need_output.push_back(n);
			}
			if (n->IsGraphInput()) 
			{
				input.push_back(n);
				input_ids.insert(n->GetId());
			}
		}
	}

	// gen traces
	for (auto n : src.GetAllRegNodes()) {
		n->ClearTraces();
	}
	build_traces(input);

	auto dst = std::make_shared<Graph>();

	// analyze output
	compress_with_output(*dst, input_ids, output, need_output);

	// analyze input
	compress_with_input(*dst, input, output_ids);

	return dst;
}

}
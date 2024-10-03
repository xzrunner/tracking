#include "tracking/GraphCompressor.h"
#include "tracking/Graph.h"
#include "tracking/RegNode.h"
#include "tracking/OpNode.h"

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

			assert(reg_outputs.size() == 1);
			tracking::Node* op = reg_outputs[0];
			auto& op_outputs = op->GetOutputs();
			tracking::OpType op_type = static_cast<tracking::OpNode*>(op)->GetType();
			switch (op_type)
			{
			case tracking::OpType::SPLIT:
			{
				for (auto c : op_outputs)
				{
					const float w = 1.0f / op_outputs.size();
					static_cast<tracking::RegNode*>(c)->CombineTraces(reg, w, root);
				}
			}
				break;
			case tracking::OpType::MERGE:
			{
				assert(op_outputs.size() == 1);
				static_cast<tracking::RegNode*>(op_outputs[0])->CombineTraces(reg, 1.0f, root);
			}
				break;
			case tracking::OpType::COPY:
			case tracking::OpType::DRIVE:
			case tracking::OpType::DRIVE_CHANGE:
			{
				assert(op_outputs.size() == 1);
				static_cast<tracking::RegNode*>(op_outputs[0])->TransmitTrace(reg, op_type, root);
			}
				break;
			}

			for (auto n : op_outputs) {
				buf.push(static_cast<tracking::RegNode*>(n));
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
			// split
			if (t.GetType() == 0)
			{
				auto itr = split_collect.find(t.GetNode());
				if (itr == split_collect.end()) {
					split_collect.insert({ t.GetNode(), {n} });
				} else {
					itr->second.push_back(n);
				}
			}
			// copy or derive_create
			else
			{
				auto copy_op = n->QueryOpNode(true, tracking::OpType::COPY);
				if (copy_op && copy_op->GetInputs()[0] == t.GetNode()) {
					g.AddOp(tracking::OpType::CREATE, { t.GetNode()->GetId() }, { n->GetId() });
				} else {
					g.AddOp(tracking::OpType::DERIVE_CREATE, { t.GetNode()->GetId() }, { n->GetId() });
				}
			}
		}
		else
		{
			// merge
			std::vector<int> inputs, inputs_only_derive, inputs_only_drive, outputs;

			outputs.push_back(n->GetId());

			bool not_merge = false;
			for (auto& t : n->GetTraces())
			{
				int t_id = t.GetNode()->GetId();
				inputs.push_back(t_id);
				if (t.GetType() == 0) {
					inputs_only_derive.push_back(t_id);
				} else {
					inputs_only_drive.push_back(t_id);
				}
				if (t.GetWeight() < 1) {
					not_merge = true;
				}
			}

			if (!not_merge && inputs.size() == inputs_only_derive.size()) {
				g.AddOp(tracking::OpType::MERGE, inputs, outputs);
			} else if (input_ids.find(n->GetId()) == input_ids.end()) {
				g.AddOp(tracking::OpType::DERIVE_CREATE, inputs, outputs);
			} else {
				g.AddOp(tracking::OpType::DERIVE, inputs_only_derive, outputs);
			}
		}
	}

	// split
	for (auto pair : split_collect)
	{
		std::vector<int> inputs, outputs, outputs_only_derive;

		inputs.push_back(pair.first->GetId());
		for (auto o : pair.second) 
		{
			outputs.push_back(o->GetId());

			bool is_derive = true;
			for (auto& t : o->GetTraces())
			{
				if (t.GetType() > 0)
				{
					is_derive = false;
					break;
				}
			}
			if (is_derive) {
				outputs_only_derive.push_back(o->GetId());
			}
		}

		if (outputs.size() == outputs_only_derive.size()) {
			g.AddOp(tracking::OpType::SPLIT, inputs, outputs);
		} else if (outputs_only_derive.empty()) {
			g.AddOp(tracking::OpType::DERIVE_CREATE, inputs, outputs);
		} else {
			g.AddOp(tracking::OpType::SPLIT, inputs, outputs_only_derive);
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
				if (t.GetType() & tracking::TRACE_TYPE_DRIVE_CHANGE_BIT) {
					inputs.push_back(t.GetNode()->GetId());
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

std::shared_ptr<Graph> GraphCompressor::Compress(const Graph& src)
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
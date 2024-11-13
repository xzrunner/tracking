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

void compress_merge(tracking::Graph& g, tracking::RegNode* node, bool simplify)
{
	assert(node->GetTraces().size() > 1);

	std::vector<std::pair<uint32_t, tracking::RegNode*>> drive_add_inputs, drive_mod_inputs;
	std::vector<std::pair<float, int>> evolve_inputs;
	for (auto& t : node->GetTraces())
	{
		if (t->IsEvolve())
		{
			const float w = std::static_pointer_cast<tracking::EvolveTrace>(t)->GetWeight();
			evolve_inputs.push_back({ w, t->GetNode()->GetId() });
		}
		else
		{
			const uint32_t type = std::static_pointer_cast<tracking::DriveTrace>(t)->GetType();
			if (type & tracking::DTT_DRIVE_CHANGE_BIT)
				drive_mod_inputs.push_back({ type, t->GetNode() });
			else
				drive_add_inputs.push_back({ type, t->GetNode() });
		}
	}

	if (!evolve_inputs.empty())
	{
		std::sort(evolve_inputs.begin(), evolve_inputs.end(),
			[](const std::pair<float, int>& p0, const std::pair<float, int>& p1)
		{
			return p0.first > p1.first;
		});
	}

	auto geo_evolve_op = [&evolve_inputs, &g, &node]()
	{
		assert(!evolve_inputs.empty());
#ifdef USE_DERIVE_TYPE
		bool is_merge = true;
		std::vector<int> inputs;
		for (auto p : evolve_inputs)
		{
			if (p.first != 1.0f)
				is_merge = false;
			inputs.push_back(p.second);
		}
		tracking::OpType type = is_merge ? tracking::OpType::MERGE : tracking::OpType::DERIVE;
		g.AddOp(type, inputs, { node->GetId() });
#else
		if (evolve_inputs.back().first < 1.0f)
		{
			g.AddMergeSplitOp(evolve_inputs, node->GetId());
		}
		else
		{
			std::vector<int> inputs;
			for (auto p : evolve_inputs) {
				inputs.push_back(p.second);
			}
			g.AddOp(tracking::OpType::MERGE, inputs, { node->GetId() });
		}
#endif // USE_DERIVE_TYPE
	};

	if (!evolve_inputs.empty() && (!drive_add_inputs.empty() || !drive_mod_inputs.empty()))
	{
		if (!drive_add_inputs.empty())
		{
			std::vector<int> inputs;
			for (auto p : evolve_inputs) {
				inputs.push_back(p.second);
			}
			for (auto& pair : drive_add_inputs) {
				inputs.push_back(pair.second->GetId());
			}
			g.AddOp(tracking::OpType::CREATE, inputs, { node->GetId() });
			g.AddOp(tracking::OpType::DRIVE, inputs, { node->GetId() });
		}
		else
		{
			geo_evolve_op();

			assert(!drive_mod_inputs.empty());
			std::vector<int> inputs;
			for (auto& pair : drive_mod_inputs) {
				inputs.push_back(pair.second->GetId());
			}
			g.AddOp(tracking::OpType::DRIVE_CHANGE, inputs, { node->GetId() });
		}
	}
	else if (!evolve_inputs.empty())
	{
		geo_evolve_op();
	}
	else if (!drive_add_inputs.empty() || !drive_mod_inputs.empty())
	{
		std::vector<int> cp_inputs, d_inputs, dc_inputs;
		for (auto& pair : drive_add_inputs)
		{
			if (pair.first & tracking::DTT_COPY_BIT) {
				cp_inputs.push_back(pair.second->GetId());
			}
			if (pair.first & tracking::DTT_DRIVE_BIT) {
				d_inputs.push_back(pair.second->GetId());
			}
		}
		for (auto& pair : drive_mod_inputs)
		{
			if (pair.first & tracking::DTT_DRIVE_CHANGE_BIT) {
				dc_inputs.push_back(pair.second->GetId());
			}
		}

		// copy
		if (evolve_inputs.empty())
		{
			if (cp_inputs.empty()) 
			{
				g.AddOp(tracking::OpType::CREATE, {}, { node->GetId() });
			} 
			else 
			{
				g.AddOp(tracking::OpType::CREATE, cp_inputs, { node->GetId() });
				g.AddOp(tracking::OpType::DRIVE, cp_inputs, { node->GetId() });
			}
		}
		else
		{
			for (auto cp : cp_inputs) {
				g.AddOp(tracking::OpType::CREATE, { cp }, { node->GetId() });
			}
		}

		// drive
		if (!d_inputs.empty()) {
			g.AddOp(tracking::OpType::DRIVE, d_inputs, { node->GetId() });
		}

		// drive_change
		if (!dc_inputs.empty()) {
			g.AddOp(tracking::OpType::DRIVE_CHANGE, dc_inputs, { node->GetId() });
		}
	}
}

void compress_split(tracking::Graph& g, const tracking::RegNode* parent, 
	                const std::vector<std::pair<tracking::RegNode*, std::shared_ptr<tracking::Trace>>>& children)
{
	std::vector<std::pair<uint32_t, tracking::RegNode*>> drive_outputs;
	std::vector<std::pair<float, tracking::RegNode*>> evolve_outputs;
	for (auto& pair : children)
	{
		auto c_node = pair.first;
		auto c_trace = pair.second;
		if (c_trace->IsEvolve())
		{
			const float w = std::static_pointer_cast<tracking::EvolveTrace>(c_trace)->GetWeight();
			evolve_outputs.push_back({ w, c_node });
		}
		else
		{
			const uint32_t type = std::static_pointer_cast<tracking::DriveTrace>(c_trace)->GetType();
			drive_outputs.push_back({ type, c_node });
		}
	}

	if (!evolve_outputs.empty())
	{
		std::vector<int> outputs;
		for (auto& pair : evolve_outputs) {
			outputs.push_back(pair.second->GetId());
		}

		tracking::Node* op_node = nullptr;
		if (auto reg_node = g.QueryRegNode(parent->GetId())) {
			op_node = static_cast<tracking::RegNode*>(reg_node)->QueryOpNode(false, tracking::OpType::SPLIT);
		}

		g.AddOp(tracking::OpType::SPLIT, { parent->GetId()}, outputs, op_node);
	}
	// drive_outputs?
}

void compress_with_output(tracking::Graph& g, const std::set<int> input_ids, 
	                      const std::vector<tracking::RegNode*>& outputs, 
	                      const std::vector<tracking::RegNode*>& others,
	                      bool simplify)
{
	std::map<tracking::RegNode*, std::vector<std::pair<tracking::RegNode*, std::shared_ptr<tracking::Trace>>>> split_collect;
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
					split_collect.insert({ t_node, { std::make_pair(n, t) } });
				} else {
					itr->second.push_back({ n, t });
				}
			}
			// drive
			else
			{
#ifdef USE_DERIVE_CREATE_TYPE
				if (simplify)
				{
					auto copy_op = n->QueryOpNode(true, tracking::OpType::COPY);
					if (copy_op && copy_op->GetInputs()[0] == t_node) {
						g.AddOp(tracking::OpType::CREATE, { t_node->GetId() }, { n->GetId() });
					} else {
						g.AddOp(tracking::OpType::DERIVE_CREATE, { t_node->GetId() }, { n->GetId() });
					}
				}
				else
#endif // USE_DERIVE_CREATE_TYPE
				{
					auto type = std::static_pointer_cast<tracking::DriveTrace>(t)->GetType();

					// copy
					if (type & tracking::DTT_COPY_BIT) 
					{
						bool direct_copy = false;
						auto copy_op = static_cast<tracking::RegNode*>(n)->QueryOpNode(true, tracking::OpType::COPY);
						if (copy_op)
						{
							for (auto reg_node : copy_op->GetInputs())
							{
								if (reg_node == t_node)
								{
									direct_copy = true;
									break;
								}
							}
						}

						if (direct_copy)
						{
							g.AddOp(tracking::OpType::CREATE, { t_node->GetId() }, { n->GetId() });
						}
						else
						{
							g.AddOp(tracking::OpType::CREATE, {}, { n->GetId() });
							g.AddOp(tracking::OpType::DRIVE, { t_node->GetId() }, { n->GetId() });
						}
					} 
					else 
					{
						g.AddOp(tracking::OpType::CREATE, {}, { n->GetId() });
					}

					// drive
					if (type & tracking::DTT_DRIVE_BIT) {
						g.AddOp(tracking::OpType::DRIVE, { t_node->GetId() }, { n->GetId() });
					}

					// drive_change
					if (type & tracking::DTT_DRIVE_CHANGE_BIT) {
						g.AddOp(tracking::OpType::DRIVE_CHANGE, { t_node->GetId() }, { n->GetId() });
					}
				}
			}
		}
		else
		{
			// merge
			compress_merge(g, n, simplify);
		}
	}

	// split
	for (auto pair : split_collect) {
		compress_split(g, pair.first, pair.second);
	}

	for (auto reg : others)
	{
		// delete
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

std::shared_ptr<Graph> Compressor::Compress(const Graph& src, bool simplify)
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
	compress_with_output(*dst, input_ids, output, need_output, false);

	// analyze input
	compress_with_input(*dst, input, output_ids);

	return dst;
}

}
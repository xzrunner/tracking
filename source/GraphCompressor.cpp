#include "tracking/GraphCompressor.h"
#include "tracking/Graph.h"
#include "tracking/RegNode.h"
#include "tracking/OpNode.h"

#include <vector>
#include <queue>
#include <map>

#include <assert.h>

namespace
{

void BuildTraces(const std::vector<tracking::RegNode*>& roots)
{
	// init root traces
	for (auto n : roots)
	{
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
			switch (static_cast<tracking::OpNode*>(op)->GetType())
			{
			case tracking::OpType::SPLIT:
			{
				for (auto c : op_outputs)
				{
					const float w = 1.0f / op_outputs.size();
					static_cast<tracking::RegNode*>(c)->CombineTraces(reg, w);
				}
			}
				break;
			case tracking::OpType::MERGE:
			{
				assert(op_outputs.size() == 1);
				static_cast<tracking::RegNode*>(op_outputs[0])->CombineTraces(reg, 1.0f);
			}
				break;
			case tracking::OpType::COPY:
			{
				assert(op_outputs.size() == 1);
				static_cast<tracking::RegNode*>(op_outputs[0])->CopyTrace(reg);
			}
				break;
			}

			for (auto n : op_outputs) {
				buf.push(static_cast<tracking::RegNode*>(n));
			}
		}
	}
}

std::shared_ptr<tracking::Graph> BuildCompressedGraph(const std::vector<tracking::RegNode*>& products)
{
	auto g = std::make_shared<tracking::Graph>();

	std::map<tracking::RegNode*, std::vector<tracking::RegNode*>> split_collect;
	for (auto n : products)
	{
		auto& traces = n->GetTraces();
		if (traces.empty())
		{
			// create
			g->AddOp(tracking::OpType::CREATE, {}, { n->GetId()});
		}
		else if (traces.size() == 1)
		{
			// split
			auto itr = split_collect.find(traces[0].GetNode());
			if (itr == split_collect.end()) {
				split_collect.insert({ traces[0].GetNode(), {n} });
			} else {
				itr->second.push_back(n);
			}
		}
		else
		{
			// merge
			std::vector<int> inputs, inputs_only_derive, outputs;

			outputs.push_back(n->GetId());

			for (auto& t : n->GetTraces())
			{
				inputs.push_back(t.GetNode()->GetId());
				if (t.GetType() == 0)
				{
					inputs_only_derive.push_back(t.GetNode()->GetId());
				}
				//if (t.GetWeight() < 1) {
				//	is_merge = false;
				//}
			}

			if (inputs.size() == inputs_only_derive.size())
			{
				g->AddOp(tracking::OpType::MERGE, inputs, outputs);
			}
			else
			{
				g->AddOp(tracking::OpType::DERIVE_CREATE, inputs, outputs);
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

		if (outputs.size() == outputs_only_derive.size())
		{
			g->AddOp(tracking::OpType::SPLIT, inputs, outputs);
		}
		else if (outputs_only_derive.empty())
		{
			g->AddOp(tracking::OpType::DERIVE_CREATE, inputs, outputs);
		}
		else
		{
			g->AddOp(tracking::OpType::SPLIT, inputs, outputs_only_derive);
		}
	}

	return g;
}

}

namespace tracking
{

std::shared_ptr<Graph> GraphCompressor::Compress(const Graph& graph)
{
	std::vector<RegNode*> src, dst;
	for (auto n : graph.GetAllRegNodes())
	{
		if (n->GetInputs().empty())
		{
			src.push_back(n);
		}
		if (n->GetOutputs().empty())
		{
			dst.push_back(n);
		}
	}

	// clear traces
	for (auto n : graph.GetAllRegNodes()) {
		n->ClearTraces();
	}
	BuildTraces(src);

	return BuildCompressedGraph(dst);
}

}
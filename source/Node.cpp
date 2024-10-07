#include "tracking/Node.h"

namespace tracking
{

void Node::Connect(Node* to)
{
	for (auto output : m_outputs)
	{
		if (output == to) {
			return;
		}
	}

	m_outputs.push_back(to);
	to->m_inputs.push_back(this);
}

void Node::Disconnect(Node* to)
{
	for (auto itr = m_outputs.begin(); itr != m_outputs.end(); )
	{
		if (*itr == to)
		{
			itr = m_outputs.erase(itr);
		}
		else
		{
			++itr;
		}
	}
	for (auto itr = to->m_inputs.begin(); itr != to->m_inputs.end(); )
	{
		if (*itr == this)
		{
			itr = to->m_inputs.erase(itr);
		}
		else
		{ 
			++itr;
		}
	}
}

}
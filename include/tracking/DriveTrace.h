#pragma once

#include "tracking/Trace.h"

#include <stdint.h>

namespace tracking
{

enum DriveTraceTypeFlagBits
{
	DTT_COPY_BIT         = 0x0001,
	DTT_DRIVE_BIT        = 0x0002,
	DTT_DRIVE_CHANGE_BIT = 0x0004
};

class DriveTrace : public Trace
{
public:
	DriveTrace(RegNode* node, uint32_t type);

	uint32_t GetType() const { return m_type; }

	void AddType(uint32_t type) { m_type |= type; }

private:
	uint32_t m_type = 0;

}; // DriveTrace

}
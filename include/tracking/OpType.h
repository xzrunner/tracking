#pragma once

namespace tracking
{

enum class OpType
{
	CREATE,
	DELETE,
	CHANGE,

	SPLIT,
	MERGE,

	COPY,
	DRIVE,
	DRIVE_CHANGE,
	TRANSFER,

	DERIVE,
	DERIVE_CREATE,
};

}
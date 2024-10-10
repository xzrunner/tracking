#pragma once

namespace tracking
{

enum class OpType
{
	UNKNOWN,

	CREATE,
	DELETE,
	CHANGE,

	SPLIT,
	MERGE,

	COPY,
	DRIVE,
	DRIVE_CHANGE,
	TRANSFER,
};

inline bool is_evolve_input(OpType type)
{
	return type != OpType::DRIVE
		&& type != OpType::DRIVE_CHANGE
		&& type != OpType::COPY;
}

inline bool is_input_delete(OpType type)
{
	return type == OpType::DELETE
		|| type == OpType::SPLIT
		|| type == OpType::MERGE;
}

inline bool is_output_create(OpType type)
{
	return type != OpType::DRIVE
		&& type != OpType::DRIVE_CHANGE;
}

inline bool is_need_output(OpType type)
{
	return type == OpType::DRIVE_CHANGE;
}

inline bool need_transmit_trace(OpType type)
{
	return type == tracking::OpType::COPY
		|| type == tracking::OpType::DRIVE
		|| type == tracking::OpType::DRIVE_CHANGE;
}

}
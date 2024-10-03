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

static bool is_derive_input(OpType type)
{
	return type != OpType::DRIVE
		&& type != OpType::DRIVE_CHANGE
		&& type != OpType::COPY;
}

static bool is_input_delete(OpType type)
{
	return type == OpType::DELETE
		|| type == OpType::SPLIT
		|| type == OpType::MERGE
		|| type == OpType::DERIVE;
}

static bool is_output_create(OpType type)
{
	return type != OpType::DRIVE
		&& type != OpType::DRIVE_CHANGE;
}

static bool is_need_output(OpType type)
{
	return type == OpType::DRIVE_CHANGE;
}

static bool can_merge_output(OpType type)
{
	return type == OpType::DERIVE_CREATE;
}

static bool need_transmit_trace(OpType type)
{
	return type == tracking::OpType::COPY
		|| type == tracking::OpType::DRIVE
		|| type == tracking::OpType::DRIVE_CHANGE;
}

}
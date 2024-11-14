#pragma once

#define USE_EVOLVE_TYPE
//#define USE_DRIVE_CREATE_TYPE

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
	DRIVE_ADD,
	DRIVE_MOD,
	TRANSFER,

#ifdef USE_EVOLVE_TYPE
	EVOLVE,
#endif // USE_EVOLVE_TYPE
#ifdef USE_DRIVE_CREATE_TYPE
	DRIVE_CREATE,
#endif // USE_DRIVE_CREATE_TYPE
};

inline bool is_evolve_input(OpType type)
{
	return type != OpType::COPY
		&& type != OpType::DRIVE_ADD
		&& type != OpType::DRIVE_MOD;
}

inline bool is_input_delete(OpType type)
{
	return type == OpType::DELETE
		|| type == OpType::SPLIT
		|| type == OpType::MERGE
#ifdef USE_EVOLVE_TYPE
		|| type == OpType::EVOLVE
#endif // USE_EVOLVE_TYPE
		;
}

inline bool is_output_create(OpType type)
{
	return /*type != OpType::COPY
		&& */type != OpType::DRIVE_ADD
		&& type != OpType::DRIVE_MOD;
}

inline bool is_need_output(OpType type)
{
	return type == OpType::DRIVE_MOD;
}

inline bool need_transmit_trace(OpType type)
{
	return type == tracking::OpType::COPY
		|| type == tracking::OpType::DRIVE_ADD
		|| type == tracking::OpType::DRIVE_MOD;
}

}
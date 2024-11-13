#pragma once

#define USE_DERIVE_TYPE
//#define USE_DERIVE_CREATE_TYPE

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

#ifdef USE_DERIVE_TYPE
	DERIVE,
#endif // USE_DERIVE_TYPE
#ifdef USE_DERIVE_CREATE_TYPE
	DERIVE_CREATE,
#endif // USE_DERIVE_CREATE_TYPE
};

inline bool is_evolve_input(OpType type)
{
	return type != OpType::COPY
		&& type != OpType::DRIVE
		&& type != OpType::DRIVE_CHANGE;
}

inline bool is_input_delete(OpType type)
{
	return type == OpType::DELETE
		|| type == OpType::SPLIT
		|| type == OpType::MERGE
#ifdef USE_DERIVE_TYPE
		|| type == OpType::DERIVE
#endif // USE_DERIVE_TYPE
		;
}

inline bool is_output_create(OpType type)
{
	return /*type != OpType::COPY
		&& */type != OpType::DRIVE
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
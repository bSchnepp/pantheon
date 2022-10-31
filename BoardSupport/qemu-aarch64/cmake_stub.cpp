/* Stub for CMake */

#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

struct TypeDescriptor
{
	UINT16 Kind;
	UINT16 Info;
	char Name[];
};

struct SourceLocation
{
	const CHAR *File;
	UINT32 Line;
	UINT32 Col;
};

struct TypeMismatchDataV1
{
	struct SourceLocation Loc;
	struct TypeDescriptor *Type;
	UINT8 LogAlign;
	UINT8 TypeCheckKind;
};

struct ShiftOutOfBoundsData
{
	struct SourceLocation Loc;
	struct TypeDescriptor *LeftType;
	struct TypeDescriptor *RightType;
};

struct OutOfBoundsData
{
	struct SourceLocation Loc;
	struct TypeDescriptor *ArrayType;
	struct TypeDescriptor *IndexType;
};

struct InvalidValueData
{
	struct SourceLocation Loc;
	struct TypeDescriptor *Type;
};

struct OverflowData
{
	struct SourceLocation Loc;
	struct TypeDescriptor *Type;
};

struct VlaBoundData
{
	struct SourceLocation Loc;
	struct TypeDescriptor *Type;
};

struct NonnullReturnData
{
	struct SourceLocation Loc;
};

struct UnreachableData
{
	struct SourceLocation Loc;
};

struct NonnullArgData
{
	struct SourceLocation Loc;
	struct SourceLocation AttrLoc;
	int ArgIndex;
};

struct InvalidBuiltinData
{
	struct SourceLocation Loc;
	UINT8 TypeCheckKind;
};

const char *TypeCheckKinds[] = 
{
	"load of",
	"store to",
	"ref binding to",
	"member access inside",
	"member call on",
	"constructor call on",
	"downcast of",
	"downcast of",
	"upcast of",
	"virtual base cast of",
	"non-null binding to",
	"dynamic operation on",
};

extern "C"
{

[[noreturn]] 
static VOID _inf_loop(void)
{
	for (;;) {}
}

void __ubsan_handle_add_overflow(struct OverflowData *Data, uintptr_t, uintptr_t)
{
	(void)Data;
	_inf_loop();
}

void __ubsan_handle_sub_overflow(struct OverflowData *Data, uintptr_t, uintptr_t)
{
	(void)Data;
	_inf_loop();
}


void __ubsan_handle_mul_overflow(struct OverflowData *Data, uintptr_t, uintptr_t) 
{
	(void)Data;
	_inf_loop();
}

void __ubsan_handle_divrem_overflow(struct OverflowData *Data, uintptr_t, uintptr_t) 
{
	(void)Data;
	_inf_loop();
}

void __ubsan_handle_negate_overflow(struct OverflowData *Data, uintptr_t) 
{
	(void)Data;
	_inf_loop();
}

void __ubsan_handle_pointer_overflow(struct OverflowData *Data, uintptr_t, uintptr_t) 
{
	(void)Data;
	_inf_loop();
}

void __ubsan_handle_shift_out_of_bounds(struct ShiftOutOfBoundsData *Data, uintptr_t, uintptr_t) 
{
	(void)Data;
	_inf_loop();
}

void __ubsan_handle_out_of_bounds(struct OutOfBoundsData *Data, uintptr_t) 
{
	(void)Data;
	_inf_loop();
}

void __ubsan_handle_load_invalid_value(struct InvalidValueData *Data, uintptr_t)
{
	(void)Data;
	_inf_loop();
}

void __ubsan_handle_vla_bound_not_positive(struct VlaBoundData *Data, uintptr_t)
{
	(void)Data;
	_inf_loop();
}

void __ubsan_handle_nonnull_arg(struct NonnullArgData *Data) 
{
	(void)Data;
	_inf_loop();
}

void __ubsan_handle_builtin_unreachable(struct UnreachableData *Data) 
{
	(void)Data;
	_inf_loop();
}

void __ubsan_handle_invalid_builtin(struct InvalidBuiltinData *Data) 
{
	(void)Data;
	_inf_loop();
}

void __ubsan_handle_type_mismatch_v1(struct TypeMismatchDataV1 *Data, uintptr_t) 
{
	(void)Data;
	_inf_loop();
}

void __ubsan_handle_nonnull_return_v1(struct NonnullReturnData *Data, struct SourceLocation *)
{
	(void)Data;
	_inf_loop();
}

uintptr_t __stack_chk_guard = 0xDEADC0DEDEADBEEF;

[[noreturn]] VOID __stack_chk_fail(void)
{
	_inf_loop();
}

}
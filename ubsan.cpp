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

void __ubsan_handle_add_overflow(struct OverflowData *Data, uintptr_t, uintptr_t)
{
	pantheon::StopErrorFmt("<__ubsan_handle_add_overflow>: %s on line %u and col %u\n\t errored type was %s, because %s\n", 
		Data->Loc.File, Data->Loc.Line, Data->Loc.Col, Data->Type->Name, TypeCheckKinds[Data->Type->Kind]);
}

void __ubsan_handle_sub_overflow(struct OverflowData *Data, uintptr_t, uintptr_t)
{
	pantheon::StopErrorFmt("<__ubsan_handle_sub_overflow>: %s on line %u and col %u\n\t errored type was %s, because %s\n", 
		Data->Loc.File, Data->Loc.Line, Data->Loc.Col, Data->Type->Name, TypeCheckKinds[Data->Type->Kind]);
}

void __ubsan_handle_mul_overflow(struct OverflowData *Data, uintptr_t, uintptr_t) 
{
	pantheon::StopErrorFmt("<__ubsan_handle_mul_overflow>: %s on line %u and col %u\n\t errored type was %s, because %s\n", 
		Data->Loc.File, Data->Loc.Line, Data->Loc.Col, Data->Type->Name, TypeCheckKinds[Data->Type->Kind]);
}

void __ubsan_handle_divrem_overflow(struct OverflowData *Data, uintptr_t, uintptr_t) 
{
	pantheon::StopErrorFmt("<__ubsan_handle_divrem_overflow>: %s on line %u and col %u\n\t errored type was %s, because %s\n", 
		Data->Loc.File, Data->Loc.Line, Data->Loc.Col, Data->Type->Name, TypeCheckKinds[Data->Type->Kind]);
}

void __ubsan_handle_negate_overflow(struct OverflowData *Data, uintptr_t) 
{
	pantheon::StopErrorFmt("<__ubsan_handle_negate_overflow>: %s on line %u and col %u\n\t errored type was %s, because %s\n", 
		Data->Loc.File, Data->Loc.Line, Data->Loc.Col, Data->Type->Name, TypeCheckKinds[Data->Type->Kind]);
}

void __ubsan_handle_pointer_overflow(struct OverflowData *Data, uintptr_t base, uintptr_t result) 
{
	pantheon::StopErrorFmt("<__ubsan_handle_pointer_overflow>: %s on line %u and col %u\n\t errored type was %s, because %s. Base was %ld and result was %ld\n", 
		Data->Loc.File, Data->Loc.Line, Data->Loc.Col, Data->Type->Name, TypeCheckKinds[Data->Type->Kind], base, result);
}

void __ubsan_handle_shift_out_of_bounds(struct ShiftOutOfBoundsData *Data, uintptr_t, uintptr_t) 
{
	pantheon::StopErrorFmt("<__ubsan_handle_shift_out_of_bounds>: %s on line %u and col %u\n\t errored types were %s and %s, because %s and %s\n", 
		Data->Loc.File, Data->Loc.Line, Data->Loc.Col, Data->LeftType->Name, Data->RightType->Name, TypeCheckKinds[Data->LeftType->Kind], TypeCheckKinds[Data->RightType->Kind]);
}

void __ubsan_handle_out_of_bounds(struct OutOfBoundsData *Data, uintptr_t Index) 
{
	pantheon::StopErrorFmt("<__ubsan_handle_out_of_bounds>: %s on line %u and col %u\n\t on array of type %s with indexed type of %s, on index %ld\n", 
		Data->Loc.File, Data->Loc.Line, Data->Loc.Col, Data->ArrayType->Name, Data->IndexType->Name, Index);
}

void __ubsan_handle_load_invalid_value(struct InvalidValueData *Data, uintptr_t val)
{
	pantheon::StopErrorFmt("<__ubsan_handle_load_invalid_value>: %s on line %u and col %u\n\t errored value was 0x%lx\n", 
		Data->Loc.File, Data->Loc.Line, Data->Loc.Col, val);
}

void __ubsan_handle_vla_bound_not_positive(struct VlaBoundData *Data, uintptr_t Bound)
{
	pantheon::StopErrorFmt("<__ubsan_handle_vla_bound_not_positive>: %s on line %u and col %u\n\t on type %s and bound %ld\n", 
		Data->Loc.File, Data->Loc.Line, Data->Loc.Col, Data->Type->Name, Bound);
}

void __ubsan_handle_nonnull_arg(struct NonnullArgData *Data) 
{
	pantheon::StopErrorFmt("<__ubsan_handle_nonnull_arg>: %s on line %u and col %u\n\t, from Non-null argument %s at line %ld and col %ld, argument number %lx\n", 
		Data->Loc.File, Data->Loc.Line, Data->Loc.Col, Data->AttrLoc.File, Data->AttrLoc.Line, Data->AttrLoc.Col, Data->ArgIndex);
}

void __ubsan_handle_builtin_unreachable(struct UnreachableData *Data) 
{
	pantheon::StopErrorFmt("<__ubsan_handle_builtin_unreachable>: %s on line %u and col %u\n\t, which was marked as unreachable\n", 
		Data->Loc.File, Data->Loc.Line, Data->Loc.Col);
}

void __ubsan_handle_invalid_builtin(struct InvalidBuiltinData *Data) 
{
	pantheon::StopErrorFmt("<__ubsan_handle_invalid_builtin>: %s on line %u and col %u\n\t, which is of kind %s\n", 
		Data->Loc.File, Data->Loc.Line, Data->Loc.Col, TypeCheckKinds[Data->TypeCheckKind]);
}

void __ubsan_handle_type_mismatch_v1(struct TypeMismatchDataV1 *Data, uintptr_t Pointer) 
{
	pantheon::StopErrorFmt("<__ubsan_handle_type_mismatch_v1>: %s on line %u and col %u\n\t on type %s with log_align %hhd and reason %s, at address %lx\n", 
		Data->Loc.File, Data->Loc.Line, Data->Loc.Col, Data->Type->Name, Data->LogAlign, TypeCheckKinds[Data->TypeCheckKind], Pointer);
}

void __ubsan_handle_nonnull_return_v1(struct NonnullReturnData *Data, struct SourceLocation *Loc)
{
	pantheon::StopErrorFmt("<__ubsan_handle_nonnull_return_v1>: %s on line %u and col %u\n\t, from Non-null function %s at line %ld and col %ld\n", 
		Data->Loc.File, Data->Loc.Line, Data->Loc.Col, Loc->File, Loc->Line, Loc->Col);
}

}
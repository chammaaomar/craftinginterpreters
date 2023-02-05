#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
// the reason we don't put the function body directly in the macro is because
// the value expression is used multiple times, and thus may be evaluated multiple times
// which would lead to bugs if the expression has side effects
#define IS_STRING(value) is_obj_type(value, OBJ_STRING)

#define AS_STRING(value) ((ObjString *)AS_OBJ(value));
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars);

typedef enum
{
    OBJ_STRING,
} ObjType;

struct Obj
{
    ObjType type;
};

struct ObjString
{
    Obj obj;
    int length;
    char *chars;
};

static inline bool is_obj_type(Value value, ObjType type)
{
    return IS_OBJ(value) && (AS_OBJ(value)->type == type);
}

#endif
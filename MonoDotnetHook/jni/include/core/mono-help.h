#pragma once
#include <mono/metadata/class.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/debug-helpers.h>

struct _MonoProfiler {
	int ncalls;
};
typedef struct _MonoProfiler MonoProfiler;

typedef struct {
	uint32_t eip;			// pc 
	uint32_t ebp;			// fp
	uint32_t esp;			// sp
	uint32_t regs[16];
	double fregs[8];		// arm : 8
} MonoContext;
typedef int(*MonoStackFrameWalk) (MonoDomain*, MonoContext*, MonoJitInfo*, void*);
extern "C" void mono_walk_stack(MonoDomain *domain, void *jit_tls, MonoContext *start_ctx, MonoStackFrameWalk func, void *user_data);

/*mono头文件中未声明 但实际却导出的函数和结构*/
extern "C" void g_free (void const *p);
extern "C" char *mono_pmip (void *ip);

void print_class_name (MonoClass *clazz);

void print_object_class_name (MonoObject *obj);

void print_class_all_methods (MonoClass *clz);

void set_obj_field_value (MonoObject *obj, char const *val_name, void *value);

void get_obj_field_value (MonoObject *obj, const char *key, void *value);

MonoMethod *get_class_method (MonoClass *clz, char const *full_name);

MonoClass *get_base_class (char const *name);

MonoClass *get_class_with_name (char const *image_name, char const *name_space, char const *class_name);

char const *get_method_image_name (MonoMethod *method);

char const *get_method_class_name (MonoMethod *method);

char const *get_method_namespace_name (MonoMethod *method);

MonoMethod *get_method_with_token (char const *image_name, uint32_t token);

MonoMethod *find_method(char *image_name, char *space_name, char *class_name, char *method_name);

void find_image_by_name(const char *image_name);
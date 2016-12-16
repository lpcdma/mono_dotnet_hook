#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <core/mono-help.h>
#include <core/logger.h>
#include <string>
#include <mono/metadata/assembly.h>

static MonoImage *found_image = NULL;

void print_class_name (MonoClass *clazz) {
    LOGD ("class name : %s", mono_class_get_name (clazz));
}

void print_object_class_name (MonoObject *obj) {
    MonoClass *clazz = mono_object_get_class (obj);
    print_class_name (clazz);
}

void print_class_all_methods (MonoClass *clz) {
    void *it = 0;
    MonoMethod *m;
    while (m = mono_class_get_methods (clz, &it)) {
        char *name = mono_method_full_name (m, 1);
        LOGD ("%s", name);
        g_free (name);
    }
}

void set_obj_field_value (MonoObject *obj, char const *val_name, void *value) {
    MonoClass *clz = mono_object_get_class (obj);
    MonoClassField *field = mono_class_get_field_from_name (clz, val_name);
    mono_field_set_value (obj, field, value);
}

/*获取对象中的一个字段的包装函数*/
void get_obj_field_value (MonoObject *obj, const char *key, void *value) {
    MonoClass *clazz = mono_object_get_class (obj);
    MonoClassField *field = mono_class_get_field_from_name (clazz, key);
    void *re = 0;
    mono_field_get_value (obj, field, value);
}

MonoMethod *get_class_method (MonoClass *clz, char const *full_name) {
    void *it = 0;
    MonoMethod *m;
    while (m = mono_class_get_methods (clz, &it)) {
        char *name = mono_method_full_name (m, 1);
        if (strcmp (full_name, name) == 0) {
            g_free (name);
            return m;
        }
        g_free (name);
    }
}


struct BaseClass {
    char const *name;
    MonoClass *(*func)();
};

static const BaseClass B[] = {
    {"object", mono_get_object_class},
    {"byte", mono_get_byte_class},
    {"void", mono_get_void_class},
    {"boolean", mono_get_boolean_class},
    {"sbyte", mono_get_sbyte_class},
    {"int16", mono_get_int16_class},
    {"uint16", mono_get_uint16_class},
    {"int32", mono_get_int32_class},
    {"uint32", mono_get_uint32_class},
    {"intptr", mono_get_intptr_class},
    {"uintptr", mono_get_uintptr_class},
    {"int64", mono_get_int64_class},
    {"uint64", mono_get_uint64_class},
    {"single", mono_get_single_class},
    {"double", mono_get_double_class},
    {"char", mono_get_char_class},
    {"string", mono_get_string_class},
    {"enum", mono_get_enum_class},
    {"array", mono_get_array_class},
    {"thread", mono_get_thread_class},
    {"exception", mono_get_exception_class}
};

MonoClass *get_base_class (char const *name) {
    for (int i = 0; i < sizeof (B) / sizeof (BaseClass); i++) {
        if (strcmp (name, B[i].name) == 0)
            return B[i].func ();
    }
    return 0;
}

MonoClass *get_class_with_name (char const *image_name, char const *name_space, char const *class_name) {
    MonoClass *clazz = get_base_class (class_name);
    if (clazz)
        return clazz;
    MonoImage *image = mono_image_loaded (image_name);
    if (!image) {
        return 0;
    }
    clazz = mono_class_from_name (image, name_space, class_name);
    if (!clazz) {
        return 0;
    }
    return clazz;
}

char const *get_method_image_name (MonoMethod *method) {
    MonoClass *clazz = mono_method_get_class (method);
    if (!clazz) return 0;
    MonoImage *image = mono_class_get_image (clazz);
    if (!image) return 0;
    return mono_image_get_name (image);
}

char const *get_method_class_name (MonoMethod *method) {
    MonoClass *clazz = mono_method_get_class (method);
    return mono_class_get_name (clazz);
}

char const *get_method_namespace_name (MonoMethod *method) {
    MonoClass *clazz = mono_method_get_class (method);
    return mono_class_get_namespace (clazz);
}

MonoMethod *get_method_with_token (char const *image_name, uint32_t token) {
    MonoImage *image = mono_image_loaded (image_name);
    if (!image) {
        return 0;
    }
    MonoMethod *method = (MonoMethod*)mono_ldtoken (image, token, 0, 0);
    if (!method) {
        return 0;
    }
    return method;
}

static void foreach_assembly(MonoAssembly *ass, void *user_data)
{
	if (!found_image)
	{
		MonoImage *cur_image = mono_assembly_get_image(ass);
		const char *cur_image_name = mono_image_get_name(cur_image);
		if (strcmp(cur_image_name, (const char *)user_data) == 0)
		{
			found_image = cur_image;
		}
	}
}

void find_image_by_name(const char *image_name)
{
	found_image = NULL;
	mono_assembly_foreach((MonoFunc)foreach_assembly, (void *)image_name);
}


MonoMethod *find_method(char *image_name, char *space_name, char *class_name, char *method_name)
{
	MonoMethod *target_method = NULL;
	find_image_by_name(image_name);
	if (found_image)
	{
		MonoClass *klass = mono_class_from_name(found_image, space_name, class_name);
		if (klass)
		{
			std::string str_fullname;
			str_fullname.append(class_name);
			str_fullname.append("::");
			str_fullname.append(method_name);
			MonoMethodDesc *method_desc = mono_method_desc_new(str_fullname.data(), false);
			target_method = mono_method_desc_search_in_class(method_desc, klass);
			mono_method_desc_free(method_desc);
		}
	}

	return target_method;
}
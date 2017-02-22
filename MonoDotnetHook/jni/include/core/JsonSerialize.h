#pragma once
#include <mono/metadata/object.h>
#include <mono/metadata/loader.h>
#include <mono/metadata/object.h>
#include <json/json.h>

#define FIELD_ATTRIBUTE_STATIC                0x0010
#define FIELD_ATTRIBUTE_SPECIAL_NAME          0x0200
#define FIELD_ATTRIBUTE_RT_SPECIAL_NAME       0x0400
#define TYPE_ATTRIBUTE_ABSTRACT              0x00000080
#define METHOD_ATTRIBUTE_ABSTRACT                  0x0400
/* a field is ignored if it's named "_Deleted" and it has the specialname and rtspecialname flags set */
#define mono_field_is_deleted(field) ((mono_field_get_flags(field) & (FIELD_ATTRIBUTE_SPECIAL_NAME | FIELD_ATTRIBUTE_RT_SPECIAL_NAME)) \
				      && (strcmp (mono_field_get_name (field), "_Deleted") == 0))

#define mono_field_is_static(field) (mono_field_get_flags(field) & FIELD_ATTRIBUTE_STATIC)


struct _MonoType {
	union {
		_MonoClass *klass; /* for VALUETYPE and CLASS */
		_MonoType *type;   /* for PTR */
		_MonoArrayType *array; /* for ARRAY */
		_MonoMethodSignature *method;
		_MonoGenericParam *generic_param; /* for VAR and MVAR */
		_MonoGenericClass *generic_class; /* for GENERICINST */
	} data;
	unsigned int attrs : 16; /* param attributes or field flags */
	MonoTypeEnum type : 8;
	unsigned int num_mods : 6;  /* max 64 modifiers follow at the end */
	unsigned int byref : 1;
	unsigned int pinned : 1;  /* valid when included in a local var signature */
	MonoCustomMod modifiers[MONO_ZERO_LEN_ARRAY]; /* this may grow */
};

struct _MonoGenericInst {
#ifndef MONO_SMALL_CONFIG
	unsigned int id;			/* unique ID for debugging */
#endif
	unsigned int type_argc : 22;	/* number of type arguments */
	unsigned int is_open : 1;	/* if this is an open type */
	MonoType *type_argv[MONO_ZERO_LEN_ARRAY];
};

struct _MonoGenericContext {
	/* The instantiation corresponding to the class generic parameters */
	_MonoGenericInst *class_inst;
	/* The instantiation corresponding to the method generic parameters */
	MonoGenericInst *method_inst;
};

struct _MonoGenericClass {
	_MonoClass *container_class;	/* the generic type definition */
	_MonoGenericContext context;	/* a context that contains the type instantiation doesn't contain any method instantiation */ /* FIXME: Only the class_inst member of "context" is ever used, so this field could be replaced with just a monogenericinst */
	unsigned int is_dynamic : 1;		/* We're a MonoDynamicGenericClass */
	unsigned int is_tb_open : 1;		/* This is the fully open instantiation for a type_builder. Quite ugly, but it's temporary.*/
	_MonoClass *cached_class;	/* if present, the MonoClass corresponding to the instantiation.  */

								/*
								* The image set which owns this generic class. Memory owned by the generic class
								* including cached_class should be allocated from the mempool of the image set,
								* so it is easy to free.
								*/
	void *owner;
};


typedef uint32_t mono_array_size_t;
typedef int32_t mono_array_lower_bound_t;
#define MONO_ARRAY_MAX_INDEX ((int32_t) 0x7fffffff)
#define MONO_ARRAY_MAX_SIZE  ((uint32_t) 0xffffffff)

typedef struct {
	mono_array_size_t length;
	mono_array_lower_bound_t lower_bound;
} MonoArrayBounds;

struct _MonoArray {
	MonoObject obj;
	/* bounds is NULL for szarrays */
	MonoArrayBounds *bounds;
	/* total number of elements of the array */
	mono_array_size_t max_length;
	/* we use double to ensure proper alignment on platforms that need it */
	double vector[MONO_ZERO_LEN_ARRAY];
};


struct _MonoMethodSignature {
	MonoType     *ret;
#ifdef MONO_SMALL_CONFIG
	guint8        param_count;
	gint8         sentinelpos;
	unsigned int  generic_param_count : 5;
#else
	uint16_t       param_count;
	int16_t        sentinelpos;
	unsigned int  generic_param_count : 16;
#endif
	unsigned int  call_convention : 6;
	unsigned int  hasthis : 1;
	unsigned int  explicit_this : 1;
	unsigned int  pinvoke : 1;
	unsigned int  is_inflated : 1;
	unsigned int  has_type_parameters : 1;
	MonoType     *params[MONO_ZERO_LEN_ARRAY];
};

struct _MonoReflectionType {
	MonoObject object;
	MonoType  *type;
};

extern "C" void *mono_vtable_get_static_field_data(MonoVTable *vt);




class JsonSerialize
{
public:
	JsonSerialize();
	void Serialize(MonoObject* obj, Json::Value &container);
	MonoObject *Deserialize(Json::Value container, MonoClass *klass);

protected:
	virtual bool CanSerializeClass(MonoClass *klass);
	virtual bool CanSerializeProperty(MonoProperty *prop);
	virtual bool CanSerializeField(_MonoClassField *field);
	virtual void SerializeProperty(MonoObject*obj, MonoProperty *prop, Json::Value &container);
	virtual void SerializeField(MonoObject*obj, MonoClassField *field, Json::Value &container);
	virtual void SerializeMonoTypeWithAddr(MonoType *mono_type, void *addr, Json::Value &container, Json::Value key);
	virtual void DeserializeProperty(MonoObject *obj, MonoProperty* prop, Json::Value container);
	virtual void DeserializeField(MonoObject *obj, MonoClassField* field, Json::Value container);
	virtual void DeserializeMonoTypeWithAddr(MonoType *mono_type, void *addr, Json::Value container);

private:
	MonoDomain	*domain;
	static void get_enum_desc_by_value(MonoType* mono_type, uint32_t enum_value, char* result, uint32_t size);
	static uint32_t get_enum_value_by_desc(MonoType *mono_type, const char *desc);
	static bool mono_type_is_enum(MonoType *mono_type);
	static void advance_json_insert(Json::Value &container, Json::Value key, Json::Value value);
};

#include <mono/metadata/assembly.h>
#include <mono/metadata/metadata.h>
#include <mono/metadata/object.h>
#include <cstring>
#include <map>
#include <string>
#include <core/logger.h>
#include <core/mono-help.h>
#include <core/common-help.h>
#include <core/armhook.h>

static char split_str = '.';
static char expire_param_count = 4;
static MonoImage *found_image = NULL;
static std::map<MonoMethod *, void *> hook_dict;

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

static void find_image_by_name(const char *image_name)
{
	found_image = NULL;
	mono_assembly_foreach((MonoFunc)foreach_assembly, (void *)image_name);
}


static MonoMethod *find_method(char *image_name, char *space_name, char *class_name, char *method_name)
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


void hook_dotnet(MonoString *str_target, MonoString *str_replace, MonoString *str_old)
{
	char *target_full_name = mono_string_to_utf8(str_target);
	char *replace_full_name = mono_string_to_utf8(str_replace);
	char *old_full_name = mono_string_to_utf8(str_old);

	std::vector<char *> vec_target;
	std::vector<char *> vec_replace;
	std::vector<char *> vec_old;
	splitstring(target_full_name, split_str, vec_target);
	splitstring(replace_full_name, split_str, vec_replace);
	splitstring(old_full_name, split_str, vec_old);

	if (vec_replace.size()!=expire_param_count || vec_target.size()!=expire_param_count || vec_old.size()!=expire_param_count)
	{
		LOGD("dotnet-hook failed split param %d %d %d", vec_target.size(), vec_replace.size(), vec_old.size());
		return;
	}

	MonoMethod *target_method = find_method(vec_target[0], vec_target[1], vec_target[2], vec_target[3]);
	MonoMethod *replace_method = find_method(vec_replace[0], vec_replace[1], vec_replace[2], vec_replace[3]);
	MonoMethod *old_method = find_method(vec_old[0], vec_old[1], vec_old[2], vec_old[3]);
	if (!target_method || !replace_method || !old_method)
	{
		LOGD("dotnet-hook failed find method %x %x %x", (int)target_method, (int)replace_method, (int)old_method);
		return;
	}

	void *origin_code_start_addr = mono_compile_method(target_method);
	void *replace_code_start_addr = mono_compile_method(replace_method);
	void *old_code_start_addr = mono_compile_method(old_method);
	if (!origin_code_start_addr || !replace_code_start_addr || !old_method)
	{
		LOGD("dotnet-hook failed compile method %x %x %x", (int)origin_code_start_addr, (int)replace_code_start_addr, (int)old_code_start_addr);
		return;
	}

	void *tramp_addr;
	arm_hook(origin_code_start_addr, replace_code_start_addr, &tramp_addr);
	arm_hook(old_code_start_addr, tramp_addr, NULL);

	hook_dict[target_method] = tramp_addr;
	mono_add_internal_call(old_full_name, old_method);

	LOGD("hook sucess");
}

void unhook_dotnet(MonoString *str_target)
{
	std::vector<char *> vec_target;
	char *target_full_name = mono_string_to_utf8(str_target);
	splitstring(target_full_name, split_str, vec_target);
	if (vec_target.size() != expire_param_count)
	{
		LOGD("dotnet-unhook failed split param %d", vec_target.size());
		return;
	}

	MonoMethod *target_method = find_method(vec_target[0], vec_target[1], vec_target[2], vec_target[3]);
	if (!target_method)
	{
		LOGD("dotnet-unhook failed find method %x", (int)target_method);
		return;
	}

	if (hook_dict.find(target_method)==hook_dict.end())
	{
		LOGD("dotnet-unhook failed find hooked method %x", (int)target_method);
		return;
	}

	arm_unhook(mono_compile_method(target_method), hook_dict[target_method]);
	hook_dict.erase(target_method);

	LOGD("unhook sucess");
}

void logd(_MonoString *strobj)
{
	LOGD("%s", mono_string_to_utf8(strobj));
}
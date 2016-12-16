#include <mono/metadata/assembly.h>
#include <mono/metadata/metadata.h>
#include <mono/metadata/object.h>
#include <map>
#include <core/logger.h>
#include <core/mono-help.h>
#include <core/common-help.h>
#include <core/armhook.h>

static char split_str = '.';
static char expire_param_count = 4;
static std::map<MonoMethod *, void *> hook_dict;

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
	if (!origin_code_start_addr || !replace_code_start_addr || !old_code_start_addr)
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
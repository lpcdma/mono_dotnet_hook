#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <pthread.h>
#include <mono/metadata/image.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/debug-helpers.h>
#include <core/logger.h>
#include <core/dotnet-hook.h>
#include <core/mono-help.h>


struct LoadParam
{
	int delay_time;
	const char *dll_path;
	const char *image_name;
	const char *space_name;
	const char *class_name;
	const char *method_name;
};


static void create_map_file(const char *filepath, char **data, uint32_t *data_len)
{
	*data = NULL;
	*data_len = 0;

	int fd = open(filepath, O_RDWR);
	if (fd>0)
	{
		struct stat status;
		fstat(fd, &status);
		void *map_ret = mmap(0, status.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
		if (map_ret != MAP_FAILED)
		{
			*data = (char *)map_ret;
			*data_len = status.st_size;
		}
	}
}

static void *load_thread(void *param)
{
	void *p_lib_mono;
	do
	{
		p_lib_mono = dlopen("libmono.so", RTLD_LAZY);
	} while (!p_lib_mono);
	dlclose(p_lib_mono);

	if (!param)
	{
		LOGE("err game-plugin thread param null...");
		return NULL;
	}

	LoadParam *load_param = (LoadParam *)param;
	sleep(load_param->delay_time);

	MonoDomain *domain = mono_get_root_domain();
	if (!domain)
	{
		LOGE("err game-plugin root domain null");
		return NULL;
	}

	MonoThread *thread = mono_thread_attach(domain);
	if (!thread)
	{
		LOGE("err game-plugin thread attach failed...");
		return NULL;
	}

	char *data = NULL;
	uint32_t data_len = 0;
	create_map_file(load_param->dll_path, &data, &data_len);
	if (data)
	{
		MonoImage *image = mono_image_open_from_data_with_name(data, data_len, false, NULL, false, load_param->image_name);
		if (image)
		{
			MonoAssembly *assembly = mono_assembly_load_from(image, load_param->dll_path, NULL);
			if (assembly)
			{
				MonoMethod *method = find_method((char *)load_param->image_name, (char *)load_param->space_name, (char *)load_param->class_name, (char *)load_param->method_name);
				if (method)
				{
					mono_runtime_invoke(method, NULL, NULL, NULL);
					LOGD("load game-plugin ok...");
				}
				else
				{
					LOGD("failed find method");
				}
			}
			else
			{
				LOGD("failed load assembly");
			}
		}
		else
		{
			LOGD("failed load image");
		}
	}
	else
	{
		LOGD("failed map file %s", load_param->dll_path);
	}

	mono_thread_detach(thread);
	return NULL;
}

__attribute__((visibility("default")))
void load_exec_dll(const char *dll_path, const char *image_name, const char *space_name, const char *class_name, const char *method_name, int delay_time)
{
	pthread_t tid;
	LoadParam *param = new LoadParam;

	param->dll_path = dll_path;
	param->space_name = space_name;
	param->class_name = class_name;
	param->method_name = method_name;
	param->delay_time = delay_time;
	param->image_name = image_name;

	pthread_create(&tid, NULL, load_thread, param);
}

__attribute__((constructor))
static void startwork_game_plugin() 
{
	LOGD("load plugin monohook...");
	load_exec_dll("/data/local/tmp/MonoHook.dll", "MonoHook", "MonoHook", "Test", "Test::Entry", 10);
}
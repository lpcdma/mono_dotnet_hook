#pragma once
#include <mono/metadata/image.h>

void load_exec_dll(const char *dll_path, const char *space_name, const char *class_name, const char *method_name);
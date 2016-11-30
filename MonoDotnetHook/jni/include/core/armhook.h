#pragma once

void cache_flush(int begin, int end);
int arm_hook(void *org, void *dst, void **trampo);
int arm_unhook(void *org, void *tramp_addr);
int call_function(void *addr, int argc, void *argv);
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <deque>
#include <pthread.h>
#include <errno.h>
#include <sys/mman.h>
#include <core/armhook.h>
#include <core/logger.h>


void cache_flush(int begin, int end)
{   
    const int syscall = 0xf0002;
    __asm __volatile (
        "mov     r0, %0\n"          
        "mov     r1, %1\n"
        "mov     r7, %2\n"
        "mov     r2, #0x0\n"
        "svc     0x00000000\n"
        :
        :   "r" (begin), "r" (end), "r" (syscall)
        :   "r0", "r1", "r7"
       );
}

int call_function(void *addr, int argc, void *argv){
	__asm __volatile(
			"push {r0-r14}\n\t"
			"mov r5, %[argc]\n\t"
			"mov r6, %[argv]\n\t"
			"mov r7, %[addr]\n\t"
			"sub sp, #200\n\t"
			"cmp r5, #4\n\t"
			"ble arg_4\n\t"
			"sub r8, r5, #4\n\t"
			"add sp, r8 , ASL #2\n\t"
			"b more_arg\n\t"
		"load_arg:"
			"sub r5, r5, #1\n\t"
			"mov r9, r5, ASL #2\n\t"
			"ldr r0, [r6, r9]\n\t"
			"bx lr\n\t"
		"more_arg:\n\t"
			"cmp r5, #5\n\t"
			"blt arg_4\n\t"
			"bl load_arg\n\t"
			"push {r0}\n\t"
			"b more_arg\n\t"
		"arg_4:\n\t"
			"cmp r5, #4\n\t"
			"blt arg_3\n\t"
			"bl load_arg\n\t"
			"mov r3, r0\n\t"
		"arg_3:\n\t"
			"cmp r5, #3\n\t"
			"blt arg_2\n\t"
			"bl load_arg\n\t"
			"mov r2, r0\n\t"
		"arg_2:\n\t"
			"cmp r5, #2\n\t"
			"blt arg_1\n\t"
			"bl load_arg\n\t"
			"mov r1, r0\n\t"
		"arg_1:\n\t"
			"cmp r5, #1\n\t"
			"blt call_addr\n\t"
			"bl load_arg\n\t"
		"call_addr:\n\t"
			"blx r7\n\t"
			"add sp, #200\n\t"
			"str r0, [sp]\n\t"
			"pop {r0-r14}"
			:
			: [argc] "r" (argc), [argv] "r" (argv), [addr] "r" (addr)
		);
}


static std::deque<char*> mem_cache;
static pthread_mutex_t alloc_mutex = PTHREAD_MUTEX_INITIALIZER;
static char *alloc_trampo() {
	char *r;
	pthread_mutex_lock(&alloc_mutex);
	if (mem_cache.empty()) {
		size_t map_size = 10240 * 16;
		char *p = (char*)mmap(0, map_size, PROT_EXEC | PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		if (p == MAP_FAILED) {
			LOGD("mmap error : %s", strerror(errno));
			r = 0;
			goto end;
		}
		for (int i = 0; i < map_size / 16; i++) {
			mem_cache.push_back(p + i * 16);
		}
	}
	r = mem_cache.front();
	mem_cache.pop_front();
end:
	pthread_mutex_unlock(&alloc_mutex);
	return r;
}

static void emit_arm_jmp(void *_buf, void *_dst) {
	char *buf = (char*)_buf;
	uint32_t dst = (uint32_t)_dst;
	uint8_t jmp_code[] = { 0x04, 0xF0, 0x1F, 0xE5 };
	memcpy(buf, &jmp_code, 4);
	memcpy(buf + 4, &dst, 4);
}

int arm_hook(void *org, void *dst, void **trampo) {
	if (!org) {
		LOGE("err armhook org null");
		return 0;
	}
	if (!dst){
		LOGE("err armhook dst null");
		return 0;
	}
	if ((uint32_t)org%4!=0){
		LOGE("err armhook org addr is not arm opcode");
		return 0;
	}
	if (mprotect((void*)((uint32_t)org & ~(PAGE_SIZE - 1)), 8, PROT_EXEC | PROT_WRITE | PROT_READ) != 0) {
		LOGD("err armhook mprotect failed : %s", strerror(errno));
		return 0;
	}

	char *tr = alloc_trampo();
	if (tr == 0)
		return 0;
	memcpy(tr, org, 8);        /*提取原函数的前两个指令*/
	emit_arm_jmp(tr + 8, (void *)((int)org + 8));
	cache_flush((uint32_t)tr, (uint32_t)(tr + 16));
	if (trampo) *trampo = tr;
	emit_arm_jmp(org, dst);    /*修改原函数头*/
	cache_flush((uint32_t)org, (uint32_t)((int)org + 8));
	return 1;
}

int arm_unhook(void *org, void *tramp_addr)
{
	if (!org || !tramp_addr)
	{
		LOGD("err armunhook addr null");
		return 0;
	}

	memcpy(org, tramp_addr, 8);
	cache_flush((uint32_t)org, (uint32_t)((int)org + 8));
	return 1;
}
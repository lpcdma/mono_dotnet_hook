#ifndef	_H_ANDROID_INJECTOR_
#define _H_ANDROID_INJECTOR_

#include <stdio.h>
#include <asm/ptrace.h>

#if defined(__i386__)
#define pt_regs         user_regs_struct
#endif

#define CPSR_T_MASK     ( 1u << 5 )
 
class ANDROID_INJECTOR
{
public:
	int ptrace_readdata(pid_t pid,  uint8_t *src, uint8_t *buf, size_t size);
	int ptrace_writedata(pid_t pid, uint8_t *dest, uint8_t *data, size_t size);
	int ptrace_call(pid_t pid, uint32_t addr, long *params, uint32_t num_params, struct pt_regs* regs);
	long ptrace_call(pid_t pid, uint32_t addr, long *params, uint32_t num_params, struct user_regs_struct * regs);
	int ptrace_getregs(pid_t pid, struct pt_regs * regs);
	int ptrace_setregs(pid_t pid, struct pt_regs * regs);
	int ptrace_continue(pid_t pid);
	int ptrace_attach(pid_t pid);
	int ptrace_detach(pid_t pid);
	void* get_module_base(pid_t pid, const char* module_name);
	void* get_remote_addr(pid_t target_pid, const char* module_name, void* local_addr);
	int find_pid_of(const char *process_name);
	int find_injected_so_of(int ipid,char *psz_so_name);
	long ptrace_retval(struct pt_regs * regs);
	long ptrace_ip(struct pt_regs * regs);
	int ptrace_call_wrapper(pid_t target_pid, const char * func_name, void * func_addr, long * parameters, int param_num, struct pt_regs * regs);
	int inject_remote_process(pid_t target_pid, const char *library_path, const char *function_name, const char *param, size_t param_size, int bUnload);
	static int list_process(char * szprocess_list);
};
 

#endif
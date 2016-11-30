 #include <stdio.h>
#include <stdlib.h>
#include <asm/user.h>
#include <asm/ptrace.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <core/logger.h>
#include "android-injector.h"

#if defined(__i386__)
#define pt_regs         user_regs_struct
#endif

#define CPSR_T_MASK     ( 1u << 5 )

const char *libc_path ="/system/lib/libc.so" ;
const char *linker_path = "/system/bin/linker";
 

int ANDROID_INJECTOR::ptrace_readdata(pid_t pid,  uint8_t *src, uint8_t *buf, size_t size)
{
	uint32_t i, j, remain;
	uint8_t *laddr;

	union u {
		long val;
		char chars[sizeof(long)];
	} d;

	j = size / 4;
	remain = size % 4;

	laddr = buf;

	for (i = 0; i < j; i ++) {
		d.val = ptrace(PTRACE_PEEKTEXT, pid, src, 0);
		memcpy(laddr, d.chars, 4);
		src += 4;
		laddr += 4;
	}

	if (remain > 0) {
		d.val = ptrace(PTRACE_PEEKTEXT, pid, src, 0);
		memcpy(laddr, d.chars, remain);
	}

	return 0;
}

int ANDROID_INJECTOR::ptrace_writedata(pid_t pid, uint8_t *dest, uint8_t *data, size_t size)
{
	uint32_t i, j, remain;
	uint8_t *laddr;

	union u {
		long val;
		char chars[sizeof(long)];
	} d;

	j = size / 4;
	remain = size % 4;

	laddr = data;

	for (i = 0; i < j; i ++) {
		memcpy(d.chars, laddr, 4); 
		ptrace(PTRACE_POKETEXT, pid, dest, (void *)d.val);
		dest  += 4;
		laddr += 4;
	}

	if (remain > 0) {
		d.val = ptrace(PTRACE_PEEKTEXT, pid, dest, 0);
		for (i = 0; i < remain; i ++) {
			d.chars[i] = *laddr ++;
		}
 
		ptrace(PTRACE_POKETEXT, pid, dest, (void *)d.val);
	}

	return 0;
}

#if defined(__arm__)
int ANDROID_INJECTOR::ptrace_call(pid_t pid, uint32_t addr, long *params, uint32_t num_params, struct pt_regs* regs)
{
	uint32_t i;
	for (i = 0; i < num_params && i < 4; i ++) {
		regs->uregs[i] = params[i];
	}

	//
	// push remained params onto stack
	//
	if (i < num_params) {
		regs->ARM_sp -= (num_params - i) * sizeof(long) ;
		ptrace_writedata(pid, (uint8_t *)regs->ARM_sp, (uint8_t *)&params[i], (num_params - i) * sizeof(long));
	}

	regs->ARM_pc = addr;
	if (regs->ARM_pc & 1) {
		/* thumb */
		regs->ARM_pc &= (~1u);
		regs->ARM_cpsr |= CPSR_T_MASK;
	} else {
		/* arm */
		regs->ARM_cpsr &= ~CPSR_T_MASK;
	}

	regs->ARM_lr = 0;

	if (ptrace_setregs(pid, regs) == -1
		|| ptrace_continue(pid) == -1) {
			printf("error\n");
			return -1;
	}

	int stat = 0;
	waitpid(pid, &stat, WUNTRACED);
	while (stat != 0xb7f) {
		if (ptrace_continue(pid) == -1) {
			printf("error\n");
			return -1;
		}
		waitpid(pid, &stat, WUNTRACED);
	}

	return 0;
}

#elif defined(__i386__)
long ANDROID_INJECTOR::ptrace_call(pid_t pid, uint32_t addr, long *params, uint32_t num_params, struct user_regs_struct * regs)
{
	regs->esp -= (num_params) * sizeof(long) ;
	ptrace_writedata(pid, (void *)regs->esp, (uint8_t *)params, (num_params) * sizeof(long));

	long tmp_addr = 0x00;
	regs->esp -= sizeof(long);
	ptrace_writedata(pid, regs->esp, (char *)&tmp_addr, sizeof(tmp_addr));

	regs->eip = addr;

	if (ptrace_setregs(pid, regs) == -1
		|| ptrace_continue( pid) == -1) {
			printf("error\n");
			return -1;
	}

	int stat = 0;
	waitpid(pid, &stat, WUNTRACED);
	while (stat != 0xb7f) {
		if (ptrace_continue(pid) == -1) {
			printf("error\n");
			return -1;
		}
		waitpid(pid, &stat, WUNTRACED);
	}

	return 0;
}
#else
	#error "Not supported"
#endif

int ANDROID_INJECTOR::ptrace_getregs(pid_t pid, struct pt_regs * regs)
{
	if (ptrace(PTRACE_GETREGS, pid, NULL, regs) < 0) {
		perror("ptrace_getregs: Can not get register values");
		return -1;
	}

	return 0;
}

int ANDROID_INJECTOR::ptrace_setregs(pid_t pid, struct pt_regs * regs)
{
	if (ptrace(PTRACE_SETREGS, pid, NULL, regs) < 0) {
		perror("ptrace_setregs: Can not set register values");
		return -1;
	}

	return 0;
}

int ANDROID_INJECTOR::ptrace_continue(pid_t pid)
{
	if (ptrace(PTRACE_CONT, pid, NULL, 0) < 0) {
		perror("ptrace_cont");
		return -1;
	}

	return 0;
}

int ANDROID_INJECTOR::ptrace_attach(pid_t pid)
{
	if (ptrace(PTRACE_ATTACH, pid, NULL, 0) < 0) {
		perror("ptrace_attach");
		return -1;
	}

	int status = 0;
	waitpid(pid, &status , WUNTRACED);

	return 0;
}

int ANDROID_INJECTOR::ptrace_detach(pid_t pid)
{
	if (ptrace(PTRACE_DETACH, pid, NULL, 0) < 0) {
		perror("ptrace_detach");
		return -1;
	}

	return 0;
}

void* ANDROID_INJECTOR::get_module_base(pid_t pid, const char* module_name)
{
	FILE *fp;
	long addr = 0;
	char *pch;
	char filename[32];
	char line[1024];

	if (pid < 0) {
		/* self process */
		snprintf(filename, sizeof(filename), "/proc/self/maps");
	} else {
		snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
	}

	fp = fopen(filename, "r");

	if (fp != NULL) {
		while (fgets(line, sizeof(line), fp)) {
			if (strstr(line, module_name)) {
				pch = strtok( line, "-" );
				addr = strtoul( pch, NULL, 16 );

				if (addr == 0x8000)
					addr = 0;

				break;
			}
		}

		fclose(fp) ;
	}

	return (void *)addr;
}

void* ANDROID_INJECTOR::get_remote_addr(pid_t target_pid, const char* module_name, void* local_addr)
{
	void* local_handle, *remote_handle;

	local_handle = get_module_base(-1, module_name);
	remote_handle = get_module_base(target_pid, module_name);

	#ifdef DEBUG_MODE
	LOGD("[+] get_remote_addr: local[%x], remote[%x]\n", (unsigned int)local_handle, (unsigned int)remote_handle);
	#endif

	void * ret_addr = (void *)((uint32_t)local_addr + (uint32_t)remote_handle - (uint32_t)local_handle);

#if defined(__i386__)
	if (!strcmp(module_name, libc_path)) {
		ret_addr += 2;
	}
#endif
	return ret_addr;
}

int ANDROID_INJECTOR::find_pid_of(const char *process_name)
{
	int id;
	pid_t pid = -1;
	DIR* dir;
	FILE *fp;
	char filename[32];
	char cmdline[256];

	struct dirent * entry;

	if (process_name == NULL)
	{
		#ifdef DEBUG_MODE
		LOGD("[+] process_name == NULL\n");
		#endif

		return -1;
	}

	dir = opendir("/proc");
	if (dir == NULL)
	{
		#ifdef DEBUG_MODE
		LOGD("[+] dir == NULL\n");
		#endif

		return -1;
	}

	while((entry = readdir(dir)) != NULL) {
		id = atoi(entry->d_name);
		if (id != 0) {
			sprintf(filename, "/proc/%d/cmdline", id);
			fp = fopen(filename, "r");
			if (fp) {
				fgets(cmdline, sizeof(cmdline), fp);
				fclose(fp);

				if (strcmp(process_name, cmdline) == 0) {
					/* process found */
					#ifdef DEBUG_MODE
					LOGD("[+] process found %d\n",id);
					#endif
					
					pid = id;
					break;
				}
			}
		}
	}
	closedir(dir);
	return pid;
}


int ANDROID_INJECTOR::find_injected_so_of(int ipid,char *psz_so_name)
{ 
	FILE *fp;
	DIR* dir;
	char filename[32];
	char sz_so_name[1024];

	struct dirent * entry;

	dir = opendir("/proc");
	if (dir == NULL)
	{
		//#ifdef DEBUG_MODE
		LOGD("[+] dir == NULL\n");
		//#endif 
		return 0;
	} 
	if (psz_so_name == NULL)
	{
		#ifdef DEBUG_MODE
		LOGD("[+] psz_so_name == NULL\n");
		#endif 
		return 0;
	} 
 	LOGD("[+] pid=%d injected %s\n",ipid,psz_so_name);
	sprintf(filename, "/proc/%d/maps", ipid);
	fp = fopen(filename, "r");
	if (fp) 
	{ 
		while(NULL != fgets(sz_so_name, sizeof(sz_so_name), fp) )
		{ 
			if(strstr(sz_so_name,psz_so_name) > 0)
			{
				//#ifdef DEBUG_MODE
				printf("[+] injected so found %d\n",ipid);
				//#endif 	 
				fclose(fp); 
				return 1;
			}
			memset(sz_so_name,0,1024);
		} 
		fclose(fp); 
	}
	else
	{
		printf("open map file error:%s\n",filename);
	}
	return 0;
}

long ANDROID_INJECTOR::ptrace_retval(struct pt_regs * regs)
{
#if defined(__arm__)
	return regs->ARM_r0;
#elif defined(__i386__)
	return regs->eax;
#else
#error "Not supported"
#endif
}

long ANDROID_INJECTOR::ptrace_ip(struct pt_regs * regs)
{
#if defined(__arm__)
	return regs->ARM_pc;
#elif defined(__i386__)
	return regs->eip;
#else
#error "Not supported"
#endif
}

int ANDROID_INJECTOR::ptrace_call_wrapper(pid_t target_pid, const char * func_name, void * func_addr, long * parameters, int param_num, struct pt_regs * regs)
{
	#ifdef DEBUG_MODE
	LOGD("[+] Calling %s in target process.\n", func_name);
	#endif

	if (ptrace_call(target_pid, (uint32_t)func_addr, parameters, param_num, regs) == -1)
		return -1;

	if (ptrace_getregs(target_pid, regs) == -1)
		return -1;

	#ifdef DEBUG_MODE
	LOGD("[+] Target process returned from %s, return value=%x, pc=%x \n", func_name, (unsigned int)ptrace_retval(regs), (unsigned int)ptrace_ip(regs));
	#endif

	return 0;
}

int ANDROID_INJECTOR::inject_remote_process(pid_t target_pid, const char *library_path, const char *function_name, const char *param, size_t param_size, int bUnload)
{
	int ret = -1;
	void *mmap_addr, *dlopen_addr, *dlsym_addr, *dlclose_addr, *dlerror_addr;
	void *local_handle, *remote_handle, *dlhandle;
	uint8_t *map_base = 0;
	uint8_t *dlopen_param1_ptr, *dlsym_param2_ptr, *saved_r0_pc_ptr, *inject_param_ptr, *remote_code_ptr, *local_code_ptr;

	struct pt_regs regs, original_regs; 
	uint32_t code_length;
	long parameters[10];
 
	LOGD("[+] Injecting process: %d\n", target_pid);
  
	if (ptrace_attach(target_pid) == -1)
		return ret;

	if (ptrace_getregs(target_pid, &regs) == -1)
		return ret;

	/* save original registers */
	memcpy(&original_regs, &regs, sizeof(regs));

	/*get the real functions' addr by dlsym calling*/
	void *handle=dlopen("/system/lib/libc.so", RTLD_LAZY);
	void *pmmap = dlsym(handle, "mmap");
    dlclose(handle);
	mmap_addr = get_remote_addr(target_pid, libc_path, (void *)pmmap);
     
	LOGD("[+] Remote mmap address: %x\n", (unsigned int)mmap_addr);
  
	/* call mmap */
	parameters[0] = 0;  // addr
	parameters[1] = 0x4000; // size
	parameters[2] = PROT_READ | PROT_WRITE | PROT_EXEC;  // prot
	parameters[3] =  MAP_ANONYMOUS | MAP_PRIVATE; // flags
	parameters[4] = -1; //fd
	parameters[5] = 0; //offset

	if (ptrace_call_wrapper(target_pid, "mmap", mmap_addr, parameters, 6, &regs) == -1)
	{ 
		LOGD("[+] ptrace_call_wrapper show mmap error = LINE-%d\n",__LINE__); 
		return ret;
	}

	map_base = (uint8_t *)ptrace_retval(&regs);
    handle = 0;
    handle = dlopen("/system/lib/libdl.so", RTLD_LAZY);
    if (handle == NULL) 
    {
        LOGD("handle is NULL");
        return ret;
    }
    void * pdlopen = 0,*pdlsym = 0,*pdlclose = 0,*pdlerror = 0;

	pdlopen = dlsym(handle, "dlopen");
	pdlsym = dlsym(handle, "dlsym");
	pdlclose = dlsym(handle, "dlclose");
	pdlerror = dlsym(handle, "dlerror");

    LOGD("[+] pdlopen = %08X ,pdlsym = %08X,pdlclose = %08X,pdlerror = %08X", (int)pdlopen,(int)pdlsym,(int)pdlclose,(int)pdlerror );

	dlopen_addr = get_remote_addr( target_pid, linker_path, (void *)pdlopen ); 
	dlsym_addr = get_remote_addr( target_pid, linker_path, (void *)pdlsym ); 
	dlclose_addr = get_remote_addr( target_pid, linker_path, (void *)pdlclose ); 
	dlerror_addr = get_remote_addr( target_pid, linker_path, (void *)pdlerror );

    dlclose(handle);
 
	LOGD("[+] Get imports: dlopen: %x, dlsym: %x, dlclose: %x, dlerror: %x\n",(unsigned int)dlopen_addr, (unsigned int)dlsym_addr, (unsigned int)dlclose_addr, (unsigned int)dlerror_addr);
	LOGD("[+] library path = %s\n", library_path);
  
	ptrace_writedata(target_pid, map_base, (uint8_t *)library_path, strlen(library_path) + 1);

	parameters[0] = (long int)map_base;
	parameters[1] = RTLD_NOW| RTLD_GLOBAL;

	if (ptrace_call_wrapper(target_pid, "dlopen", dlopen_addr, parameters, 2, &regs) == -1)
	{ 
		LOGD("[+] ptrace_call_wrapper show dlopen error = LINE-%d\n",__LINE__);  
		return ret;
	}

	void *sohandle = (void *)ptrace_retval(&regs);
 
	LOGD("[+] sohandle = %p \n",sohandle); 

	if (function_name != NULL)
	{
		#define FUNCTION_NAME_ADDR_OFFSET       0x100
		ptrace_writedata(target_pid, map_base + FUNCTION_NAME_ADDR_OFFSET, (uint8_t *)function_name, strlen(function_name) + 1);
		parameters[0] = (long int)sohandle;
		parameters[1] = (long int)map_base + FUNCTION_NAME_ADDR_OFFSET;

		if (ptrace_call_wrapper(target_pid, "dlsym", dlsym_addr, parameters, 2, &regs) == -1)
		{
			LOGD("[+] ptrace_call_wrapper show dlsym error = LINE-%d\n", __LINE__);
			return ret;
		}

		void *hook_entry_addr = (void *)ptrace_retval(&regs);

		LOGD("[+] hook_entry_addr = %p\n", hook_entry_addr);

		#define FUNCTION_PARAM_ADDR_OFFSET      0x200
		ptrace_writedata(target_pid, map_base + FUNCTION_PARAM_ADDR_OFFSET, (uint8_t *)param, param_size); // change  strlen(param) + 1   to param_size so that compatible with different data type
		parameters[0] = (long int)map_base + FUNCTION_PARAM_ADDR_OFFSET;

		if (ptrace_call_wrapper(target_pid, function_name, hook_entry_addr, parameters, 1, &regs) == -1)
		{
			LOGD("[+] ptrace_call_wrapper show hook_entry error = LINE-%d\n", __LINE__);
			return ret;
		}
	}

	if (bUnload==1)
	{
		parameters[0] = (long int)sohandle;

		if (ptrace_call_wrapper(target_pid, "dlclose", (void *)dlclose, (long int *)parameters, 1, &regs) == -1)
		{
			LOGD("[+] ptrace_call_wrapper show dlclose error = LINE-%d\n",__LINE__);
			return ret;
		}
	}

	/* restore */
	ptrace_setregs(target_pid, &original_regs);
	ptrace_detach(target_pid);
	
	ret = 1; 
	return ret;
} 

// list current running process by travelling /proc/<pid>/cmdline directory
int ANDROID_INJECTOR::list_process(char * szprocess_list)
{
	int id;
	pid_t pid = -1;
	DIR* dir;
	FILE *fp;
	char filename[32];
	
	
	struct dirent * entry; 
	dir = opendir("/proc");
	if (dir == NULL)
	{
		printf("dir == NULL\n");
		return -1;
	}
	
	int ilistLen = 0;
	char * pStart = szprocess_list;
	while((entry = readdir(dir)) != NULL) {
		id = atoi(entry->d_name);
		if (id != 0)
		 {
			sprintf(filename, "/proc/%d/cmdline", id);
			fp = fopen(filename, "r");
			if (fp) 
			{
				char cmdline[256] = {0};
				fgets(cmdline, sizeof(cmdline), fp);
				fclose(fp); 
				if (strlen(cmdline) <= 0)
				{
					continue;
				}
				char szProcessInfo[512]={0};
				sprintf(szProcessInfo, "[%d]%s|", id,cmdline); 
				//printf("%d---%s\n",strlen(cmdline), szProcessInfo); 
				memcpy(pStart,szProcessInfo,sizeof(szProcessInfo));
				pStart +=strlen(szProcessInfo);
			}
		}
	} 
	closedir(dir);
	return pid;
}
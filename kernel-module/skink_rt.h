/*
 * Copyright (C) 2011-2013  Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * The research leading to these results has received funding from
 * the European Commission's Seventh Framework Programme (FP7) under
 * Grant Agreement n. 231500 (ROBOSKIN).
 *
 * This file is part of Skinware.
 *
 * Skinware is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Skinware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Skinware.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SKINK_RT_H
#define SKINK_RT_H

// This header is shared between the kernel module and the user-lib

#ifdef SKIN_DS_ONLY	// For remote modules, such as ROSSkin users
# include <stdio.h>
# include <stdbool.h>
typedef int64_t skin_rt_time;
#else

#include <rtai.h>
#ifndef __KERNEL__
# include <rtai_lxrt.h>
#endif
#include <rtai_sched.h>
#include <rtai_sem.h>
#include <rtai_rwl.h>
#include <rtai_shm.h>
#include <rtai_registry.h>
#include <rtai_nam2num.h>
#ifdef __KERNEL__
# include <linux/proc_fs.h>
# include <linux/semaphore.h>
# include <linux/string.h>
#else
# include <stdio.h>
# include <stdbool.h>
#endif

// TODO: inspect whether NULL is more appropriate for rt_register or "current".  Or if there is any difference in them, or if there are cases one would
// fail and the other not!  Note that "current" is something defined by the kernel itself (not RTAI)

// Note: These are return values of skin_rt* functions.  For now, they correspond exactly to RTAI and the values are taken from RTAI documentation
// However, be wary that if RTAI changes these outputs, or if another rt library is used, these values must change accordingly.
// TODO: check the output of RTAI functions and generate these outputs as opposed to returning RTAI outputs directly and counting on them being equal
// TODO: When writing documentation, also put comments on the output of each function in this file (read from RTAI's documentation)
#define					SKIN_RT_SUCCESS					0
#define					SKIN_RT_FAIL					!SKIN_RT_SUCCESS
#define					SKIN_RT_INVALID					EINVAL
#define					SKIN_RT_NO_MEM					ENOMEM
#define					SKIN_RT_TIMEOUT					SEM_TIMOUT
#define					SKIN_RT_SYNC_MECHANISM_ERROR			SEM_ERR
#define					SKIN_RT_LOCK_NOT_ACQUIRED			-1
#define					SKIN_RT_INVALID_SEM				0xffff
#define					SKIN_RT_INVALID_MUTEX				SKIN_RT_INVALID_SEM
#define					SKIN_RT_INVALID_RWLOCK				SKIN_RT_INVALID_SEM

#ifdef __KERNEL__
#define					SKIN_RT_FLAG_STARTED_TIMER			0x0001
#define					SKIN_RT_FLAG_CREATED_PROC			0x0002

#define					SKIN_RT_GET_FREE_NAME_LOCK			"SKNRTP"	// Note: this name is used to register a linux semaphore
													// with RTAI.  This is first used with skin_rt_init
													// to know if the /proc file is created or not
													// It is then used by skin_rt_get_free_name to produce
													// unique names
#define					SKIN_RT_GET_FREE_NAME_SHMEM			"SKNRTM"	// This shared memory is used by skin_rt_get_free_name to
													// advance the free-name id in all modules using skin_rt
													// so that once skin_rt_get_free_name returns a name, no one
													// else could get that name, even if the name is never used
													// this removes the problem of another module getting the same
													// name before the first module could register it
#endif
#define					SKIN_RT_PROC_FS_NAME				"skin_rt_free_name"

#define					SKIN_RT_MIN_PRIORITY				RT_SCHED_LOWEST_PRIORITY
#define					SKIN_RT_MAX_PRIORITY				RT_SCHED_HIGHEST_PRIORITY
#define					SKIN_RT_LINUX_PRIORITY				RT_SCHED_LINUX_PRIORITY
// NOTE: In RTAI, lower priority value means higher priority.  This may not be true with other future realtime extensions.  Therefore, to get higher
// priority, add the priority by SKIN_RT_MORE_PRIORITY.  To check to see if the obtained priority is valid, do NOT check against
// SKIN_RT_MIN_PRIORITY and SKIN_RT_MAX_PRIORITY, but use skin_rt_priority_is_valid function.
#define					SKIN_RT_MORE_PRIORITY				-1

#define					SKIN_RT_MAX_NAME_LENGTH				6

#define					SKIN_RT_STRINGIFY_(x)				#x
#define					SKIN_RT_STRINGIFY(x)				SKIN_RT_STRINGIFY_(x)

#ifdef __KERNEL__

# include "skink_common.h"

# if					SKINK_DEBUG
#  define				SKINK_RT_LOG(x, ...)				rt_printk(KERN_INFO SKINK_MODULE_NAME": "FILE_LINE x"\n",\
														END_OF_FILE_PATH, ##__VA_ARGS__)
#  define				SKINK_RT_DBG(x, ...)				SKINK_RT_LOG(x, ##__VA_ARGS__)
# else
#  define				SKINK_RT_LOG(x, ...)				rt_printk(KERN_INFO SKINK_MODULE_NAME": "x"\n", ##__VA_ARGS__)
#  define				SKINK_RT_DBG(x, ...)				((void)0)
# endif
#endif

typedef RTIME				skin_rt_time;			// Always in nanoseconds
typedef RT_TASK				skin_rt_task;
#ifndef __KERNEL__
typedef pthread_t			skin_rt_task_id;
#endif
typedef SEM				skin_rt_semaphore;
typedef SEM				skin_rt_mutex;
typedef RWL				skin_rt_rwlock;			// Multi-reader single-writer lock

static inline int skin_rt_get_free_name(char *name)
{
#ifdef __KERNEL__
	unsigned long *next_free_name = rt_shm_alloc(nam2num(SKIN_RT_GET_FREE_NAME_SHMEM), 0, USE_VMALLOC);
	static unsigned long next = 0;
	struct semaphore *sem = rt_get_adr(nam2num(SKIN_RT_GET_FREE_NAME_LOCK));
	if (sem)
		if (down_interruptible(sem))
			return SKIN_RT_FAIL;
	if (next_free_name == NULL)
		next = nam2num("THREAD");
	else
		next = *next_free_name;
	while (rt_get_adr(next++));	// Note: with RTAI, this will go like THREAD, THREAE, THREAF, THREAG etc which here is fine
					// However, in general this may not be the case, so this operation is not so safe and can cause
					// name clash.  If RTAI was not used, manually change the name.
					// skip the names that are not available
	if (next_free_name != NULL)
	{
		*next_free_name = next;
		rt_shm_free(nam2num(SKIN_RT_GET_FREE_NAME_SHMEM));
	}
	if (sem)
		up(sem);
	num2nam(next - 1, name);
	return SKIN_RT_SUCCESS;
#else
	FILE *fin = fopen("/proc/"SKIN_RT_PROC_FS_NAME, "r");
	if (fin)
	{
		int ret = fscanf(fin, "%"SKIN_RT_STRINGIFY(SKIN_RT_MAX_NAME_LENGTH)"s", name);
		fclose(fin);
		if (ret != 1)
			return SKIN_RT_FAIL;
		return SKIN_RT_SUCCESS;
	}
	else
	{
		// If the /proc file can't be open, it means that no real-time kernel modules (who have called skin_rt_init) exist, or the one creating the /proc
		// file has exited.  This normally shouldn't happen, so this is an error case.  Just search for a free name and hope everything goes well
		unsigned long last_num = nam2num("XXXXXX") + rand() % 100000;	// start with a name far from THREAD so it wouldn't cause clash with kernel's
										// also, +rand() so that if two threads both couldn't open /proc, at least chances
										// would be small that they get the same name
		int i;
		for (i = 0; i < 100000; ++i)					// try a 10000 more or less random ids for this thread
				if (!rt_get_adr((last_num += rand() % 50)))	// take random steps in hope that the same name wouldn't be produced
					break;
		if (i == 100000)
			return SKIN_RT_FAIL;
		num2nam(last_num, name);
		return SKIN_RT_SUCCESS;
	}
#endif
}

#ifdef __KERNEL__
// Note: the following variable/function (starting with the_skin_rt_) are only meaningful for the module creating the procfs file which should be only one.
static struct semaphore the_skin_rt_get_free_name_mutex;

static int the_skin_rt_procfs_read(char *page, char **start, off_t offset, int count, int *eof, void *data)
{
	char name[SKIN_RT_MAX_NAME_LENGTH + 1];
	int length;
	if (offset > 0)		// even though I set *eof = 1, the kernel is still calling me, so return 0 for it to stop
		return 0;
	if (skin_rt_get_free_name(name) != SKIN_RT_SUCCESS)
		return 0;
	length = strlen(name);
	if (count < length + 1)
	{
		SKINK_LOG("error: get free name: page size provided is too small");
		return -EFAULT;
	}
	sprintf(page + offset, "%s", name);	// no need for copy_to_user as it is done by the kernel.  See definition of __proc_file_read here:
						// http://lxr.linux.no/#linux+v3.3.1/fs/proc/generic.c#L42
	*eof = 1;
	return length + 1;
}
#endif

static inline bool skin_rt_priority_is_valid(int priority)
{
	return (priority <= RT_SCHED_LOWEST_PRIORITY && priority >= RT_SCHED_HIGHEST_PRIORITY);
}

#ifdef __KERNEL__
static inline skin_rt_time skin_rt_init(skin_rt_time tick_period, int *stop_flags)	// in nanoseconds
{
	skin_rt_time ret = tick_period;
	*stop_flags = 0;
	if (!rt_is_hard_timer_running())
	{
		rt_set_oneshot_mode();		// Note: according to the RTAI documentation, in oneshot mode the tick_period is ignored
		*stop_flags |= SKIN_RT_FLAG_STARTED_TIMER;
		ret = count2nano(start_rt_timer(nano2count(tick_period)));
	}
	if (!rt_get_adr(nam2num(SKIN_RT_GET_FREE_NAME_LOCK)))		// Hopefully, no skin_rt_init would be called between this line and rt_register
	{
		struct proc_dir_entry *proc_entry;
		unsigned long *next_free_name;
		sema_init(&the_skin_rt_get_free_name_mutex, 1);
		rt_register(nam2num(SKIN_RT_GET_FREE_NAME_LOCK), &the_skin_rt_get_free_name_mutex, IS_SEM, current);
		next_free_name = rt_shm_alloc(nam2num(SKIN_RT_GET_FREE_NAME_SHMEM), sizeof(*next_free_name), USE_VMALLOC);
		if (next_free_name != NULL)
			*next_free_name = nam2num("THREAD");
		proc_entry = create_proc_entry(SKIN_RT_PROC_FS_NAME, 0400, NULL);
		if (proc_entry == NULL)
			remove_proc_entry(SKIN_RT_PROC_FS_NAME, NULL);
		else
		{
			proc_entry->read_proc	= the_skin_rt_procfs_read;
			proc_entry->mode	= S_IFREG | S_IRUGO;
			proc_entry->uid		= 0;
			proc_entry->gid		= 0;
			proc_entry->size	= 0;
			*stop_flags |= SKIN_RT_FLAG_CREATED_PROC;
		}
	}
	return ret;	// TODO: if timer was started, then the tick_period is set by someone else, how to retrieve it?
}
#else
static inline int skin_rt_init(void)
{
	if (!rt_is_hard_timer_running())
	{
		rt_set_oneshot_mode();
		start_rt_timer(0);
	}
	if (rt_buddy() == NULL)		// if not, then already an RT thread
	{
		char name[SKIN_RT_MAX_NAME_LENGTH + 1];
		if (skin_rt_get_free_name(name) != SKIN_RT_SUCCESS)
			return SKIN_RT_FAIL;
		if (!rt_thread_init(nam2num(name), SKIN_RT_LINUX_PRIORITY, 0, SCHED_FIFO, 0xf))	// possible to run on CPUS 0 through 15
												// Since it's initializing main, set it to
												// have linux priority
			return SKIN_RT_FAIL;
		mlockall(MCL_CURRENT | MCL_FUTURE);
	}
	return SKIN_RT_SUCCESS;
}
#endif

static inline void skin_rt_stop(
#ifdef __KERNEL__
		int stop_flags
#else
		void
#endif
		)
{
#ifdef __KERNEL__
	if (stop_flags & SKIN_RT_FLAG_STARTED_TIMER)
		stop_rt_timer();
	if (stop_flags & SKIN_RT_FLAG_CREATED_PROC)
	{
		remove_proc_entry(SKIN_RT_PROC_FS_NAME, NULL);
		rt_drg_on_name(nam2num(SKIN_RT_GET_FREE_NAME_LOCK));
		if (rt_get_adr(nam2num(SKIN_RT_GET_FREE_NAME_SHMEM)))
			rt_shm_free(nam2num(SKIN_RT_GET_FREE_NAME_SHMEM));
	}
#else
	rt_make_soft_real_time();
	if (rt_buddy() != NULL)		// if not, then it's not an RT thread
		rt_task_delete(NULL);	// kill the task buddy of main
#endif
}

// NOTE: Although the contents of the following functions are similar, I prefer to have them with different names, i.e. Kernel_task and User_task
// so that the programmer would be at all times aware of which side he is working in and also to make migration from kernel to user (or vice versa)
// more than just a copy-paste
#ifdef __KERNEL__
static inline int skin_rt_kernel_task_init(skin_rt_task *task, void (*function)(long int), int data, int stack_size, int priority)
{
	return rt_task_init(task, function, data, stack_size, priority, 0, NULL);
}

static inline int skin_rt_kernel_task_init_detailed(skin_rt_task *task, void (*function)(long int), int data, int stack_size, int priority,
		int uses_fpu, void (*signal)(void))
{
	return rt_task_init(task, function, data, stack_size, priority, uses_fpu, signal);
}

static inline int skin_rt_kernel_task_resume(skin_rt_task *task)
{
	return rt_task_resume(task);
}

static inline int skin_rt_kernel_task_suspend(skin_rt_task *task)
{
	return rt_task_suspend(task);
}

static inline void skin_rt_kernel_task_wait_period(void)
{
	rt_task_wait_period();
}

static inline int skin_rt_kernel_task_make_periodic(skin_rt_task *task, skin_rt_time start_time, skin_rt_time period)
				// Times in nanoseconds
{
	return rt_task_make_periodic(task, nano2count(start_time), nano2count(period));
}

static inline int skin_rt_kernel_task_make_periodic_relative(skin_rt_task *task, skin_rt_time delay_time, skin_rt_time period)
				// Times in nanoseconds
{
	return rt_task_make_periodic_relative_ns(task, delay_time, period);
}

static inline int skin_rt_kernel_task_delete(skin_rt_task *task)
{
	return rt_task_delete(task);
}

// The following two functions are to introduce symmetry between kernel and user space thread creation.  Also, for possible future introduction of a need
// for such functions.  This function MUST be called upon start of real-time kernel thread.
static inline int skin_rt_kernel_task_on_start(void)
{
	return SKIN_RT_SUCCESS;
}

// This function MUST be called upon start of real-time kernel thread.
static inline int skin_rt_kernel_task_on_stop(void)
{
	return SKIN_RT_SUCCESS;
}

#else // then !defined(__KERNEL__)

// NOTE: RTAI suddenly uses pthread style creation/join/etc of threads in LXRT!
// For now, skin_rt would also do like that, but if changing away from RTAI was even on option, then these interfaces should become
// more similar to the kernel side ones
static inline skin_rt_task_id skin_rt_user_task_init(void *(*fun)(void *), void *data, int stack_size)
{
	return rt_thread_create((void *)fun, data, stack_size);
}

static inline int skin_rt_user_task_join(skin_rt_task_id tid)
{
	return rt_thread_join(tid);		// TODO: what does rt_thread_join return? Same as pthread_join? If so, then it's fine
}

static inline int skin_rt_user_task_resume(skin_rt_task *task)
{
	return rt_task_resume(task);
}

static inline int skin_rt_user_task_suspend(skin_rt_task *task)
{
	return rt_task_suspend(task);
}

static inline void skin_rt_user_task_wait_period(void)
{
	rt_task_wait_period();
}

static inline int skin_rt_user_task_make_periodic(skin_rt_task *task, skin_rt_time start_time, skin_rt_time period)		// Times in nanoseconds
{
	return rt_task_make_periodic(task, nano2count(start_time), nano2count(period));
}

static inline int skin_rt_user_task_make_periodic_relative(skin_rt_task *task, skin_rt_time delay_time, skin_rt_time period)	// Times in nanoseconds
{
	return rt_task_make_periodic_relative_ns(task, delay_time, period);
}

static inline int skin_rt_user_task_delete(skin_rt_task *task)
{
	if (!task)
		rt_make_soft_real_time();
	return rt_task_delete(task);
}

static inline void skin_rt_user_task_make_hard_real_time(void)
{
	rt_make_hard_real_time();
}

static inline void skin_rt_user_task_make_soft_real_time(void)
{
	rt_make_soft_real_time();
}

// This function MUST be called upon start of real-time user thread.
// returns the task created for this user
static inline skin_rt_task *skin_rt_user_task_on_start(int priority)
{
	skin_rt_task *t = NULL;
	if (rt_buddy() == NULL)		// if not, then already an RT thread
	{
		char name[SKIN_RT_MAX_NAME_LENGTH + 1];
		if (skin_rt_get_free_name(name) != SKIN_RT_SUCCESS)
			return NULL;
		if ((t = rt_thread_init(nam2num(name), priority, 0, SCHED_FIFO, 0xf)))	// possible to run on CPUS 0 through 15
			mlockall(MCL_CURRENT | MCL_FUTURE);
	}
	return t;
}

// This function MUST be called before end of real-time user thread.
static inline int skin_rt_user_task_on_stop(void)
{
	if (rt_buddy() != NULL)		// if not, then it's not an RT thread
	{
		rt_make_soft_real_time();
		return rt_task_delete(NULL);
	}
	else
		return SKIN_RT_FAIL;
}

#endif // __KERNEL__

static inline skin_rt_task *skin_rt_get_task(void)
{
#ifdef __KERNEL__
	return rt_whoami();
#else
	return rt_buddy();
#endif
}

static inline bool skin_rt_is_rt_context(void)
{
	RT_TASK *t = skin_rt_get_task();
#ifdef __KERNEL__
	return t->is_hard > 0;
#else
	return t != NULL;
#endif
}

static inline skin_rt_time skin_rt_get_time(void)	// returns time in nano seconds
{
	return rt_get_time_ns();
}

static inline skin_rt_time skin_rt_get_exectime(void)	// returns execution time in nano seconds
{
	RT_TASK *task = skin_rt_get_task();
#ifdef __KERNEL__
	if (task == NULL)
		return 0;				// if not real-time refuse to work
	return count2nano(task->exectime[0]);
#else
	skin_rt_time exectime[3];
	if (task == NULL)
		return 0;				// if not real-time refuse to work
	rt_get_exectime(task, exectime);
	return count2nano(exectime[0]);
#endif
}

#ifdef __KERNEL__
static inline skin_rt_time skin_rt_next_period(void)
{
	return count2nano(next_period());
}

static inline skin_rt_time skin_rt_period_time_left(void)
{
	return count2nano(next_period()) - rt_get_time_ns();
}
#endif

static inline void skin_rt_sleep(skin_rt_time t)
{
	rt_sleep(nano2count(t));
}

static inline void skin_rt_sleep_until(skin_rt_time t)
{
	rt_sleep_until(nano2count(t));
}

// NOTE: The kernel and user space behavior of this function in RTAI are different!
// In kernel space, acquire your semaphore and send its address through sem.  It will be initialized
// In user space, you can ignore parameter sem (send NULL for example) and the new semaphore's address will be returned to you
// In both cases, the return value of skin_rt_sem_init is the semaphore you should use.
static inline skin_rt_semaphore *skin_rt_sem_init(skin_rt_semaphore *sem, int initial_value)
{
#ifdef __KERNEL__
	rt_typed_sem_init(sem, initial_value, CNT_SEM);
	return sem;
#else
	return rt_typed_sem_init(0, initial_value, CNT_SEM);
#endif
}

static inline int skin_rt_sem_delete(skin_rt_semaphore *sem)
{
#ifdef __KERNEL__
	return rt_sem_delete(sem);
#else
	int ret = rt_sem_delete(sem);
	return ret > 0?SKIN_RT_SUCCESS:ret == SKIN_RT_INVALID_SEM?SKIN_RT_INVALID_SEM:SKIN_RT_FAIL;
#endif
}

static inline int skin_rt_sem_wait(skin_rt_semaphore *sem)
{
	return (rt_sem_wait(sem) == 0xffff)?SKIN_RT_INVALID_SEM:SKIN_RT_SUCCESS;
}

static inline int skin_rt_sem_try_wait(skin_rt_semaphore *sem)
{
	int res = rt_sem_wait_if(sem);
	return (res == 0xffff)?SKIN_RT_INVALID_SEM:(res == 0)?SKIN_RT_LOCK_NOT_ACQUIRED:SKIN_RT_SUCCESS;
}

static inline int skin_rt_sem_timed_wait(skin_rt_semaphore *sem, skin_rt_time t)
{
	int ret = rt_sem_wait_timed(sem, nano2count(t));
	return (ret == 0xffff)?SKIN_RT_INVALID_SEM:(ret == SKIN_RT_TIMEOUT)?SKIN_RT_TIMEOUT:SKIN_RT_SUCCESS;
}

static inline int skin_rt_sem_post(skin_rt_semaphore *sem)
{
	return rt_sem_signal(sem);
}

static inline int skin_rt_sem_broadcast(skin_rt_semaphore *sem)
{
	return rt_sem_broadcast(sem);
}

// NOTE: The kernel and user space behavior of this function in RTAI are different!
// In kernel space, acquire your mutex and send its address through sem.  It will be initialized
// In user space, you can ignore parameter mutex (send NULL for example) and the new mutex's address will be returned to you
// In both cases, the return value of skin_rt_mutex_init is the mutex you should use.
static inline skin_rt_mutex *skin_rt_mutex_init(skin_rt_mutex *mutex, int initial_value)
{
#ifdef __KERNEL__
	rt_typed_sem_init(mutex, initial_value, BIN_SEM);
	return mutex;
#else
	return rt_typed_sem_init(0, initial_value, BIN_SEM);
#endif
}

static inline int skin_rt_mutex_delete(skin_rt_mutex *mutex)
{
#ifdef __KERNEL__
	return rt_sem_delete(mutex);
#else
	int ret = rt_sem_delete(mutex);
	return ret > 0?SKIN_RT_SUCCESS:ret == SKIN_RT_INVALID_MUTEX?SKIN_RT_INVALID_MUTEX:SKIN_RT_FAIL;
#endif
}

static inline int skin_rt_mutex_lock(skin_rt_mutex *mutex)
{
	return (rt_sem_wait(mutex) == 0xffff)?SKIN_RT_INVALID_SEM:SKIN_RT_SUCCESS;
}

static inline int skin_rt_mutex_try_lock(skin_rt_mutex *mutex)
{
	int res = rt_sem_wait_if(mutex);
	return (res == 0xffff)?SKIN_RT_INVALID_MUTEX:(res == 0)?SKIN_RT_LOCK_NOT_ACQUIRED:SKIN_RT_SUCCESS;
}

static inline int skin_rt_mutex_timed_lock(skin_rt_mutex *mutex, skin_rt_time t)
{
	int ret = rt_sem_wait_timed(mutex, nano2count(t));
	return (ret == 0xffff)?SKIN_RT_INVALID_SEM:(ret == SKIN_RT_TIMEOUT)?SKIN_RT_TIMEOUT:SKIN_RT_SUCCESS;
}

static inline int skin_rt_mutex_unlock(skin_rt_mutex *mutex)
{
	return rt_sem_signal(mutex);
}

static inline int skin_rt_mutex_broadcast(skin_rt_mutex *mutex)
{
	return rt_sem_broadcast(mutex);
}

// NOTE: The kernel and user space behavior of this function in RTAI are different!
// In kernel space, acquire your rwl and send its address through sem.  It will be initialized
// In user space, you can ignore parameter sem (send NULL for example) and the new rwl's address will be returned to you
// In both cases, the return value of skin_rt_rwlock_init is the rwl you should use.
static inline skin_rt_rwlock *skin_rt_rwlock_init(skin_rt_rwlock *rwl)
{
#ifdef __KERNEL__
	rt_rwl_init(rwl);
	return rwl;
#else
	return rt_rwl_init(0);
#endif
}

static inline int skin_rt_rwlock_delete(skin_rt_rwlock *rwl)
{
#ifdef __KERNEL__
	return rt_rwl_delete(rwl) == 0?SKIN_RT_SUCCESS:SKIN_RT_INVALID_RWLOCK;
#else
	int ret = rt_rwl_delete(rwl);
	return ret > 0?SKIN_RT_SUCCESS:ret == SKIN_RT_SYNC_MECHANISM_ERROR?SKIN_RT_INVALID_RWLOCK:SKIN_RT_FAIL;
#endif
}

static inline int skin_rt_rwlock_read_lock(skin_rt_rwlock *rwl)
{
	return rt_rwl_rdlock(rwl);
}

static inline int skin_rt_rwlock_timed_read_lock(skin_rt_rwlock *rwl, skin_rt_time t)
{
	return rt_rwl_rdlock_timed(rwl, nano2count(t));
}

static inline int skin_rt_rwlock_try_read_lock(skin_rt_rwlock *rwl)
{
	// if it didn't work, try rt_rwl_rdlock_timed(rwl, 0);
	return rt_rwl_rdlock_if(rwl);
}

static inline int skin_rt_rwlock_write_lock(skin_rt_rwlock *rwl)
{
	return rt_rwl_wrlock(rwl);
}

static inline int skin_rt_rwlock_timed_write_lock(skin_rt_rwlock *rwl, skin_rt_time t)
{
	return rt_rwl_wrlock_timed(rwl, nano2count(t));
}

static inline int skin_rt_rwlock_try_write_lock(skin_rt_rwlock *rwl)
{
	return rt_rwl_wrlock_if(rwl);
}

static inline void skin_rt_rwlock_read_unlock(skin_rt_rwlock *rwl)
{
	rt_rwl_unlock(rwl);
}

static inline void skin_rt_rwlock_write_unlock(skin_rt_rwlock *rwl)
{
	rt_rwl_unlock(rwl);
}

// NOTE: In RTAI, memory allocation is not allowed in RT tasks, so make sure you get all your memory before starting RT tasks.
// TODO: Apparently it is possible, if rtai_malloc.ko is built.  But for now, lets just preallocated everything.

// Note: The first SKIN_RT_MEMORY_MAX_NAME_LENGTH characters of "name" will be used as shared memory name (True for all shared memory functions)
static inline void *skin_rt_shared_memory_alloc(const char *name, size_t size, bool *is_contiguous)
{
	void *res;
	if (rt_get_adr(nam2num(name)))				// If not NULL, then name is in use
		return NULL;
	res = rt_shm_alloc(nam2num(name), size, USE_GFP_KERNEL);
	if (is_contiguous)
		*is_contiguous = true;
	if (!res)
	{
		res = rt_shm_alloc(nam2num(name), size, USE_VMALLOC);
		if (is_contiguous)
			*is_contiguous = false;
	}
	return res;
}

static inline void *skin_rt_shared_memory_attach(const char *name)
{
	return rt_shm_alloc(nam2num(name), 0, USE_VMALLOC);	// When attaching, these values are ignored.
								// If not allocated before, then should (I hope) return NULL
}

static inline int skin_rt_shared_memory_free(const char *name)
{
	return (rt_shm_free(nam2num(name)) == 0)?SKIN_RT_FAIL:SKIN_RT_SUCCESS;
}

// _detach is the same as free, so if the last attached process detaches from it, it will be deleted too and skin_rt_shared_memory_attach
// cannot be used after.
static inline int skin_rt_shared_memory_detach(const char *name)
{
	return (rt_shm_free(nam2num(name)) == 0)?SKIN_RT_FAIL:SKIN_RT_SUCCESS;
}

#ifdef __KERNEL__		// in RTAI rt_register and rt_drg_on_name only exist in kernel mode

static inline int skin_rt_make_name_available(const char *name)
{
	return (rt_drg_on_name(nam2num(name)) != 0)?SKIN_RT_SUCCESS:SKIN_RT_FAIL;
}

static inline int skin_rt_share_semaphore(const char *name, skin_rt_semaphore *sem)
{
	return (rt_register(nam2num(name), sem, IS_SEM, current) == 0)?SKIN_RT_FAIL:SKIN_RT_SUCCESS;
}

static inline int skin_rt_share_mutex(const char *name, skin_rt_mutex *mutex)
{
	return (rt_register(nam2num(name), mutex, IS_SEM, current) == 0)?SKIN_RT_FAIL:SKIN_RT_SUCCESS;
}

static inline int skin_rt_share_rwlock(const char *name, skin_rt_rwlock *rwl)
{
	return (rt_register(nam2num(name), rwl, IS_RWL, current) == 0)?SKIN_RT_FAIL:SKIN_RT_SUCCESS;
}

static inline int skin_rt_unshare_semaphore(const char *name)
{
	return (rt_drg_on_name(nam2num(name)) == 0)?SKIN_RT_FAIL:SKIN_RT_SUCCESS;
}

static inline int skin_rt_unshare_mutex(const char *name)
{
	return (rt_drg_on_name(nam2num(name)) == 0)?SKIN_RT_FAIL:SKIN_RT_SUCCESS;
}

static inline int skin_rt_unshare_rwlock(const char *name)
{
	return (rt_drg_on_name(nam2num(name)) == 0)?SKIN_RT_FAIL:SKIN_RT_SUCCESS;
}

#endif // __KERNEL__

// See note on skin_rt_sem_init
static inline skin_rt_semaphore *skin_rt_sem_init_and_share(skin_rt_semaphore *sem, int initial_value, const char *name)
{
#ifdef __KERNEL__
	if (skin_rt_sem_init(sem, initial_value) == NULL)
		return NULL;
	if (rt_register(nam2num(name), sem, IS_SEM, current) == 0)
	{
		skin_rt_sem_delete(sem);
		return NULL;
	}
	return sem;
#else
	return rt_typed_named_sem_init(name, initial_value, CNT_SEM);
#endif
}

// See note on skin_rt_mutex_init
static inline skin_rt_mutex *skin_rt_mutex_init_and_share(skin_rt_mutex *mutex, int initial_value, const char *name)
{
#ifdef __KERNEL__
	if (skin_rt_mutex_init(mutex, initial_value) == NULL)
		return NULL;
	if (rt_register(nam2num(name), mutex, IS_SEM, current) == 0)
	{
		skin_rt_mutex_delete(mutex);
		return NULL;
	}
	return mutex;
#else
	return rt_typed_named_sem_init(name, initial_value, BIN_SEM);
#endif
}

// See note on skin_rt_rwlock_init
static inline skin_rt_rwlock *skin_rt_rwlock_init_and_share(skin_rt_rwlock *rwl, const char *name)
{
#ifdef __KERNEL__
	if (skin_rt_rwlock_init(rwl) == NULL)
		return NULL;
	if (rt_register(nam2num(name), rwl, IS_RWL, current) == 0)
	{
		skin_rt_rwlock_delete(rwl);
		return NULL;
	}
	return rwl;
#else
	return rt_named_rwl_init(name);
#endif
}

static inline int skin_rt_sem_unshare_and_delete(skin_rt_semaphore *sem)
{
#ifdef __KERNEL__
	rt_drg_on_adr(sem);
	return skin_rt_sem_delete(sem);
#else
	return (rt_named_sem_delete(sem) < 0)?SKIN_RT_FAIL:SKIN_RT_SUCCESS;
#endif
}

static inline int skin_rt_mutex_unshare_and_delete(skin_rt_mutex *mutex)
{
#ifdef __KERNEL__
	rt_drg_on_adr(mutex);
	return skin_rt_mutex_delete(mutex);
#else
	return (rt_named_sem_delete(mutex) < 0)?SKIN_RT_FAIL:SKIN_RT_SUCCESS;
#endif
}

static inline int skin_rt_rwlock_unshare_and_delete(skin_rt_rwlock *rwl)
{
#ifdef __KERNEL__
	rt_drg_on_adr(rwl);
	return skin_rt_rwlock_delete(rwl);
#else
	return (rt_named_rwl_delete(rwl) < 0)?SKIN_RT_FAIL:SKIN_RT_SUCCESS;
#endif
}

static inline skin_rt_semaphore *skin_rt_get_shared_semaphore(const char *name)
{
	return (skin_rt_semaphore *)rt_get_adr(nam2num(name));
}

static inline skin_rt_mutex *skin_rt_get_shared_mutex(const char *name)
{
	return (skin_rt_mutex *)rt_get_adr(nam2num(name));
}

static inline skin_rt_rwlock *skin_rt_get_shared_rwlock(const char *name)
{
	return (skin_rt_rwlock *)rt_get_adr(nam2num(name));
}

static inline int skin_rt_name_available(const char *name)
{
	return !rt_get_adr(nam2num(name));
}

#endif // SKIN_DS_ONLY

#endif

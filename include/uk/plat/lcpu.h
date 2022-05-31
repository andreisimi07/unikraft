/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __UKPLAT_LCPU_H__
#define __UKPLAT_LCPU_H__

#include <uk/arch/time.h>
#include <uk/arch/lcpu.h>
#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enables interrupts
 */
void ukplat_lcpu_enable_irq(void);

/**
 * Disables interrupts
 */
void ukplat_lcpu_disable_irq(void);

/**
 * Returns current interrupt flags and disables them
 *
 * @return interrupt flags (the format is unspecified)
 */
unsigned long ukplat_lcpu_save_irqf(void);

/**
 * Loads interrupt flags
 *
 * @param flags interrupt flags (the format is unspecified)
 */
void ukplat_lcpu_restore_irqf(unsigned long flags);

/**
 * Checks if interrupts are disabled
 *
 * @return non-zero value if interrupts are disabled
 */
int ukplat_lcpu_irqs_disabled(void);

/**
 * TODO: This is Xen specific. Remove from public API?
 */
void ukplat_lcpu_irqs_handle_pending(void);

/**
 * Halts the current logical CPU execution
 */
void ukplat_lcpu_halt(void);

/**
 * Halts the current logical CPU. Execution is resumed when an interrupt/signal
 * arrives or the specified deadline expires
 *
 * @param until deadline in nanoseconds
 */
void ukplat_lcpu_halt_to(__nsec until);

/**
 * Halts the current logical CPU. Execution is resumed when an interrupt/signal
 * arrives.
 */
void ukplat_lcpu_halt_irq(void);

/* Non-prototyped logical CPU entry function */
typedef void __noreturn (*ukplat_lcpu_entry_t)();

typedef __u32 __lcpuidx;	/* Sequential index of logical CPU */
typedef __u64 __lcpuid;		/* Physical ID of logical CPU */

/**
 * Returns the hardware ID of the current logical CPU
 */
__lcpuid ukplat_lcpu_id(void);

#ifdef CONFIG_HAVE_SMP
#include <uk/list.h>

struct ukplat_lcpu_func {
	struct uk_list_head lentry;

	/**
	 * Function to execute.
	 *
	 * @param regs pointer to a snapshot of the current CPU state. Changes
	 *   to the state are applied after the RUN IRQ handler returns
	 * @param fn pointer to this structure. The structure is not touched
	 *   after function invocation and must be freed if necessary during
	 *   function execution
	 */
	void (*fn)(struct __regs *regs, struct ukplat_lcpu_func *fn);

	/* Optional user-supplied pointer. */
	void *user;
};

/**
 * Returns the index of the current logical CPU
 */
__lcpuidx ukplat_lcpu_idx(void);

/**
 * Returns the number of logical CPUs present in the system
 */
__u32 ukplat_lcpu_count(void);

/**
 * Starts multiple logical CPUs and assigns them the given stacks. The logical
 * CPUs execute the entry functions if supplied or enter a low-power wait state
 * otherwise. CPUs that have already been started are ignored.
 *
 * @param lcpuidx array with the indices of the logical CPUs that are to be
 *   started. Can be NULL to include all logical CPUs except the one executing
 *   the function
 * @param num number of logical CPUs specified, 0 if lcpuidx is NULL
 * @param sp array of stack pointers, one for each logical CPU to start. If
 *   lcpuidx is NULL, must be ukplat_lcpu_count() - 1 stacks
 * @param entry array of entry function pointers, one for logical CPU to start.
 *   Can be NULL, otherwise if lcpuidx is NULL, must contain
 *   ukplat_lcpu_count() - 1 function pointers. Provided functions must not
 *   return. If the parameter or individual function pointers are NULL the
 *   respective logical CPUs enter a low-power wait state after startup
 * @param flags (architecture-dependent) flags that specify how to start the
 *   CPUs (see UKPLAT_LCPU_SFLG_* flags if available)
 *
 * @return 0 on success, an errno-type error value otherwise
 */
int ukplat_lcpu_start(__lcpuidx lcpuidx[], unsigned int num, void *sp[],
		      ukplat_lcpu_entry_t entry[], unsigned long flags);

/**
 * Waits for the specified logical CPUs to enter idle state, or until the
 * timeout expires.
 *
 * @param lcpuidx array with the indices of the logical CPUs to wait for. Can
 *   be NULL to include all logical CPUs except the one executing the function
 * @param num number of logical CPUs specified, 0 if lcpuidx is NULL
 * @param timeout timeout in nanoseconds for the wait to be satisfied. Can be
 *   0 to wait indefinitely
 *
 * @return 0 if the wait for all specified logical CPUs has been satisfied,
 *   an errno-type error value otherwise (e.g., timeout)
 */
int ukplat_lcpu_wait(__lcpuidx lcpuidx[], unsigned int num, __nsec timeout);

/**
 * Executes a function on the specified logical CPUs. The run function does not
 * wait for the execution to start or complete. Multiple functions can be run
 * at the same time without having to wait for their completion.
 *
 * @param lcpuidx array with the indices of the logical CPUs that should
 *   execute the function. Can be NULL to execute the function on all logical
 *   CPUs except the current one
 * @param num number of logical CPUs specified, 0 if lcpuidx is NULL
 * @param fn the function to be executed
 * @param flags (architecture-dependent) flags that specify how the function
 *   should be executed (see UKPLAT_LCPU_RFLG_* flags if available)
 *
 * @return 0 on success, an errno-type error value otherwise
 */
int ukplat_lcpu_run(__lcpuidx lcpuidx[], unsigned int num,
		    struct ukplat_lcpu_func *fn, unsigned long flags);

/**
 * Wakes up the specified logical CPUs from a halt or low-power sleep state.
 *
 * @param lcpuidx array with the indices of the logical CPUs that should be
 *   woken up. Can be NULL to wakeup all logical CPUs except the current one.
 * @param num number of logical CPUs specified, 0 if lcpuidx is NULL
 *
 * @return 0 on success, an errno-type error value otherwise
 */
int ukplat_lcpu_wakeup(__lcpuidx lcpuidx[], unsigned int num);

#else /* CONFIG_HAVE_SMP */
#define ukplat_lcpu_idx()	(0)
#define ukplat_lcpu_count()	(1)
#endif /* CONFIG_HAVE_SMP */

#ifdef __cplusplus
}
#endif

#endif /* __UKPLAT_LCPU_H__ */

/* implement fork from user space */

#include "inc/env.h"
#include <inc/string.h>
#include <inc/lib.h>

/* User-level fork with copy-on-write.
 * Create a child.
 * Lazily copy our address space and page fault handler setup to the child.
 * Then mark the child as runnable and return.
 *
 * Returns: child's envid to the parent, 0 to the child, < 0 on error.
 * It is also OK to panic on error.
 *
 * Hint:
 *   Use sys_map_region, it can perform address space copying in one call
 *   Don't forget to set page fault handler in the child (using sys_env_set_pgfault_upcall()).
 *   Remember to fix "thisenv" in the child process.
 */
envid_t
fork(void) {
    // LAB 9: Your code here.

    envid_t child_id = sys_exofork();

    if (child_id == 0) {
        // child
        thisenv = &envs[ENVX(sys_getenvid())];
    } else {
        // parent
        // todo: check ret
        sys_map_region(0, NULL, child_id, NULL, MAX_USER_ADDRESS, PROT_ALL | PROT_LAZY | PROT_COMBINE);
        sys_env_set_pgfault_upcall(child_id, thisenv->env_pgfault_upcall);

        sys_env_set_status(child_id, ENV_RUNNABLE);
    }

    return child_id;
}

// FIXME: What is duppage(), pagefault() in lab's text?

envid_t
sfork() {
    panic("sfork() is not implemented");
}

/* User-level IPC library routines */

#include "inc/assert.h"
#include "inc/error.h"
#include "inc/memlayout.h"
#include <inc/lib.h>

#define store_nonnull(storage, value) \
    do {                              \
        if (storage)                  \
            *storage = value;         \
    } while (0)

/* Receive a value via IPC and return it.
 * If 'pg' is nonnull, then any page sent by the sender will be mapped at
 *    that address.
 * If 'from_env_store' is nonnull, then store the IPC sender's envid in
 *    *from_env_store.
 * If 'perm_store' is nonnull, then store the IPC sender's page permission
 *    in *perm_store (this is nonzero iff a page was successfully
 *    transferred to 'pg').
 * If the system call fails, then store 0 in *fromenv and *perm (if
 *    they're nonnull) and return the error.
 * Otherwise, return the value sent by the sender
 *
 * Hint:
 *   Use 'thisenv' to discover the value and who sent it.
 *   If 'pg' is null, pass sys_ipc_recv a value that it will understand
 *   as meaning "no page".  (Zero is not the right value, since that's
 *   a perfectly valid place to map a page.) */
int32_t
ipc_recv(envid_t *from_env_store, void *pg, size_t *size, int *perm_store) {
    // LAB 9: Your code here:

    size_t actual_size = 0;

    // [dkay]: tell that we are not expecting for a page
    if (pg == NULL) {
        pg = (void *)(MAX_USER_ADDRESS + 1);
    // user is a dumbass by passing a size=0
    // so punish them with undocumented behavior
    } else if (size != NULL) {
        actual_size = *size;
    }

    int ret = sys_ipc_recv(pg, actual_size);

    if (ret != 0) {
        store_nonnull(from_env_store, 0);
        store_nonnull(perm_store, 0);
        store_nonnull(size, 0);

        return ret;
    }

    store_nonnull(from_env_store, thisenv->env_ipc_from);
    store_nonnull(perm_store, thisenv->env_ipc_perm);
    store_nonnull(size, thisenv->env_ipc_maxsz);

    return thisenv->env_ipc_value;
}

/* Send 'val' (and 'pg' with 'perm', if 'pg' is nonnull) to 'toenv'.
 * This function keeps trying until it succeeds.
 * It should panic() on any error other than -E_IPC_NOT_RECV.
 *
 * Hint:
 *   Use sys_yield() to be CPU-friendly.
 *   If 'pg' is null, pass sys_ipc_recv a value that it will understand
 *   as meaning "no page".  (Zero is not the right value.) */
void
ipc_send(envid_t to_env, uint32_t val, void *pg, size_t size, int perm) {
    // LAB 9: Your code here:

    // user is a dumbass if they allow this
    // so we skip the check and silent return on failure
    // and allow  panic in such case
    // if (pg != NULL && size == 0)
    //     return;

    if (pg == NULL)
        pg = (void *)(MAX_USER_ADDRESS + 1);

    int ret = 0;

    do {
        // [dkay]: if previous send ended up with not recv,
        //         yielding instead of heating up cpu
        if (ret == -E_IPC_NOT_RECV)
            sys_yield();

        ret = sys_ipc_try_send(to_env, val, pg, size, perm);
    } while (ret == -E_IPC_NOT_RECV);

    if (ret != 0)
        panic("Error while calling to sys_ipc_try_send: %i", ret);
}

/* Find the first environment of the given type.  We'll use this to
 * find special environments.
 * Returns 0 if no such environment exists. */
envid_t
ipc_find_env(enum EnvType type) {
    for (size_t i = 0; i < NENV; i++)
        if (envs[i].env_type == type)
            return envs[i].env_id;
    return 0;
}

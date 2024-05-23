

#ifndef __MX_PROCESS_H_
#define __MX_PROCESS_H_


#include "mx/core/message.h"

#include "mx/pthread/pt.h"




#define PROCESS_NONE                    NULL
#define PROCESS_BROADCAST               NULL
#define PROCESS_ZOMBIE                  ((struct process *)0x1)



enum process_state_e
{
    PROCESS_STATE_NONE      = 0,
    PROCESS_STATE_RUNNING   = 1,
    PROCESS_STATE_CALLED    = 2,
};



enum process_status_e
{
    PROCESS_SUCCESS = 0,

    PROCESS_ERR_NOT_POSSIBLE = -1,
    PROCESS_ERR_NOT_SUPPORTED = -2,
    PROCESS_ERR_BAD_LENGTH = -3 ,
    PROCESS_ERR_NO_MEMORY = -4,
    PROCESS_ERR_QUEUE_FULL = -5,
    PROCESS_ERR_MSG_NOT_FOUND = -6,
};






/**
 * \name Process protothread functions
 * @{
 */

/**
 * Define the beginning of a process.
 *
 * This macro defines the beginning of a process, and must always
 * appear in a PROCESS_THREAD() definition. The PROCESS_END() macro
 * must come at the end of the process.
 *
 * \hideinitializer
 */
#define PROCESS_BEGIN()             PT_BEGIN(&self->pt)

/**
 * Define the end of a process.
 *
 * This macro defines the end of a process. It must appear in a
 * PROCESS_THREAD() definition and must always be included. The
 * process exits when the PROCESS_END() macro is reached.
 *
 * \hideinitializer
 */
#define PROCESS_END()               PT_END(&self->pt)

/**
 * Wait for an event to be posted to the process.
 *
 * This macro blocks the currently running process until the process
 * receives an event.
 *
 * \hideinitializer
 */
#define PROCESS_WAIT_EVENT()        PROCESS_YIELD()

/**
 * Wait for an event to be posted to the process, with an extra
 * condition.
 *
 * This macro is similar to PROCESS_WAIT_EVENT() in that it blocks the
 * currently running process until the process receives an event. But
 * PROCESS_WAIT_EVENT_UNTIL() takes an extra condition which must be
 * true for the process to continue.
 *
 * \param c The condition that must be true for the process to continue.
 * \sa PT_WAIT_UNTIL()
 *
 * \hideinitializer
 */
#define PROCESS_WAIT_EVENT_UNTIL(c) PROCESS_YIELD_UNTIL(c)

/**
 * Yield the currently running process.
 *
 * \hideinitializer
 */
#define PROCESS_YIELD()             PT_YIELD(&self->pt)

/**
 * Yield the currently running process until a condition occurs.
 *
 * This macro is different from PROCESS_WAIT_UNTIL() in that
 * PROCESS_YIELD_UNTIL() is guaranteed to always yield at least
 * once. This ensures that the process does not end up in an infinite
 * loop and monopolizing the CPU.
 *
 * \param c The condition to wait for.
 *
 * \hideinitializer
 */
#define PROCESS_YIELD_UNTIL(c)      PT_YIELD_UNTIL(&self->pt, c)

/**
 * Wait for a condition to occur.
 *
 * This macro does not guarantee that the process yields, and should
 * therefore be used with care. In most cases, PROCESS_WAIT_EVENT(),
 * PROCESS_WAIT_EVENT_UNTIL(), PROCESS_YIELD() or
 * PROCESS_YIELD_UNTIL() should be used instead.
 *
 * \param c The condition to wait for.
 *
 * \hideinitializer
 */
#define PROCESS_WAIT_UNTIL(c)       PT_WAIT_UNTIL(&self->pt, c)
#define PROCESS_WAIT_WHILE(c)       PT_WAIT_WHILE(&self->pt, c)

/**
 * Exit the currently running process.
 *
 * \hideinitializer
 */
#define PROCESS_EXIT()              PT_EXIT(&self->pt)

/**
 * Spawn a protothread from the process.
 *
 * \param pt The protothread state (struct pt) for the new protothread
 * \param thread The call to the protothread function.
 * \sa PT_SPAWN()
 *
 * \hideinitializer
 */
#define PROCESS_PT_SPAWN(pt, thread)   PT_SPAWN(&self->pt, pt, thread)

/**
 * Yield the process for a short while.
 *
 * This macro yields the currently running process for a short while,
 * thus letting other processes run before the process continues.
 *
 * \hideinitializer
 */
#define PROCESS_PAUSE()             do {                 \
    process_send_msg_p0(self, PROCESS_EV_CONTINUE);      \
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EV_CONTINUE); \
} while(0)

/** @} end of protothread functions */




/**
 * \name Process declaration and definition
 * @{
 */

/**
 * Define the body of a process.
 *
 * This macro is used to define the body (protothread) of a
 * process. The process is called whenever an event occurs in the
 * system, A process always start with the PROCESS_BEGIN() macro and
 * end with the PROCESS_END() macro.
 *
 * \hideinitializer
 */
#define PROCESS_THREAD(name, ev, msg)                   \
static PT_THREAD(process_thread_##name(struct process *self, msgtype_t ev, struct msg *msg))

/**
 * Declare the name of a process.
 *
 * This macro is typically used in header files to declare the name of
 * a process that is implemented in the C file.
 *
 * \hideinitializer
 */
#define PROCESS_NAME(name) extern struct process name

/**
 * Declare a process.
 *
 * This macro declares a process. The process has two names: the
 * variable of the process structure, which is used by the C program,
 * and a human readable string name, which is used when debugging.
 * A configuration option allows removal of the readable name to save RAM.
 *
 * \param name The variable name of the process structure.
 * \param strname The string representation of the process' name.
 *
 * \hideinitializer
 */
#if PROCESS_CONF_NO_PROCESS_NAMES
#define PROCESS(name, strname)                          \
  PROCESS_THREAD(name, ev, msg);                        \
  struct process name = { NULL, NULL,                   \
                          process_thread_##name,        \
                          NULL,                         \
                          {0}, 0, 0 }
#else
#define PROCESS(name, strname)                          \
  PROCESS_THREAD(name, ev, msg);                        \
  struct process name = { NULL, NULL, strname,          \
                          process_thread_##name,        \
                          {0}, 0, 0 }
#endif


/**
 * Get a pointer to the currently running process.
 *
 * This macro get a pointer to the currently running
 * process. Typically, this macro is used to post an event to the
 * current process with process_post().
 *
 * \hideinitializer
 */
#define PROCESS_CURRENT() process_get_current()

/** @} */




struct process {
    struct process *next;
    struct scheduler *sched;
#if PROCESS_CONF_NO_PROCESS_NAMES
#define PROCESS_NAME_STRING(process) ""
#else
    const char *name;
#define PROCESS_NAME_STRING(process) (process)->name
#endif
    PT_THREAD((* thread)(struct process *, msgtype_t, struct msg*));
    struct pt pt;
    unsigned char state, needspoll;
};




void process_init(void *buffer, uint16_t len);
void process_start(struct process *p);
void process_exit(struct process *p);
void process_poll(struct process *p);

unsigned int process_run(void);
unsigned int process_events(void);

int process_is_running(struct process *p);

struct process* process_get_current(void);







struct mx_msg_process
{
    msgtype_t type;
    struct process *process;
};



struct msg* process_malloc(uint32_t size);
struct msg* process_free(struct msg *msg);

// post message object allocated with process_alloc()
int process_post_msg(struct process *p, struct msg *msg);

// handle message object synchronously
void process_handle_msg(struct process *p, struct msg *msg);
void process_handle_msg_p0(struct process *p, msgtype_t type);

// send message object is created/copied internally
int process_send_msg(struct process *p, struct msg *msg, uint32_t msg_len);
int process_send_msg_p0(struct process *p, msgtype_t type);
int process_send_msg_p1(struct process *p, msgtype_t type, uint8_t param1);
int process_send_msg_p2(struct process *p, msgtype_t type, uint8_t param1, uint8_t param2);
int process_send_msg_p3(struct process *p, msgtype_t type, uint8_t param1, uint8_t param2, uint8_t param3);
int process_send_msg_p4(struct process *p, msgtype_t type, uint8_t param1, uint8_t param2, uint8_t param3, uint8_t param4);
int process_send_msg_data(struct process *p, msgtype_t type, uint8_t *data, uint32_t data_len);



#endif /* __MX_PROCESS_H_ */

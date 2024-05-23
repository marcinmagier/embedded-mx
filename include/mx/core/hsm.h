
#ifndef __MX_HSM_H_
#define __MX_HSM_H_


//enum hsm_ev
//{
//    HSM_EV_EXIT_STATE = 0,
//    HSM_EV_ENTER_STATE,
//
//    HSM_EV_TIMER_1MS,
//    HSM_EV_TIMER_10MS,
//    HSM_EV_TIMER_100MS,
//    HSM_EV_TIMER_1S,
//    HSM_EV_TIMER_10S,
//    HSM_EV_TIMER_1M,
//
//    HSM_EV_LAST = 0x10,
//};


#define HSM_UNWIND_SKIP        0
#define HSM_UNWIND_CURRENT     1
#define HSM_UNWIND_PARRENT     2




#define HSM_RET_HANDLED        0
#define HSM_RET_IGNORED        1
#define HSM_RET_SUPER          2
#define HSM_RET_TRANS          3


#define HSM_HANDLED()                               (HSM_RET_HANDLED)
#define HSM_IGNORED()                               (HSM_RET_IGNORED)
#define HSM_SUPER(hsm, new_state)                   (hsm->state = new_state, HSM_RET_SUPER)
#define HSM_TRANS(hsm, new_state, unwind_depth)     (hsm->state = new_state, hsm->unwind = (unwind_depth), HSM_RET_TRANS)
#define HSM_PASS(hsm, new_state)                    HSM_TRANS(hsm, new_state, HSM_UNWIND_SKIP)



typedef int (*hsm_state_handler_f)(void *object, int type, const void *msg);

struct hsm_state
{
#ifdef DEBUG_HSM
    const char *name;
#endif
    hsm_state_handler_f handler;
};

#ifdef DEBUG_HSM
  #define HSM_STATE_DEF(state, handler) [state] = {#state, (hsm_state_handler_f)handler}
#else
  #define HSM_STATE_DEF(state, handler) [state] = {(hsm_state_handler_f)handler}
#endif



struct hsm_timer
{
    unsigned long counter;
    unsigned short mask;
};



struct hsm
{
    int state;
    int current_state;
    int unwind;
    const struct hsm_state *router;
    struct hsm_timer timer;
};



void hsm_init(struct hsm *hsm, int initial_state, const struct hsm_state *state_router);
void hsm_clean(struct hsm *hsm);

int  hsm_get_current_state(struct hsm *hsm);

void hsm_handle_event(struct hsm *hsm, void *container, int ev, const void *data);
void hsm_handle_time(struct hsm *hsm, void *container, unsigned int time_lapse);



#define HSM_TIMER_DISABLED    (0)
#define HSM_TIMER_1MS         (0x1 << 0)
#define HSM_TIMER_10MS        (0x1 << 1)
#define HSM_TIMER_100MS       (0x1 << 2)
#define HSM_TIMER_1S          (0x1 << 3)
#define HSM_TIMER_10S         (0x1 << 4)
#define HSM_TIMER_1M          (0x1 << 5)

void hsm_timer_reset(struct hsm *hsm);
void hsm_timer_enable(struct hsm *hsm, unsigned short mask);
void hsm_timer_disable(struct hsm *hsm, unsigned short mask);
unsigned long hsm_timer_milis(struct hsm *hsm);
unsigned long hsm_timer_seconds(struct hsm *hsm);



#endif /* __MX_HSM_H_ */

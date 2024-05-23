
#ifndef __MX_MESSAGE_H_
#define __MX_MESSAGE_H_


#include <stdint.h>


#ifndef MSGTYPE_SIZE
  #define MSGTYPE_SIZE          1
#endif


#if (MSGTYPE_SIZE != 1) && (MSGTYPE_SIZE != 2)
    #error Unsupported MSGTYPE_SIZE value
#endif


#if MSGTYPE_SIZE == 1
  #define MSG_REPORT            0
  #define MSG_REQUEST           0x40
  #define MSG_RESPONSE          0x80
  #define MSG_TYPE_MASK         0xC0
  #define MSG_ID_MASK           (~MSG_TYPE_MASK)
  typedef uint8_t msgtype_t;
#elif MSGTYPE_SIZE == 2
  #define MSG_REPORT            0
  #define MSG_REQUEST           0x4000
  #define MSG_RESPONSE          0x8000
  #define MSG_TYPE_MASK         0xC000
  #define MSG_ID_MASK           (~MSG_TYPE_MASK)
  typedef uint16_t msgtype_t;
#endif




struct msg
{
    msgtype_t type;
}
__attribute__((packed));



#define MSG_BASE() union { msgtype_t type; struct msg base; }





struct msg_p1
{
    union {
        msgtype_t type;
        struct msg base;
    };

    uint8_t param1;
}
__attribute__((packed));



struct msg_p2
{
    union {
        msgtype_t type;
        struct msg base;
    };

    uint8_t param1;
    uint8_t param2;
}
__attribute__((packed));



struct msg_p3
{
    union {
        msgtype_t type;
        struct msg base;
    };

    uint8_t param1;
    uint8_t param2;
    uint8_t param3;
}
__attribute__((packed));



struct msg_p4
{
    union {
        msgtype_t type;
        struct msg base;
    };

    uint8_t param1;
    uint8_t param2;
    uint8_t param3;
    uint8_t param4;
}
__attribute__((packed));



struct msg_data
{
    union {
        msgtype_t type;
        struct msg base;
    };

    uint8_t data[];
}
__attribute__((packed));



struct msg_pointer
{
    union {
        msgtype_t type;
        struct msg base;
    };

    void *pointer;
}
__attribute__((packed));




enum msgtype_e
{
    // process events, 0x00
    PROCESS_EV_INIT = 0x00,
    PROCESS_EV_POLL = 0x01,
    PROCESS_EV_EXIT = 0x02,
    PROCESS_EV_FINISHED = 0x03,
    PROCESS_EV_DUMMY = 0x04,
    PROCESS_EV_TIMER = 0x05,
    PROCESS_EV_PAUSE = 0x06,
    PROCESS_EV_CONTINUE = 0x07,

    // hsm events, 0x08
    HSM_EV_EXIT_STATE = 0x08,
    HSM_EV_ENTER_STATE = 0x09,
    HSM_EV_TIMER_1MS = 0x0A,
    HSM_EV_TIMER_10MS = 0x0B,
    HSM_EV_TIMER_100MS = 0x0C,
    HSM_EV_TIMER_1S = 0x0D,
    HSM_EV_TIMER_10S = 0x0E,
    HSM_EV_TIMER_1M = 0x0F,


    // MISC_REQ
    // MISC_RESP
    // MISC_REPORT
    // MISC_TIMEOUT

    // _CFM, _REJ, _IND, _DATA, _PACKET, _MSG, _INFO, _ERR, _TICK
};



#endif /* __MX_MESSAGE_H_ */

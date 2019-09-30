#ifndef _POETS_MSG_H_
#define _POETS_MSG_H_

#include <cstddef>
#include <cstdint>

// poets_hardware.h is a platform-specific configuration file
// generated by the Orchestrator, and which must include at least
// P_MSG_MAX_SIZE, LOG_BOARDS_PER_BOX, LOG_CORES_PER_BOARD,
// and LOG_THREADS_PER_CORE
#include "poets_hardware.h"
#include "SoftwareAddress.h"

#define DEST_BROADCAST 0xFFFFFFFF

#define MAX_P_SUP_MSG_BYTES 256
#define LOG_DEVICES_PER_THREAD 10
#define P_DEVICE_OS 0
#define P_THREAD_OS (LOG_DEVICES_PER_THREAD)
#define P_CORE_OS (LOG_THREADS_PER_CORE+P_THREAD_OS)
#define P_BOARD_OS (LOG_CORES_PER_BOARD+P_CORE_OS)
#define P_BOX_OS (LOG_BOARDS_PER_BOX+P_BOARD_OS)
#define P_SUP_OS 31
#define P_SUP_MASK (0x1 << P_SUP_OS)

//------------------------------------------------------------------------------
// TODO: rationalise these defines as they are superfluous?
//------------------------------------------------------------------------------
#define P_BOX_MASK ((0x1 << P_SUP_OS) - (0x1 << P_BOX_OS))
#define P_BOARD_MASK ((0x1 << P_BOX_OS) - (0x1 << P_BOARD_OS)) 
#define P_CORE_MASK ((0x1 << P_BOARD_OS) - (0x1 << P_CORE_OS))
#define P_THREAD_MASK ((0x1 << P_CORE_OS) - (0x1 << P_THREAD_OS))
#define P_DEVICE_MASK ((0x1 << P_THREAD_OS) - (0x1 << P_DEVICE_OS))
#define P_THREAD_HWOS 0
#define P_CORE_HWOS (LOG_THREADS_PER_CORE+P_THREAD_HWOS)
#define P_MAILBOX_HWOS (TinselLogCoresPerMailbox+P_CORE_HWOS)
#define P_BOARD_HWOS (LOG_CORES_PER_BOARD+P_CORE_HWOS)
#define P_BOX_HWOS (LOG_BOARDS_PER_BOX+P_BOARD_HWOS)
#define P_BOX_HWMASK ((0x1 << P_SUP_OS) - (0x1 << P_BOX_HWOS))
#define P_BOARD_HWMASK ((0x1 << P_BOX_HWOS) - (0x1 << P_BOARD_HWOS)) 
#define P_CORE_HWMASK ((0x1 << P_BOARD_HWOS) - (0x1 << P_CORE_HWOS))
#define P_THREAD_HWMASK ((0x1 << P_CORE_HWOS) - (0x1 << P_THREAD_HWOS))
//------------------------------------------------------------------------------

#define P_PKT_MSGTYP_OS 10
#define P_PKT_MSGTYP_BARRIER 0x1000
#define P_PKT_MSGTYP_SUPER 0x2000
#define P_PKT_MSGTYP_ALIVE 0x4000
#define P_MSG_TAG_INIT 0x0 // could also be 0xFFFF
#define P_MSG_TAG_STOP 0x8000 
#define P_SUP_PIN_SYS 0xFFFF    //An actual supervisor control message?
#define P_SUP_PIN_SYS_SHORT 0xFF //A "fake?" Supervisor control message?!?!
#define P_SUP_PIN_INIT 0 // very temporary bodge for __init__ pins
#define P_SUP_MSG_BARR 0x8000
#define P_SUP_MSG_KILL 0x8001
#define P_SUP_MSG_LOG  0x8002


//------------------------------------------------------------------------------
// Header bitmasks and bitshifts
//------------------------------------------------------------------------------
#define P_HW_HWADDR_MASK            0xFFFFFFFF
#define P_HW_HWADDR_SHIFT           0
    
#define P_SW_MOTHERSHIP_MASK        ISMOTHERSHIP_BIT_MASK       
#define P_SW_MOTHERSHIP_SHIFT       ISMOTHERSHIP_SHIFT
#define P_SW_CNC_MASK               ISCNC_BIT_MASK
#define P_SW_CNC_SHIFT              ISCNC_SHIFT
#define P_SW_TASK_MASK              TASK_BIT_MASK
#define P_SW_TASK_SHIFT             TASK_SHIFT
#define P_SW_OPCODE_MASK            OPCODE_BIT_MASK
#define P_SW_OPCODE_SHIFT           OPCODE_SHIFT
#define P_SW_DEVICE_MASK            DEVICE_BIT_MASK
#define P_SW_DEVICE_SHIFT           DEVICE_SHIFT


#define P_HD_TGTPIN_MASK            0xFF
#define P_HD_TGTPIN_SHIFT           0
#define P_HD_DESTEDGEINDEX_MASK     0xFFFFFF00
#define P_HD_DESTEDGEINDEX_SHIFT    8
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Magic Addresses
//------------------------------------------------------------------------------
#define P_ADDR_BROADCAST            0xFFFF


#define P_CNC_LOG                   0xFB
#define P_CNC_BARRIER               0xFC
#define P_CNC_INIT                  0xFD
#define P_CNC_STOP                  0xFE
#define P_CNC_KILL                  0xFF

//------------------------------------------------------------------------------

#pragma pack(push,1)
typedef struct poets_message_header
{
    // Begin flit 0
    // Header
    //uint32_t hwAddr;
    uint32_t swAddr;
    uint32_t pinAddr;
    // End Header.
    // 64-bits ~~~32-bits~~~ left in the first flit.
    
    /*
    uint32_t destDeviceAddr;   // Opaque destination device address
    uint32_t destEdgeIndex;    // Input edge index at the destination
    uint8_t  destPin;          // Input pin index at the destination
    uint8_t  messageLenBytes;  // Total message length including header
    uint16_t messageTag;       // Type of message
    */
} P_Msg_Hdr_t;

typedef struct poets_message
{
    P_Msg_Hdr_t header; // a message always has a header      
    uint8_t payload[P_MSG_MAX_SIZE-sizeof(P_Msg_Hdr_t)]; // and it might have some more data
    // In the default tinsel configuration the first flit has 4 bytes of payload
    // Further flits contain data.
} P_Msg_t;


typedef struct poets_supervisor_message
{
    uint32_t hwAddr;    // The destination HW address
    P_Msg_t msg;        // The POETs fabric message
    uint32_t len;       // Number of bytes for the P_Msg_t
} P_Super_Msg_t;


const unsigned int p_msg_pyld_size = sizeof(P_Msg_t)-sizeof(P_Msg_Hdr_t);
const unsigned int p_log_msg_pyld_size = p_msg_pyld_size-sizeof(uint8_t);
const unsigned int p_log_msg_hdr_size = sizeof(P_Msg_Hdr_t)+sizeof(uint8_t);


inline size_t p_super_msg_size() {return sizeof(P_Super_Msg_t);}
inline size_t p_msg_size() {return sizeof(P_Msg_t);}
inline size_t p_hdr_size() {return sizeof(P_Msg_Hdr_t);}

// message buffers (last argument) in the message setters must be volatile because we might wish
// to write directly to a hardware resource containing the buffer, which in general may be volatile.
unsigned set_msg_hdr(uint8_t, uint8_t, uint8_t, uint8_t, uint32_t, uint8_t, 
                     uint32_t, uint8_t, P_Msg_Hdr_t*);

unsigned pack_msg(uint8_t, uint8_t, uint8_t, uint8_t, uint32_t, uint8_t, 
                   uint32_t, uint8_t, void*, P_Msg_Hdr_t*);


#pragma pack(pop)
#endif

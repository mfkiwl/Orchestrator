#ifndef _POETS_MSG_H_
#define _POETS_MSG_H_

#include <cstddef>
#include <cstdint>

// poets_hardware.h is a platform-specific configuration file
// generated by the Orchestrator, and which must include at least
// P_MSG_MAX_SIZE, LOG_BOARDS_PER_BOX, LOG_CORES_PER_BOARD,
// and LOG_THREADS_PER_CORE
#include "poets_hardware.h"

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
#define P_BOX_MASK ((0x1 << P_SUP_OS) - (0x1 << P_BOX_OS))
#define P_BOARD_MASK ((0x1 << P_BOX_OS) - (0x1 << P_BOARD_OS)) 
#define P_CORE_MASK ((0x1 << P_BOARD_OS) - (0x1 << P_CORE_OS))
#define P_THREAD_MASK ((0x1 << P_CORE_OS) - (0x1 << P_THREAD_OS))
#define P_DEVICE_MASK ((0x1 << P_THREAD_OS) - (0x1 << P_DEVICE_OS))
#define P_THREAD_HWOS 0
#define P_CORE_HWOS (LOG_THREADS_PER_CORE+P_THREAD_HWOS)
#define P_BOARD_HWOS (LOG_CORES_PER_BOARD+P_CORE_HWOS)
#define P_BOX_HWOS (LOG_BOARDS_PER_BOX+P_BOARD_HWOS)
#define P_BOX_HWMASK ((0x1 << P_SUP_OS) - (0x1 << P_BOX_HWOS))
#define P_BOARD_HWMASK ((0x1 << P_BOX_HWOS) - (0x1 << P_BOARD_HWOS)) 
#define P_CORE_HWMASK ((0x1 << P_BOARD_HWOS) - (0x1 << P_CORE_HWOS))
#define P_THREAD_HWMASK ((0x1 << P_CORE_HWOS) - (0x1 << P_THREAD_HWOS))
#define P_PKT_MSGTYP_OS 10
#define P_PKT_MSGTYP_BARRIER 0x1000
#define P_PKT_MSGTYP_SUPER 0x2000
#define P_PKT_MSGTYP_ALIVE 0x4000
#define P_MSG_TAG_INIT 0x0 // could also be 0xFFFF
#define P_MSG_TAG_STOP 0x8000 
#define P_SUP_PIN_SYS 0xFFFF	//An actual supervisor control message?
#define P_SUP_PIN_SYS_SHORT 0xFF //A "fake?" Supervisor control message?!?!
#define P_SUP_PIN_INIT 0 // very temporary bodge for __init__ pins
#define P_SUP_MSG_BARR 0x8000
#define P_SUP_MSG_KILL 0x8001
#define P_SUP_MSG_LOG  0x8002

#pragma pack(push,1)
typedef struct poets_message_header
{
        // Begin flit 0
        // Header
        uint32_t destDeviceAddr;   // Opaque destination device address
        uint32_t destEdgeIndex;    // Input edge index at the destination
        uint8_t  destPin;          // Input pin index at the destination
        uint8_t  messageLenBytes;  // Total message length including header
        uint16_t messageTag;       // Type of message
} P_Msg_Hdr_t;
  
typedef struct poets_message
{
        P_Msg_Hdr_t header; // a message always has a header      
        uint8_t payload[P_MSG_MAX_SIZE-sizeof(P_Msg_Hdr_t)]; // and it might have some more data
        // In the default tinsel configuration the first flit has 4 bytes of payload
        // Further flits contain data.
} P_Msg_t;

typedef struct p_super_msg_header
{
        // flit 0
        uint32_t sourceDeviceAddr; // issuing device in the Application
        uint16_t command;          // commands might be split into (fixed) system commands and user-defined commands
        uint16_t destPin;          // destination pin on the Supervisor
        uint32_t cmdLenBytes;      // length of the entire command (including all headers)
        uint32_t seq;              // sequence number of this sub-message

        // further flits contain data - which could be arguments, packets, data dumps, or just about anything else. 
} P_Sup_Hdr_t;

typedef struct p_super_message
{
        P_Sup_Hdr_t header; // a Supervisor message always has an appropriate header
        uint8_t data[P_MSG_MAX_SIZE-sizeof(P_Sup_Hdr_t)]; // and it might have some sort of data
} P_Sup_Msg_t;

const unsigned int p_msg_pyld_size = sizeof(P_Msg_t)-sizeof(P_Msg_Hdr_t);
const unsigned int p_super_data_size = sizeof(P_Sup_Msg_t)-sizeof(P_Sup_Hdr_t);

inline size_t p_msg_size() {return sizeof(P_Msg_t);};
inline size_t p_hdr_size() {return sizeof(P_Msg_Hdr_t);};
inline size_t p_sup_msg_size() {return sizeof(P_Sup_Msg_t);};
inline size_t p_sup_hdr_size() {return sizeof(P_Sup_Hdr_t);};
// message buffers (last argument) in the message setters must be volatile because we might wish
// to write directly to a hardware resource containing the buffer, which in general may be volatile.
void set_msg_hdr(uint32_t, uint32_t, uint8_t, uint8_t, uint16_t = 0, P_Msg_Hdr_t* = 0);
void pack_msg(uint32_t, uint32_t, uint32_t, uint32_t, uint8_t, uint16_t = 0, void* = 0, P_Msg_t* = 0);
void set_super_hdr(uint32_t, uint16_t, uint16_t, uint32_t, uint32_t = 0, P_Sup_Hdr_t* = 0);
uint32_t pack_super_msg(uint32_t, uint16_t, uint16_t, uint32_t, void*, P_Sup_Msg_t*);
void super_buf_clr(P_Sup_Msg_t*);
bool super_buf_recvd(const P_Sup_Msg_t*);

#pragma pack(pop)
#endif

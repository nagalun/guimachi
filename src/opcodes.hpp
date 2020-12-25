#pragma once

#include "types.hpp"

const static u8 PROTOCOL_VERSION = 0x19;

namespace NETW {
namespace ACTION {
enum : u8 {
	CONNECT    = 0x01,
	DISCONNECT = 0x02,
	LEAVE      = 0x03,
	DELETE     = 0x04
};
}

namespace BITS {
enum : u8 {
	CAN_LEAVE  = 0x02
};
}
}

namespace ENDPOINT {
namespace STATE {
enum : u8 {
	OK       = 0x00,
	OBSOLETE = 0x01,
	ERRORS   = 0x02
};
}

namespace TYPE {
enum : u8 {
	DIRECT = 0x01,
	RELAY  = 0x02,
	SERVER = 0x03
};
}
}

namespace TUN_BITS {
enum : u32 {
	IP_PROTO_MISMATCH = 0x02000000,
	IP_CONFLICT       = 0x00080000,
	DEVICE_DOWN       = 0x00020000,
	DOMAIN_ERROR      = 0x00010000, // or device error?
	GHOST_TUNNEL      = 0x00008000,
	BLOCKED           = 0x00004000,
	ENCRYPTION_ERROR  = 0x00001000,
	UNTRUSTED_ERROR   = 0x00000008,
	AUTH_ERROR        = 0x00000001
};
}

namespace PEER {
namespace BITS {
enum : u32 {
	CHAT_ENABLED  = 0x00001000,
	IPV4_DISABLED = 0x00000080,
	IPV6_ENABLED  = 0x00000020
};
}

namespace AUTH {
enum : u8 {
	NONE = 0x01,
	OK   = 0x04
};
}
}

namespace ENGINE {
namespace STATE {
enum : u8 {
	UNAVAILABLE = 1, // network unavailable, ask if auto reconnect is desired
	DISCONNECTING = 2,
	RESOLVING = 4, // ??
	RECONNECTING = 6,
	RESOLVING2 = 8,
	CONNECTING = 9,
	CONNECTING2 = 10,
	ENROLLING = 11, // ??
	AUTHENTIFYING = 12,
	PROBING = 13,
	PROBING2 = 14,
	SYNCHRONIZING = 16,
	READY = 17
};
}	
}

// numbers are big endian
// string is preceded with varint length
namespace SV_OPCODES {
enum : u8 {
	ENGINE_HELLO   = 0x01, /* u8{PROTO_VER} = 0x19, u32{VER_MAJOR} = 2, u32{VER_MINOR} = 2, u32{VER_REV} = 0, u32{VER_BUILD} = 607, u32{CONNECTION_CLIENT_ID} */
	ENGINE_INFO    = 0x02, /* vString{SELF_PEER_ID}, u8{PROTO_VER} = 0x19, u32{VER_MAJOR} = 2, u32{VER_MINOR} = 2, u32{VER_REV} = 0, u32{VER_BUILD} = 607, vString{LANG}, vString{ENGINE_DATA_PATH}, u8{?} = 0, [u8{client_key}, ...], u32{? (repeated in many keys after ending)} = 0x03010001,  */
	IM_SENT        = 0x08, /* u8{?} = 1, u8{?} = 0, vString{SENT_TO_ID} */
	IM_LOCAL_QUEUE = 0x0A,
	IM_CHATLOG     = 0x0B, /* u8{STATUS} = 1, u8{?} = 0, vString{PEER_ID}, u32{ROW}, u8{ARR_LEN}, [char '<'(out) or '>'(in), u8{?} = 0, u32{TIMESTAMP}, vString{MSG}, ...] */
	UNK_U16        = 0x0C, /* u8{?} = 1, u8{?} = 0 */
	PEER_CONFIG    = 0x0D, /* u8{?} = 1, */
	PEER_KEY       = 0x0F, /* u8{?} = 1, vString{PEER_ID}, u16{?} = 128, u8{?} = 2, key (256 bytes), u32{TIMESTAMP}, u8{TRUST_STATUS}, u32{TIMESTAMP} */
	KNOWN_PEERS    = 0x12, /* u16{?} = 1, u32{ARR_LEN}, [vString{PEER_ID}, ...] PROBABLY WRONG (sure it doesn't use varints for array size?)*/
	NICK_CHANGED   = 0x13,
	// 0x24: u16{0}
	NLIST_END      = 0x26,
	NLIST_SET_NET  = 0x27,
	NLIST_ADD_PEER = 0x28,
	SET_STATE      = 0x81, /* u32, u8, u8{?}, u8{?}, */
	NETW_BATCH_UPD = 0x82, /* bool, possible extra info? */
	NETW_UPDATED   = 0x83, /* vString{NETWORK_ID}, vString{NETWORK_NAME}, vString{OWNER_PEER_ID}, u32{MAX_MEMBERS}, u32{MEMBERS}, vString{ON_LOGIN_MSG} */
	NETW_DELETED   = 0x84,
	PEER_UPDATED   = 0x85, /* vString{PEER_ID}, vString{PEER_NAME} */
	PEER_DELETED   = 0x86, /* vString{PEER_ID} */
	PEER_JOIN_NETW = 0x87, /* vString{NETWORK_ID}, vString{PEER_ID} */
	PEER_QUIT_NETW = 0x88, /* vString{NETWORK_ID}, vString{PEER_ID} */
	IM_RECEIVED    = 0x89, /* vString{SENDER_ID}, bool{IS_GROUP_MESSAGE}, vString{NETW_ID}, u32{TIMESTAMP}, vString{MSG} */
	IM_TYPING      = 0x8A, /* vString{PEER_ID} */
	TUN_ACTIVITY   = 0x8C, /* [vString{PEER_ID}, ...] */
	TUN_DOMAIN     = 0x90, /* vString{DOM_NAME}, u8{?} = 5, u8, u8, u8, u32{TUN_IPv4}, u32{TUN_IPv4_MASK} */
	ACTION_DENIED  = 0x94 /* u8{REASON?} = 5 (chat related?) */
};
}

namespace CL_OPCODES {
enum : u8 { // after : is data sent on windows (if different)
	ENGINE_HELLO    = 0x01, /* u8{PROTO_VER} = PROTOCOL_VERSION */
	ENGINE_GET_INFO = 0x02, /* vString{LANG} = "en", u32 = 1 : "es", 0 */
	ENGINE_LOGIN    = 0x03, /* bool{RETRY} = 0x00 */
	ENGINE_LOGOFF   = 0x04,
	NETW_CREATE     = 0x05, /* vString{NAME}, vString{PASSWORD} */
	NETW_JOIN       = 0x06, /* vString{NAME}, vString{PASSWORD}, u8{?} = 0 */
	NETW_MANAGE     = 0x07, /* vString{NETWORK_ID}, u16{NETW_ACTION} */
	IM_SEND         = 0x08, /* bool{IS_GROUP_MESSAGE}, vString{TO_ID}, vString{MESSAGE} */
	IM_TYPING       = 0x09,
	IM_GET_CHATLOG  = 0x0B, /* u8{?} = 0, vString{PEER_ID}, u32{FROM_ROW}, u32{TO_READ_MAX} = 50, u32{FILE_OFFSET} */
	GET_UNK_U16     = 0x0C, /* might be part of the handshake */
	GET_PEER_CONFIG = 0x0D, /* vString{PEER_ID} */
	GET_PEER_KEY    = 0x0F, /* vString{PEER_ID} */
	GET_KNOWN_PEERS = 0x12, /* u64{?} = 0x40 */
	SET_NICK        = 0x13, /* vString{NEW_NICKNAME} */
	FIND_UPDATE     = 0x1C,
	GET_STATE       = 0x21, /* u8{?} = 0x93 */ // i believe this does something else as it doesn't work when disconnected
	GET_PEER_DETAIL = 0x24, /* vString{PEER_ID}, bool{GET_UPDATES} */
	NETW_GET_LIST   = 0x26,
	NETW_GET_INFO   = 0x2A, /* vString{NETWORK_ID} */
	NETW_TRANSFER   = 0x2C  /* vString{NETWORK_ID} */
};
}

/* The version notice (protocol.h, written in protocol.cpp): a server
   answers a packet from another build of this game (same "GDM" id prefix,
   different last letter) with these 8 bytes, its own protocol id then
   NET_VERSION_NOTICE_MAGIC. The format never changes, so a client of any
   version can tell "the server runs a different version" from "the server
   is not answering". It is smaller than any packet that triggers it, so it
   multiplies nobody's traffic. */

#define NET_VERSION_NOTICE_SIZE 8
#define NET_VERSION_NOTICE_MAGIC 0x3f524556u // "VER?"
#define NET_PROTOCOL_FAMILY_MASK 0xffffff00u

// True when Buffer starts with another version's protocol id.
internal bool32 NetIsOtherVersion(u8 *Buffer, u32 Size);
// Writes the notice into Buffer (NET_VERSION_NOTICE_SIZE bytes).
internal void NetWriteVersionNotice(u8 *Buffer);
// True when Buffer is a version notice; *ServerProtocol gets the sender's id.
internal bool32 NetReadVersionNotice(u8 *Buffer, u32 Size, u32 *ServerProtocol);

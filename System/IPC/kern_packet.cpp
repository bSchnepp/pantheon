#include <kern_runtime.hpp>
#include <kern_datatypes.hpp>

#include <System/IPC/kern_port.hpp>
#include <System/IPC/kern_packet.hpp>
#include <System/IPC/kern_connection.hpp>
#include <System/IPC/kern_client_port.hpp>

void pantheon::ipc::Packet::Initialize(Port *Server, ClientPort *Client)
{
	this->CurAmount = 0;
	ClearBuffer((CHAR*)this->Buffer, this->BufferSz);
	this->ServerSide = Server;
	this->ClientSide = Client;
}

UINT32 pantheon::ipc::Packet::BeginWrite(UINT32 *CopyContent, UINT32 Amount)
{
	/* Enforce max of 256 bytes to copy */
	UINT32 ContentSz = (Amount < (this->BufferSz - this->CurAmount)) ? Amount : this->BufferSz;
	CopyMemory(this->Buffer, CopyContent, ContentSz);
	this->CurAmount += ContentSz;
	return ContentSz;
}

void pantheon::ipc::Packet::Consume()
{
	this->CurAmount = 0;
	this->CurrentState = pantheon::ipc::Packet::State::WAITING_FOR_WRITE;
}

void pantheon::ipc::Packet::FinishWrite()
{
	this->CurrentState = pantheon::ipc::Packet::State::WAITING_FOR_READ;
}
#include <kern_thread.hpp>
#include <kern_datatypes.hpp>

#ifndef _KERN_PACKET_HPP_
#define _KERN_PACKET_HPP_

namespace pantheon::ipc
{

class Port;
class ClientPort;

class Packet
{
public:
	enum class State : UINT8
	{
		WAITING_FOR_WRITE,
		WAITING_FOR_READ,
	};

	static constexpr UINT8 BufferSz32 = 64;
	static constexpr UINT32 BufferSz = BufferSz32 * sizeof(UINT32);

	Packet() = default;
	~Packet() = default;

	void Initialize(Port *Server, ClientPort *Client);

	UINT32 BeginWrite(UINT32 *CopyContent, UINT32 Amount);
	void FinishWrite();

	void Consume();

	[[nodiscard]] UINT32 ContentSize() const { return this->CurAmount; }
	[[nodiscard]] const UINT32 *GetContent() const { return this->Buffer; }

private:
	State CurrentState;
	Port *ServerSide;
	ClientPort *ClientSide;

	UINT32 CurAmount;
	UINT32 Buffer[BufferSz32];
};

}

#endif
#include <SDK/pantheon.h>

void prgm_Main()
{
	svc_LogText("program manager started");

	INT32 Read;
	INT32 Write;

	svc_CreateNamedEvent("signal", &Read, &Write);
	for (;;)
	{
		volatile BOOL Recieve = svc_PollEvent(Read);
		if (Recieve)
		{
			svc_ResetEvent(Read);
			svc_LogText("GOT SIGNAL");
			svc_Yield();
		}

		INT32 ClientConn;
		pantheon::Result Status = svc_ConnectToNamedPort("sysm:reg", &ClientConn);
		if (Status != pantheon::Result::SYS_OK)
		{
			svc_LogText("Cannot connect to sysm:reg!");
		}
		else 
		{
			ThreadLocalRegion *Location = GetTLS();
			for (UINT32 &Index : Location->Payload.Data)
			{
				Index = 0;
			}

			/* Lorem ipsum tumbled a few times, this text is nonsense. */
			static const char *RandomString = 
				"Troublemaker"
				"After the Gods of the West"
				"We don't need Hendlerite Tellus Benenatis."
				"Show me the hall, Pelentes."
				"For fear of justice"
				"Fuegia Varius Tota. Sometimes he laughs with hate."
				"I'll take care of you at the Hendlerite course gate."
				"The author of Pain also expected it."
				"Who is the author of Mauris Vestibulum? Even the most cunning installations are possible."
				"They are there because the elephants want freedom, but they are among the lions."
				"Who fears the doors? My face is like a face, but a joke."
				"The night who influences the moon fears the doors."
				"The sea is ugly. Singing is more beautiful than pain";

			UINT64 StrLen = 0;
			for (UINT64 Index = 0; Index < 1022; Index++)
			{
				char Current = RandomString[Index];
				if (Current == '\0')
				{
					break;
				}
				Location->Payload.Data[Index] = RandomString[Index];
				StrLen++;
			}

			/* [10 bits Size] [2 Bits ReqType] [4 Bits ReqTypeData] */
			Location->Header.Meta = (StrLen << 6) | (0 << 4) | (0 << 0);
			Location->Header.Cmd = 0;

			svc_SendRequest(ClientConn);


			svc_SignalEvent(Write);
			svc_CloseHandle(ClientConn);
		}

		svc_LogText("IN USERSPACE [prgm]");		
	}
}

extern "C"
{

uintptr_t __stack_chk_guard = 0xDEADBEEFDEADC0DE;

[[noreturn]] VOID __stack_chk_fail(void)
{
	svc_LogText("CRASH: prgm has stack canary smashed.\n");
	svc_ExitProcess();
	for(;;){}
}

}

extern "C" void main()
{
	svc_LogText("prgm began in main");
	prgm_Main();
	svc_LogText("prgm returned from main");
}
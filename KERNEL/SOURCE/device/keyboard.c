#include <device/keyboard.h>
#include <declspec.h>
#include <memory/page.h>
#include <system.h>

CODEDECL KEY_EVENT_RING KEY_RING;

void KeyEvent(KEY_EVENT_RING *ring, BYTE keycode)
{
	if (((ring->NID + 1) % ring->CNT) != ring->EID)
	{
		ring->RNG[ring->NID++] = keycode;
		ring->NID %= ring->CNT;
	}
}
BYTE KeyNext(KEY_EVENT_RING *ring)
{
	if (ring->EID != ring->NID)
	{
		BYTE keycode = ring->RNG[ring->EID++];
		ring->EID %= ring->CNT;
		return keycode;
	}
	return 0;
}
void CreateKeyEventRing(KEY_EVENT_RING *ring)
{
	QWORD pAddr = 0;
	QWORD pCont = 1;
	AllocatePhysicalMemory(&pAddr, PAGE4_4K, &pCont);
	pAddr |= SYSTEM_LINEAR;
	ring->RNG = (BYTE *) pAddr;
	ring->CNT = 4096;
	ring->NID = ring->EID = 0;
}
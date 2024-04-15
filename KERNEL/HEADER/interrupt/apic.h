#ifndef __KERNEL_APIC_H__
#define __KERNEL_APIC_H__

#include <types.h>

int check_apic();
void SetupAPIC();
void setup_apic_timer(DWORD);
DWORD CurrentAPIC();
void StartupAP(DWORD);
void APMainStartup();
void ConfigureAPIC();
void InterruptIPI(DWORD, BYTE);

#endif

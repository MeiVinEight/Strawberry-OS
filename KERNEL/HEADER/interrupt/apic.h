#ifndef __KERNEL_APIC_H__
#define __KERNEL_APIC_H__

int check_apic();
void setup_apic();
void setup_apic_timer(DWORD);

#endif
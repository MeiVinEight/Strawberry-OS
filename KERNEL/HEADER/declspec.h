#ifndef __KERNEL_DECPSPEC_H__
#define __KERNEL_DECLSPEC_H__

#define SECTION "CODE"

#pragma section(SECTION, read, write)

#define CODEDECL __declspec(allocate(SECTION))

#endif
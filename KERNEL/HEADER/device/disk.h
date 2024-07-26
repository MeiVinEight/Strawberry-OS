#ifndef __KERNEL_DEVICE_DISK_H__
#define __KERNEL_DEVICE_DISK_H__

#include <types.h>
#include <device/driver.h>

#define DISK_RET_SUCCESS       0x00
#define DISK_RET_EPARAM        0x01
#define DISK_RET_EADDRNOTFOUND 0x02
#define DISK_RET_EWRITEPROTECT 0x03
#define DISK_RET_ECHANGED      0x06
#define DISK_RET_EBOUNDARY     0x09
#define DISK_RET_EBADTRACK     0x0c
#define DISK_RET_ECONTROLLER   0x20
#define DISK_RET_ETIMEOUT      0x80
#define DISK_RET_ENOTLOCKED    0xb0
#define DISK_RET_ELOCKED       0xb1
#define DISK_RET_ENOTREMOVABLE 0xb2
#define DISK_RET_ETOOMANYLOCKS 0xb4
#define DISK_RET_EMEDIA        0xC0
#define DISK_RET_ENOTREADY     0xAA

#define CMD_RESET   0x00
#define CMD_READ    0x02
#define CMD_WRITE   0x03
#define CMD_VERIFY  0x04
#define CMD_FORMAT  0x05
#define CMD_SEEK    0x07
#define CMD_ISREADY 0x10
#define CMD_SCSI    0x20

typedef struct _DISK_OPERATION DISK_OPERATION;
typedef DWORD (*DISK_OPERATOR)(DISK_OPERATION *);
typedef struct _DISK_CHS
{
    WORD HEAD;
    WORD CYLD;
    WORD SECT;
    WORD PADD;
} DISK_CHS;
typedef struct _DISK_DRIVER
{
    DEVICE_DRIVER DVR;
    DISK_OPERATOR OP;
    QWORD         SCT;  // Total sectors count
    DISK_CHS      LCHS; // Logical CHS
    DWORD         ID;   // Unique id for a given driver type.
    BYTE          FLPT; // Type of floppy (only for floppy drives).
    BYTE          RMV;  // Is media removable (currently unused)

    // Info for EDD calls
    DISK_CHS      PCHS; // Physical CHS
    DWORD         MSS;  // max_segment_size
    DWORD         MS;   // max_segments
    BYTE          TRAN; // type of translation
    WORD          BS;   // block size
} DISK_DRIVER;
typedef struct _DISK_OPERATION
{
    void        *DAT;
    DISK_DRIVER *DRV;
    // Commands: SCSI
    void        *CDB;
    QWORD        LBA;
    WORD         BSZ;
    WORD         CNT;
    // Commands: READ, WRITE, VERIFY, SEEK, FORMAT
    BYTE         CMD;
} DISK_OPERATION;

DWORD ExecuteDiskOperation(DISK_OPERATION *);
int DefaultDiskOperation(DISK_OPERATION *);
DWORD DISKRW(DISK_DRIVER *, void *, QWORD, WORD, BYTE);

#endif
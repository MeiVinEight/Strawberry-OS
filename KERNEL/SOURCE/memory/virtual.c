#include <memory/virtual.h>
#include <declspec.h>
#include <memory/block.h>
#include <memory/heap.h>
#include <intrinsic.h>
#include <console/console.h>
#include <memory/page.h>

CODEDECL MEMORY_BLOCK *VTL_FRE = 0;
CODEDECL MEMORY_BLOCK *VTL_RSV = 0;
CODEDECL MEMORY_BLOCK *VTL_CMT = 0;
CODEDECL const char MSG0600[] = "SETUP VIRTUAL MEMORY\n";

void SetupVirtualMemory()
{
	OUTPUTTEXT(MSG0600);
	MEMORY_BLOCK *block = HeapAlloc(HEAPK, sizeof(MEMORY_BLOCK));
	memset(block, 0, sizeof(MEMORY_BLOCK));
	// Virtual address 0xFFFF900000000000 + 0x0000100000000000 is free allocate region
	block->A = 0xFFFF900000000000ULL;
	block->S = 0x0000100000000000ULL;
	InsertMemoryNode(&VTL_FRE, block, 1);
}
void UncommitVirtualMemory(QWORD address, QWORD size)
{
	OUTPUTTEXT("UNCOMMIT ");
	PRINTRAX(address, 16);
	OUTCHAR('+');
	PRINTRAX(size, 16);
	LINEFEED();
	QWORD pagedSize = (((size - 1) >> 12) + 1) << 12;

	QWORD searchAddress = 0;
	while (1)
	{
		MEMORY_BLOCK *region = SearchMemoryNode(&VTL_CMT, searchAddress, 1);
		if (!region)
		{
			break;
		}
		// Next begin address
		searchAddress = region->A + region->S;
		// A0+S0 is common region
		QWORD A0 = 0;
		QWORD S0 = 0;
		// A1+S1 is contiguous physical memory
		QWORD A1 = ~0ULL;
		QWORD S1 = 0;
		// If has common region
		if (region->A <= address && region->A + region->S > address)
		{
			A0 = address;
			S0 = region->A + region->S - A0;
			S0 = (S0 < pagedSize) ? S0 : pagedSize;
			// Reduce regions
			QWORD R0 = region->A + region->S;
			QWORD V0 = region->V;
			region->S = address - region->A;
			if (!region->S)
			{
				RemoveMemoryNode(&VTL_CMT, region);
			}
			if (R0 > address + pagedSize)
			{
				MEMORY_BLOCK *rx = HeapAlloc(HEAPK, sizeof(MEMORY_BLOCK));
				memset(rx, 0, sizeof(MEMORY_BLOCK));
				rx->A = address + pagedSize;
				rx->S = R0 - rx->A;
				rx->V = V0;
				InsertMemoryNode(&VTL_CMT, rx, 0);
			}
		}
		else if (address <= region->A && address + pagedSize > region->A)
		{
			A0 = region->A;
			S0 = address + pagedSize - A0;
			S0 = (S0 < region->S) ? S0 : region->S;
			// Reduce region
			region->A += S0;
			region->S -= S0;
			if (!region->S)
			{
				RemoveMemoryNode(&VTL_CMT, region);
			}
		}
		else if (address + pagedSize <= region->A)
		{
			// No more region has common
			break;
		}
		
		// Free common region
		OUTPUTTEXT("COMMON ");
		PRINTRAX(A0, 16);
		OUTCHAR('+');
		PRINTRAX(S0, 16);
		LINEFEED();
		while (S0)
		{
			// This 4K page has physical memory mapping
			QWORD PA = physical_mapping(A0);
			if (~PA)
			{
				if (A1 + S1 != PA)
				{
					FreePhysicalMemory(A1, 0, S1 >> 12);
					A1 = PA;
					S1 = 0;
				}
				S1 += 0x1000;
				linear_unmapping(A0);
			}
			A0 += 0x1000;
			S0 -= 0x1000;
		}
		FreePhysicalMemory(A1, 0, S1 >> 12);
	}
}
void *NtAllocateVirtualMemory(QWORD address, QWORD size, DWORD option, DWORD protect)
{
	// Fix allocate address, upper round to next boundary of 4K
	address = (((address - 1) >> 12) + 1) << 12;
	// Fix allocate size, upper round to multiple of 4K
	QWORD pagedSize = (((size - 1) >> 12) + 1) << 12;
	if (!pagedSize)
	{
		return 0;
	}

	// NULL pointer page is not accessible
	if (!address)
	{
		if (!(option & MEM_RESERVE))
		{
			return 0;
		}
		else
		{
			// NULL for reserve, auto allocate a region from virtual address space
			QWORD searchAddress = 0;
			while (1)
			{
				MEMORY_BLOCK *region = SearchMemoryNode(&VTL_FRE, searchAddress, 1);
				if (!region)
				{
					return 0;
				}
				searchAddress = region->A + region->S;
				if (region->S >= pagedSize)
				{
					address = region->A;
					break;
				}
			}
		}
	}

	// Step.1 RESERVE
	if (option & MEM_RESERVE)
	{
		MEMORY_BLOCK *region = SearchMemoryNode(&VTL_FRE, address, 0);
		if (!region)
		{
			// No more free region usable
			return 0;
		}
		if (region->A > address + pagedSize)
		{
			// Given address can not use: this region not included in any free virtual address block
			return 0;
		}

		QWORD A0 = region->A;
		QWORD S0 = region->S;
		QWORD A1 = address;
		QWORD S1 = pagedSize;
		if (A0 + S0 < A1 + S1)
		{
			return 0;
		}
		// RESERVE this region
		// | Free | Reserved | Free |
		// |         Region         |
		region->S = A1 - A0;
		if ((A0 + S0) - (A1 + S1))
		{
			MEMORY_BLOCK *block = HeapAlloc(HEAPK, sizeof(MEMORY_BLOCK));
			memset(block, 0, sizeof(MEMORY_BLOCK));
			block->A = A1 + S1;
			block->S = A0 + S0 - block->A;
			InsertMemoryNode(&VTL_FRE, block, 1);
		}
		if (!region->S)
		{
			RemoveMemoryNode(&VTL_FRE, region);
		}
		// Insert reserved region into reserve map
		MEMORY_BLOCK *rsv = HeapAlloc(HEAPK, sizeof(MEMORY_BLOCK));
		memset(rsv, 0, sizeof(MEMORY_BLOCK));
		rsv->A = address;
		rsv->S = pagedSize;
		InsertMemoryNode(&VTL_RSV, rsv, 1);
	}

	// Step.2 COMMIT
	if (option & MEM_COMMIT)
	{
		// Check reserved map
		MEMORY_BLOCK *rsv = SearchMemoryNode(&VTL_RSV, address, 0);
		if (!rsv)
		{
			return 0;
		}
		if (rsv->A + rsv->S - address < pagedSize)
		{
			// Reserved memory not enough
			return 0;
		}

		// Uncommit all regions which included in this region
		UncommitVirtualMemory(address, pagedSize);

		// Insert region into commit map
		MEMORY_BLOCK *cr = HeapAlloc(HEAPK, sizeof(MEMORY_BLOCK));
		memset(cr, 0, sizeof(MEMORY_BLOCK));
		cr->A = address;
		cr->S = pagedSize;
		cr->V = protect;
		InsertMemoryNode(&VTL_CMT, cr, 0);
	}

	// Step.3 UNCOMMIT
	if (option & MEM_UNCOMMIT)
	{
		MEMORY_BLOCK *rsv = SearchMemoryNode(&VTL_RSV, address, 0);
		if (!rsv)
		{
			return 0;
		}
		if (rsv->A + rsv->S - address < pagedSize)
		{
			return 0;
		}
		UncommitVirtualMemory(address, pagedSize);
	}

	// Step.4 FREE
	if (option & MEM_FREE)
	{
		MEMORY_BLOCK blk = {0};
		MEMORY_BLOCK *rsv = SearchMemoryNode(&VTL_RSV, address, 0);
		if (!rsv)
		{
			return 0;
		}
		if (rsv->A + rsv->S < address + pagedSize)
		{
			return 0;
		}

		UncommitVirtualMemory(address, pagedSize);
		blk.A = address + pagedSize;
		blk.S = (rsv->A + rsv->S) - blk.A;
		rsv->S = address - rsv->A;
		if (!rsv->S)
		{
			RemoveMemoryNode(&VTL_RSV, rsv);
		}
		if (blk.S)
		{
			MEMORY_BLOCK *block = HeapAlloc(HEAPK, sizeof(MEMORY_BLOCK));
			memcpy(block, &blk, sizeof(MEMORY_BLOCK));
			InsertMemoryNode(&VTL_RSV, block, 1);
		}
		MEMORY_BLOCK *block = HeapAlloc(HEAPK, sizeof(MEMORY_BLOCK));
		memset(block, 0, sizeof(MEMORY_BLOCK));
		block->A = address;
		block->S = pagedSize;
		InsertMemoryNode(&VTL_FRE, block, 1);
	}

	return (void *) address;
}

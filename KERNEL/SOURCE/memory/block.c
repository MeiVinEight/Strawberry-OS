#include <memory/block.h>
#include <memory/heap.h>
#include <declspec.h>

/*
* Get node reference:
* Which is a pointer to the node's address.
*/
MEMORY_BLOCK **NodeReference(MEMORY_BLOCK **root, MEMORY_BLOCK *block)
{
	if (block->P)
	{
		if (block->P->L == block)
		{
			return &block->P->L;
		}
		else
		{
			return &block->P->R;
		}
	}
	else
	{
		return root;
	}
	return 0;
}
QWORD TreeHeight(MEMORY_BLOCK *tree)
{
	if (tree)
	{
		return tree->H;
	}
	return 0;
}
void AdjustHeight(MEMORY_BLOCK *block)
{
	QWORD LH = TreeHeight(block->L);
	QWORD RH = TreeHeight(block->R);
	block->H = (LH > RH ? LH : RH) + 1;
}
void AdjustAVL(MEMORY_BLOCK **reference, DWORD type)
{
	MEMORY_BLOCK *block = *reference;
	MEMORY_BLOCK *P = block->P;
	MEMORY_BLOCK *L = block->L;
	MEMORY_BLOCK *R = block->R;
	switch (type)
	{
		case 0:
		{
			// LR
			if (R->L)
			{
				R->L->P = block;
			}
			block->R = R->L;
			block->P = R;
			R->L = block;
			R->P = P;
			*reference = R;
			break;
		}
		case 1:
		{
			// RR
			if (L->R)
			{
				L->R->P = block;
			}
			block->L = L->R;
			block->P = L;
			L->R = block;
			L->P = P;
			*reference = L;
			break;
		}
	}
	AdjustHeight(block);
	AdjustHeight(*reference);
}
void AdjustMemoryMap(MEMORY_BLOCK **root, MEMORY_BLOCK *block)
{
	while (block)
	{
		MEMORY_BLOCK **reference = NodeReference(root, block);
		MEMORY_BLOCK *P = block->P;
		MEMORY_BLOCK *L = block->L;
		MEMORY_BLOCK *R = block->R;

		QWORD LH = TreeHeight(L);
		QWORD RH = TreeHeight(R);

		if (LH > (RH + 1))
		{
			// Right rotate
			// Assume left child is not null
			if (TreeHeight(L->R) > TreeHeight(L->L))
			{
				AdjustAVL(&block->L, 0);
			}
			AdjustAVL(NodeReference(root, block), 1);
		}
		else if (RH > (LH + 1))
		{
			// Left rotate
			// Assume right child is not null
			if (TreeHeight(R->L) > TreeHeight(R->R))
			{
				AdjustAVL(&block->R, 1);
			}
			AdjustAVL(NodeReference(root, block), 0);
		}
		AdjustHeight(*reference);
		block = (*reference)->P;
	}
}
void RemoveMemoryNode(MEMORY_BLOCK **root, MEMORY_BLOCK *block)
{
	QWORD lc = (QWORD) block->L;
	QWORD rc = (QWORD) block->R;
	if (lc && rc)
	{
		// Left and right children are not null.
		// Find the greatest node which is less than the block.
		MEMORY_BLOCK *prev = block->L;
		while (prev->R)
		{
			prev = prev->R;
		}
		// Copy data in prev to block, means block deleted and prev is invalid.
		block->H = prev->H;
		block->A = prev->A;
		block->S = prev->S;
		block->V = prev->V;
		// Delete node prev.
		// Because the node's right children is null,
		// only needs to move the left children to parent's pointer.
		*NodeReference(root, prev) = prev->L;
		if (prev->L)
		{
			prev->L->P = prev->P;
		}
		// Make block point to the deleted node.
		block = prev;
	}
	else
	{
		// Has up to one child
		MEMORY_BLOCK *child = (MEMORY_BLOCK *) (lc | rc);
		// Move the child to parent's pointer
		*NodeReference(root, block) = child;
		if (child)
		{
			child->P = block->P;
		}
	}
	// Recursively adjust tree
	AdjustMemoryMap(root, block->P);
	// Free the memory of the node.
	HeapFree(block);
}
void InsertMemoryNode(MEMORY_BLOCK **root, MEMORY_BLOCK *block, DWORD merge)
{
	if (merge)
	{
		// Find smallest value which greater than the block
		MEMORY_BLOCK *next = SearchMemoryNode(root, block->A, 1);
		// Find biggest value which less than the block
		MEMORY_BLOCK *prev = SearchMemoryNode(root, block->A, 0);

		// Check merge
		if (prev && prev->A + prev->S >= block->A)
		{
			QWORD A = prev->A;
			QWORD Z = block->A + block->S;
			if (prev->A + prev->S > Z) Z = prev->A + prev->S;
			block->A = A;
			block->S = Z - A;
			RemoveMemoryNode(root, prev);
		}
		if (next && block->A + block->S >= next->A)
		{
			QWORD A = block->A;
			QWORD Z = next->A + next->S;
			if (block->A + block->S > Z) Z = block->A + block->S;
			block->A = A;
			block->S = Z - A;
			RemoveMemoryNode(root, next);
		}
	}

	// Insert block into memory map
	MEMORY_BLOCK *parent = 0;
	MEMORY_BLOCK **reference = root;
	while (*reference)
	{
		if (block->A > (*reference)->A)
		{
			// Insert into right subtree
			parent = *reference;
			reference = &(*reference)->R;
		}
		else
		{
			// Insert into left subtree
			parent = *reference;
			reference = &(*reference)->L;
		}
	}
	block->P = parent;
	*reference = block;
	AdjustMemoryMap(root, block);
}
MEMORY_BLOCK *SearchMemoryNode(MEMORY_BLOCK **root, QWORD address, DWORD option)
{
	switch (option)
	{
		case 0:
		{
			// Find biggest value which is less than the block
			MEMORY_BLOCK *next = 0;
			MEMORY_BLOCK *curr = *root;
			while (curr)
			{
				if (curr->A <= address)
				{
					next = curr;
					curr = curr->R;
				}
				else
				{
					curr = curr->L;
				}
			}
			return next;
		}
		case 1:
		{
			// Find smallest value which is greater than the block
			MEMORY_BLOCK *next = 0;
			MEMORY_BLOCK *curr = *root;
			while (curr)
			{
				if (curr->A >= address)
				{
					next = curr;
					curr = curr->L;
				}
				else
				{
					curr = curr->R;
				}
			}
			return next;
		}
		default: return 0;
	}
}

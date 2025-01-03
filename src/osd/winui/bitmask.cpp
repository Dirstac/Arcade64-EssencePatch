// license:BSD-3-Clause
// For licensing and usage information, read docs/release/winui_license.txt

#include "winui.h"

/* Bit routines */
static uint8_t maskTable[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

/* Create a new LPBITS struct and return the new structure
 * initialized to 'all bits cleared'
 *
 * nLength is the number of desired bits
 */
LPBITS NewBits(UINT nLength)
{
	LPBITS lpBits = (LPBITS)malloc(sizeof(BITS));
	UINT nSize = (nLength + 7) / 8;
	
	if (lpBits)
	{
		lpBits->m_lpBits = (uint8_t *)malloc(nSize);
		
		if (lpBits->m_lpBits)
		{
			memset(lpBits->m_lpBits, 0, nSize);
			lpBits->m_nSize = nSize;
		}
		else
		{
			free(lpBits);
			lpBits = 0;
		}
	}

	return lpBits;
}

void DeleteBits(LPBITS lpBits)
{
	if (!lpBits)
		return;

	if (lpBits->m_nSize && lpBits->m_lpBits)
		free(lpBits->m_lpBits);

	free(lpBits);
}

/* Test the 'nBit'th bit */
bool TestBit(LPBITS lpBits, UINT nBit)
{
	if (nBit < 0 || !lpBits || !lpBits->m_lpBits)
		return false;

	UINT offset = nBit >> 3;

	if (offset >= lpBits->m_nSize)
		return false;

	uint8_t mask = maskTable[nBit & 7];
	return	(lpBits->m_lpBits[offset] & mask) ? true : false;
}

/* Set the 'nBit'th bit */
void SetBit(LPBITS lpBits, UINT nBit)
{
	if (nBit < 0 || !lpBits || !lpBits->m_lpBits)
		return;

	UINT offset = nBit >> 3;

	if (offset >= lpBits->m_nSize)
		return;

	uint8_t mask = maskTable[nBit & 7];
	lpBits->m_lpBits[offset] |= mask;
}

/* Clear the 'nBit'th bit */
void ClearBit(LPBITS lpBits, UINT nBit)
{
	if (nBit < 0 || !lpBits || !lpBits->m_lpBits)
		return;

	UINT offset = nBit >> 3;

	if (offset >= lpBits->m_nSize)
		return;

	uint8_t mask = maskTable[nBit & 7];
	lpBits->m_lpBits[offset] &= ~mask;
}

/* Set or Clear all bits as specified by 'bSet' */
void SetAllBits(LPBITS lpBits, bool bSet)
{
	if (lpBits && lpBits->m_nSize != 0 && lpBits->m_lpBits)
		memset(lpBits->m_lpBits, (!bSet) ? 0 : 0xff, lpBits->m_nSize);
}

/* Find next bit that matches 'bSet'
 *
 * 'nStartPos' specifies the bit to start search after
 * 'bSet' specifies to search for a set or unset bit
 *
 * Returns -1 if no bits are found
 */
int FindBit(LPBITS lpBits, int nStartPos, bool bSet)
{
	if (!lpBits || !lpBits->m_nSize || !lpBits->m_lpBits)
		return -1;

	UINT end = lpBits->m_nSize << 3;

	if (nStartPos < 0)
		nStartPos = 0;

	for (int i = nStartPos; i < end; i++)
	{
		bool res = (TestBit(lpBits, i)) ? true : false;
		
		if ((res && bSet) || (!res && !bSet))
			return i;
	}

	return -1;
}

// license:BSD-3-Clause
// copyright-holders:Paul Leaman, Andreas Naive, Nicola Salmoria,Charles MacDonald
/******************************************************************************

CPS-2 Encryption

All credit goes to Andreas Naive for breaking the encryption algorithm.
Code by Nicola Salmoria.
Thanks to Charles MacDonald and Razoola for extracting the data from the hardware.


The encryption only affects opcodes, not data.

It consists of two 4-round Feistel networks (FN) and involves both
the 16-bit opcode and the low 16 bits of the address.

Let be:

E = 16-bit ciphertext
A = 16-bit address
K = 64-bit key
D = 16-bit plaintext
y = FN1(x,k) = function describing the first Feistel network (x,y = 16 bit, k = 64 bit)
y = FN2(x,k) = function describing the second Feistel network (x,y = 16 bit, k = 64 bit)
y = EX(x) = fixed function that expands the 16-bit x to the 64-bit y

Then the cipher can be described as:

D = FN2( E, K XOR EX( FN1(A, K ) ) )


Each round of the Feistel networks consists of four substitution boxes. The boxes
have 6 inputs and 2 outputs. Usually the input is the XOR of a data bit and a key
bit, however in some cases only the key is used.

(TODO-notes about accuracy of s-boxes)

The s-boxes were chosen in order to use an empty key (all FF) for the dead board.


Also, the hardware has different watchdog opcodes and address range (see below)
which are stored in the battery backed RAM. There doesn't appear to be any relation
between those and the 64-bit encryption key, so they probably use an additional
64 bits of battery-backed RAM.



First FN:

 B(0 1 3 5 8 9 11 12)        A(10 4 6 7 2 13 15 14)
         L0                             R0
         |                              |
        XOR<-----------[F1]<------------|
         |                              |
         R1                             L1
         |                              |
         |------------>[F2]----------->XOR
         |                              |
         L2                             R2
         |                              |
        XOR<-----------[F3]<------------|
         |                              |
         R3                             L3
         |                              |
         |------------>[F4]----------->XOR
         |                              |
         L4                             R4
  (10 4 6 7 2 13 15 14)       (0 1 3 5 8 9 11 12)


Second FN:

 B(3 5 9 10 8 15 12 11)      A(6 0 2 13 1 4 14 7)
         L0                             R0
         |                              |
        XOR<-----------[F1]<------------|
         |                              |
         R1                             L1
         |                              |
         |------------>[F2]----------->XOR
         |                              |
         L2                             R2
         |                              |
        XOR<-----------[F3]<------------|
         |                              |
         R3                             L3
         |                              |
         |------------>[F4]----------->XOR
         |                              |
         L4                             R4
  (6 0 2 13 1 4 14 7)         (3 5 9 10 8 15 12 11)

******************************************************************************

Some Encryption notes.
----------------------

Address range.

The encryption does _not_ cover the entire address space. The range covered
differs per game.


Encryption Watchdog.

The CPS2 system has a watchdog system that will disable the decryption
of data if the watchdog isn't triggered at least once every few seconds.
The trigger varies from game to game (some games do use the same) and is
basically a 68000 opcode/s instruction. The instruction is the same for
all regions of the game. The watchdog instructions are listed alongside
the decryption keys.

*******************************************************************************/

#include "emu.h"
#include "cps1.h"

#include "cpu/m68000/m68000.h"

#include "ui/uimain.h"


namespace {

/******************************************************************************/

const int fn1_groupA[8] = { 10, 4, 6, 7, 2, 13, 15, 14 };
const int fn1_groupB[8] = {  0, 1, 3, 5, 8,  9, 11, 12 };

const int fn2_groupA[8] = { 6, 0, 2, 13, 1,  4, 14,  7 };
const int fn2_groupB[8] = { 3, 5, 9, 10, 8, 15, 12, 11 };

/******************************************************************************/

// The order of the input and output bits in the s-boxes is arbitrary.
// Each s-box can be XORed with an arbitrary vale in range 0-3 (but the same value
// must be used for the corresponding output bits in f1 and f3 or in f2 and f4)

struct sbox
{
	int extract_inputs(uint32_t val) const
	{
		int res = 0;

		for (int i = 0; i < 6; ++i)
		{
			if (inputs[i] >= 0)
				res |= BIT(val, inputs[i]) << i;
		}

		return res;
	}

	const uint8_t table[64];
	const int inputs[6];        // positions of the inputs bits, -1 means no input except from key
	const int outputs[2];       // positions of the output bits
};

// the above struct better defines how the hardware works, however
// to speed up the decryption at run time we convert it to the
// following one
class optimised_sbox
{
public:
	void optimise(sbox const &in)
	{
		// precalculate the input lookup
		for (int i = 0; i < 256; ++i)
			input_lookup[i] = in.extract_inputs(i);

		// precalculate the output masks
		for (int i = 0; i < 64; ++i)
		{
			int const o = in.table[i];

			output[i] = 0;
			if (o & 1)
				output[i] |= 1 << in.outputs[0];
			if (o & 2)
				output[i] |= 1 << in.outputs[1];
		}
	}

	uint8_t fn(uint8_t in, uint32_t key) const
	{
		return output[input_lookup[in] ^ (key & 0x3f)];
	}

private:
	uint8_t input_lookup[256];
	uint8_t output[64];
};


const sbox fn1_r1_boxes[4] =
{
	{   // subkey bits  0- 5
		{
			0,2,2,0,1,0,1,1,3,2,0,3,0,3,1,2,1,1,1,2,1,3,2,2,2,3,3,2,1,1,1,2,
			2,2,0,0,3,1,3,1,1,1,3,0,0,1,0,0,1,2,2,1,2,3,2,2,2,3,1,3,2,0,1,3,
		},
		{ 3, 4, 5, 6, -1, -1 },
		{ 3, 6 }
	},
	{   // subkey bits  6-11
		{
			3,0,2,2,2,1,1,1,1,2,1,0,0,0,2,3,2,3,1,3,0,0,0,2,1,2,2,3,0,3,3,3,
			0,1,3,2,3,3,3,1,1,1,1,2,0,1,2,1,3,2,3,1,1,3,2,2,2,3,1,3,2,3,0,0,
		},
		{ 0, 1, 2, 4, 7, -1 },
		{ 2, 7 }
	},
	{   // subkey bits 12-17
		{
			3,0,3,1,1,0,2,2,3,1,2,0,3,3,2,3,0,1,0,1,2,3,0,2,0,2,0,1,0,0,1,0,
			2,3,1,2,1,0,2,0,2,1,0,1,0,2,1,0,3,1,2,3,1,3,1,1,1,2,0,2,2,0,0,0,
		},
		{ 0, 1, 2, 3, 6, 7 },
		{ 0, 1 }
	},
	{   // subkey bits 18-23
		{
			3,2,0,3,0,2,2,1,1,2,3,2,1,3,2,1,2,2,1,3,3,2,1,0,1,0,1,3,0,0,0,2,
			2,1,0,1,0,1,0,1,3,1,1,2,2,3,2,0,3,3,2,0,2,1,3,3,0,0,3,0,1,1,3,3,
		},
		{ 0, 1, 3, 5, 6, 7 },
		{ 4, 5 }
	},
};

const sbox fn1_r2_boxes[4] =
{
	{   // subkey bits 24-29
		{
			3,3,2,0,3,0,3,1,0,3,0,1,0,2,1,3,1,3,0,3,3,1,3,3,3,2,3,2,2,3,1,2,
			0,2,2,1,0,1,2,0,3,3,0,1,3,2,1,2,3,0,1,3,0,1,2,2,1,2,1,2,0,1,3,0,
		},
		{ 0, 1, 2, 3, 6, -1 },
		{ 1, 6 }
	},
	{   // subkey bits 30-35
		{
			1,2,3,2,1,3,0,1,1,0,2,0,0,2,3,2,3,3,0,1,2,2,1,0,1,0,1,2,3,2,1,3,
			2,2,2,0,1,0,2,3,2,1,2,1,2,1,0,3,0,1,2,3,1,2,1,3,2,0,3,2,3,0,2,0,
		},
		{ 2, 4, 5, 6, 7, -1 },
		{ 5, 7 }
	},
	{   // subkey bits 36-41
		{
			0,1,0,2,1,1,0,1,0,2,2,2,1,3,0,0,1,1,3,1,2,2,2,3,1,0,3,3,3,2,2,2,
			1,1,3,0,3,1,3,0,1,3,3,2,1,1,0,0,1,2,2,2,1,1,1,2,2,0,0,3,2,3,1,3,
		},
		{ 1, 2, 3, 4, 5, 7 },
		{ 0, 3 }
	},
	{   // subkey bits 42-47
		{
			2,1,0,3,3,3,2,0,1,2,1,1,1,0,3,1,1,3,3,0,1,2,1,0,0,0,3,0,3,0,3,0,
			1,3,3,3,0,3,2,0,2,1,2,2,2,1,1,3,0,1,0,1,0,1,1,1,1,3,1,0,1,2,3,3,
		},
		{ 0, 1, 3, 4, 6, 7 },
		{ 2, 4 }
	},
};

const sbox fn1_r3_boxes[4] =
{
	{   // subkey bits 48-53
		{
			0,0,0,3,3,1,1,0,2,0,2,0,0,0,3,2,0,1,2,3,2,2,1,0,3,0,0,0,0,0,2,3,
			3,0,0,1,1,2,3,3,0,1,3,2,0,1,3,3,2,0,0,1,0,2,0,0,0,3,1,3,3,3,3,3,
		},
		{ 0, 1, 5, 6, 7, -1 },
		{ 0, 5 }
	},
	{   // subkey bits 54-59
		{
			2,3,2,3,0,2,3,0,2,2,3,0,3,2,0,2,1,0,2,3,1,1,1,0,0,1,0,2,1,2,2,1,
			3,0,2,1,2,3,3,0,3,2,3,1,0,2,1,0,1,2,2,3,0,2,1,3,1,3,0,2,1,1,1,3,
		},
		{ 2, 3, 4, 6, 7, -1 },
		{ 6, 7 }
	},
	{   // subkey bits 60-65
		{
			3,0,2,1,1,3,1,2,2,1,2,2,2,0,0,1,2,3,1,0,2,0,0,2,3,1,2,0,0,0,3,0,
			2,1,1,2,0,0,1,2,3,1,1,2,0,1,3,0,3,1,1,0,0,2,3,0,0,0,0,3,2,0,0,0,
		},
		{ 0, 2, 3, 4, 5, 6 },
		{ 1, 4 }
	},
	{   // subkey bits 66-71
		{
			0,1,0,0,2,1,3,2,3,3,2,1,0,1,1,1,1,1,0,3,3,1,1,0,0,2,2,1,0,3,3,2,
			1,3,3,0,3,0,2,1,1,2,3,2,2,2,1,0,0,3,3,3,2,2,3,1,0,2,3,0,3,1,1,0,
		},
		{ 0, 1, 2, 3, 5, 7 },
		{ 2, 3 }
	},
};

const sbox fn1_r4_boxes[4] =
{
	{   // subkey bits 72-77
		{
			1,1,1,1,1,0,1,3,3,2,3,0,1,2,0,2,3,3,0,1,2,1,2,3,0,3,2,3,2,0,1,2,
			0,1,0,3,2,1,3,2,3,1,2,3,2,0,1,2,2,0,0,0,2,1,3,0,3,1,3,0,1,3,3,0,
		},
		{ 1, 2, 3, 4, 5, 7 },
		{ 0, 4 }
	},
	{   // subkey bits 78-83
		{
			3,0,0,0,0,1,0,2,3,3,1,3,0,3,1,2,2,2,3,1,0,0,2,0,1,0,2,2,3,3,0,0,
			1,1,3,0,2,3,0,3,0,3,0,2,0,2,0,1,0,3,0,1,3,1,1,0,0,1,3,3,2,2,1,0,
		},
		{ 0, 1, 2, 3, 5, 6 },
		{ 1, 3 }
	},
	{   // subkey bits 84-89
		{
			0,1,1,2,0,1,3,1,2,0,3,2,0,0,3,0,3,0,1,2,2,3,3,2,3,2,0,1,0,0,1,0,
			3,0,2,3,0,2,2,2,1,1,0,2,2,0,0,1,2,1,1,1,2,3,0,3,1,2,3,3,1,1,3,0,
		},
		{ 0, 2, 4, 5, 6, 7 },
		{ 2, 6 }
	},
	{   // subkey bits 90-95
		{
			0,1,2,2,0,1,0,3,2,2,1,1,3,2,0,2,0,1,3,3,0,2,2,3,3,2,0,0,2,1,3,3,
			1,1,1,3,1,2,1,1,0,3,3,2,3,2,3,0,3,1,0,0,3,0,0,0,2,2,2,1,2,3,0,0,
		},
		{ 0, 1, 3, 4, 6, 7 },
		{ 5, 7 }
	},
};

/******************************************************************************/

const sbox fn2_r1_boxes[4] =
{
	{   // subkey bits  0- 5
		{
			2,0,2,0,3,0,0,3,1,1,0,1,3,2,0,1,2,0,1,2,0,2,0,2,2,2,3,0,2,1,3,0,
			0,1,0,1,2,2,3,3,0,3,0,2,3,0,1,2,1,1,0,2,0,3,1,1,2,2,1,3,1,1,3,1,
		},
		{ 0, 3, 4, 5, 7, -1 },
		{ 6, 7 }
	},
	{   // subkey bits  6-11
		{
			1,1,0,3,0,2,0,1,3,0,2,0,1,1,0,0,1,3,2,2,0,2,2,2,2,0,1,3,3,3,1,1,
			1,3,1,3,2,2,2,2,2,2,0,1,0,1,1,2,3,1,1,2,0,3,3,3,2,2,3,1,1,1,3,0,
		},
		{ 1, 2, 3, 4, 6, -1 },
		{ 3, 5 }
	},
	{   // subkey bits 12-17
		{
			1,0,2,2,3,3,3,3,1,2,2,1,0,1,2,1,1,2,3,1,2,0,0,1,2,3,1,2,0,0,0,2,
			2,0,1,1,0,0,2,0,0,0,2,3,2,3,0,1,3,0,0,0,2,3,2,0,1,3,2,1,3,1,1,3,
		},
		{ 1, 2, 4, 5, 6, 7 },
		{ 1, 4 }
	},
	{   // subkey bits 18-23
		{
			1,3,3,0,3,2,3,1,3,2,1,1,3,3,2,1,2,3,0,3,1,0,0,2,3,0,0,0,3,3,0,1,
			2,3,0,0,0,1,2,1,3,0,0,1,0,2,2,2,3,3,1,2,1,3,0,0,0,3,0,1,3,2,2,0,
		},
		{ 0, 2, 3, 5, 6, 7 },
		{ 0, 2 }
	},
};

const sbox fn2_r2_boxes[4] =
{
	{   // subkey bits 24-29
		{
			3,1,3,0,3,0,3,1,3,0,0,1,1,3,0,3,1,1,0,1,2,3,2,3,3,1,2,2,2,0,2,3,
			2,2,2,1,1,3,3,0,3,1,2,1,1,1,0,2,0,3,3,0,0,2,0,0,1,1,2,1,2,1,1,0,
		},
		{ 0, 2, 4, 6, -1, -1 },
		{ 4, 6 }
	},
	{   // subkey bits 30-35
		{
			0,3,0,3,3,2,1,2,3,1,1,1,2,0,2,3,0,3,1,2,2,1,3,3,3,2,1,2,2,0,1,0,
			2,3,0,1,2,0,1,1,2,0,2,1,2,0,2,3,3,1,0,2,3,3,0,3,1,1,3,0,0,1,2,0,
		},
		{ 1, 3, 4, 5, 6, 7 },
		{ 0, 3 }
	},
	{   // subkey bits 36-41
		{
			0,0,2,1,3,2,1,0,1,2,2,2,1,1,0,3,1,2,2,3,2,1,1,0,3,0,0,1,1,2,3,1,
			3,3,2,2,1,0,1,1,1,2,0,1,2,3,0,3,3,0,3,2,2,0,2,2,1,2,3,2,1,0,2,1,
		},
		{ 0, 1, 3, 4, 5, 7 },
		{ 1, 7 }
	},
	{   // subkey bits 42-47
		{
			0,2,1,2,0,2,2,0,1,3,2,0,3,2,3,0,3,3,2,3,1,2,3,1,2,2,0,0,2,2,1,2,
			2,3,3,3,1,1,0,0,0,3,2,0,3,2,3,1,1,1,1,0,1,0,1,3,0,0,1,2,2,3,2,0,
		},
		{ 1, 2, 3, 5, 6, 7 },
		{ 2, 5 }
	},
};

const sbox fn2_r3_boxes[4] =
{
	{   // subkey bits 48-53
		{
			2,1,2,1,2,3,1,3,2,2,1,3,3,0,0,1,0,2,0,3,3,1,0,0,1,1,0,2,3,2,1,2,
			1,1,2,1,1,3,2,2,0,2,2,3,3,3,2,0,0,0,0,0,3,3,3,0,1,2,1,0,2,3,3,1,
		},
		{ 2, 3, 4, 6, -1, -1 },
		{ 3, 5 }
	},
	{   // subkey bits 54-59
		{
			3,2,3,3,1,0,3,0,2,0,1,1,1,0,3,0,3,1,3,1,0,1,2,3,2,2,3,2,0,1,1,2,
			3,0,0,2,1,0,0,2,2,0,1,0,0,2,0,0,1,3,1,3,2,0,3,3,1,0,2,2,2,3,0,0,
		},
		{ 0, 1, 3, 5, 7, -1 },
		{ 0, 2 }
	},
	{   // subkey bits 60-65
		{
			2,2,1,0,2,3,3,0,0,0,1,3,1,2,3,2,2,3,1,3,0,3,0,3,3,2,2,1,0,0,0,2,
			1,2,2,2,0,0,1,2,0,1,3,0,2,3,2,1,3,2,2,2,3,1,3,0,2,0,2,1,0,3,3,1,
		},
		{ 0, 1, 2, 3, 5, 7 },
		{ 1, 6 }
	},
	{   // subkey bits 66-71
		{
			1,2,3,2,0,2,1,3,3,1,0,1,1,2,2,0,0,1,1,1,2,1,1,2,0,1,3,3,1,1,1,2,
			3,3,1,0,2,1,1,1,2,1,0,0,2,2,3,2,3,2,2,0,2,2,3,3,0,2,3,0,2,2,1,1,
		},
		{ 0, 2, 4, 5, 6, 7 },
		{ 4, 7 }
	},
};

const sbox fn2_r4_boxes[4] =
{
	{   // subkey bits 72-77
		{
			2,0,1,1,2,1,3,3,1,1,1,2,0,1,0,2,0,1,2,0,2,3,0,2,3,3,2,2,3,2,0,1,
			3,0,2,0,2,3,1,3,2,0,0,1,1,2,3,1,1,1,0,1,2,0,3,3,1,1,1,3,3,1,1,0,
		},
		{ 0, 1, 3, 6, 7, -1 },
		{ 0, 3 }
	},
	{   // subkey bits 78-83
		{
			1,2,2,1,0,3,3,1,0,2,2,2,1,0,1,0,1,1,0,1,0,2,1,0,2,1,0,2,3,2,3,3,
			2,2,1,2,2,3,1,3,3,3,0,1,0,1,3,0,0,0,1,2,0,3,3,2,3,2,1,3,2,1,0,2,
		},
		{ 0, 1, 2, 4, 5, 6 },
		{ 4, 7 }
	},
	{   // subkey bits 84-89
		{
			2,3,2,1,3,2,3,0,0,2,1,1,0,0,3,2,3,1,0,1,2,2,2,1,3,2,2,1,0,2,1,2,
			0,3,1,0,0,3,1,1,3,3,2,0,1,0,1,3,0,0,1,2,1,2,3,2,1,0,0,3,2,1,1,3,
		},
		{ 0, 2, 3, 4, 5, 7 },
		{ 1, 2 }
	},
	{   // subkey bits 90-95
		{
			2,0,0,3,2,2,2,1,3,3,1,1,2,0,0,3,1,0,3,2,1,0,2,0,3,2,2,3,2,0,3,0,
			1,3,0,2,2,1,3,3,0,1,0,3,1,1,3,2,0,3,0,2,3,2,1,3,2,3,0,0,1,3,2,1,
		},
		{ 2, 3, 4, 5, 6, 7 },
		{ 5, 6 }
	},
};

/******************************************************************************/


uint8_t fn(uint8_t in, const optimised_sbox *sboxes, uint32_t key)
{
	return
			sboxes[0].fn(in, key >>  0) |
			sboxes[1].fn(in, key >>  6) |
			sboxes[2].fn(in, key >> 12) |
			sboxes[3].fn(in, key >> 18);
}



// srckey is the 64-bit master key (2x32 bits)
// dstkey will contain the 96-bit key for the 1st FN (4x24 bits)
void expand_1st_key(uint32_t *dstkey, const uint32_t *srckey)
{
	static const int bits[96] =
	{
		33, 58, 49, 36,  0, 31,
		22, 30,  3, 16,  5, 53,
		10, 41, 23, 19, 27, 39,
		43,  6, 34, 12, 61, 21,
		48, 13, 32, 35,  6, 42,
		43, 14, 21, 41, 52, 25,
		18, 47, 46, 37, 57, 53,
		20,  8, 55, 54, 59, 60,
		27, 33, 35, 18,  8, 15,
		63,  1, 50, 44, 16, 46,
		 5,  4, 45, 51, 38, 25,
		13, 11, 62, 29, 48,  2,
		59, 61, 62, 56, 51, 57,
		54,  9, 24, 63, 22,  7,
		26, 42, 45, 40, 23, 14,
		 2, 31, 52, 28, 44, 17,
	};

	dstkey[0] = 0;
	dstkey[1] = 0;
	dstkey[2] = 0;
	dstkey[3] = 0;

	for (int i = 0; i < 96; ++i)
		dstkey[i / 24] |= BIT(srckey[bits[i] / 32], bits[i] % 32) << (i % 24);
}


// srckey is the 64-bit master key (2x32 bits) XORed with the subkey
// dstkey will contain the 96-bit key for the 2nd FN (4x24 bits)
void expand_2nd_key(uint32_t *dstkey, const uint32_t *srckey)
{
	static const int bits[96] =
	{
		34,  9, 32, 24, 44, 54,
		38, 61, 47, 13, 28,  7,
		29, 58, 18,  1, 20, 60,
		15,  6, 11, 43, 39, 19,
		63, 23, 16, 62, 54, 40,
		31,  3, 56, 61, 17, 25,
		47, 38, 55, 57,  5,  4,
		15, 42, 22,  7,  2, 19,
		46, 37, 29, 39, 12, 30,
		49, 57, 31, 41, 26, 27,
		24, 36, 11, 63, 33, 16,
		56, 62, 48, 60, 59, 32,
		12, 30, 53, 48, 10,  0,
		50, 35,  3, 59, 14, 49,
		51, 45, 44,  2, 21, 33,
		55, 52, 23, 28,  8, 26,
	};
	int i;

	dstkey[0] = 0;
	dstkey[1] = 0;
	dstkey[2] = 0;
	dstkey[3] = 0;

	for (i = 0; i < 96; ++i)
		dstkey[i / 24] |= BIT(srckey[bits[i] / 32], bits[i] % 32) << (i % 24);
}



// seed is the 16-bit seed generated by the first FN
// subkey will contain the 64-bit key to be XORed with the master key
// for the 2nd FN (2x32 bits)
void expand_subkey(uint32_t* subkey, uint16_t seed)
{
	// Note that each row of the table is a permutation of the seed bits.
	static const int bits[64] =
	{
		 5, 10, 14,  9,  4,  0, 15,  6,  1,  8,  3,  2, 12,  7, 13, 11,
		 5, 12,  7,  2, 13, 11,  9, 14,  4,  1,  6, 10,  8,  0, 15,  3,
		 4, 10,  2,  0,  6,  9, 12,  1, 11,  7, 15,  8, 13,  5, 14,  3,
		14, 11, 12,  7,  4,  5,  2, 10,  1, 15,  0,  9,  8,  6, 13,  3,
	};
	int i;

	subkey[0] = 0;
	subkey[1] = 0;

	for (i = 0; i < 64; ++i)
		subkey[i / 32] |= BIT(seed, bits[i]) << (i % 32);
}



uint16_t feistel(uint16_t val, const int *bitsA, const int *bitsB,
		const optimised_sbox* boxes1, const optimised_sbox* boxes2, const optimised_sbox* boxes3, const optimised_sbox* boxes4,
		uint32_t key1, uint32_t key2, uint32_t key3, uint32_t key4)
{
	uint8_t l = bitswap<8>(val, bitsB[7],bitsB[6],bitsB[5],bitsB[4],bitsB[3],bitsB[2],bitsB[1],bitsB[0]);
	uint8_t r = bitswap<8>(val, bitsA[7],bitsA[6],bitsA[5],bitsA[4],bitsA[3],bitsA[2],bitsA[1],bitsA[0]);

	l ^= fn(r, boxes1, key1);
	r ^= fn(l, boxes2, key2);
	l ^= fn(r, boxes3, key3);
	r ^= fn(l, boxes4, key4);

	return
		(BIT(l, 0) << bitsA[0]) |
		(BIT(l, 1) << bitsA[1]) |
		(BIT(l, 2) << bitsA[2]) |
		(BIT(l, 3) << bitsA[3]) |
		(BIT(l, 4) << bitsA[4]) |
		(BIT(l, 5) << bitsA[5]) |
		(BIT(l, 6) << bitsA[6]) |
		(BIT(l, 7) << bitsA[7]) |
		(BIT(r, 0) << bitsB[0]) |
		(BIT(r, 1) << bitsB[1]) |
		(BIT(r, 2) << bitsB[2]) |
		(BIT(r, 3) << bitsB[3]) |
		(BIT(r, 4) << bitsB[4]) |
		(BIT(r, 5) << bitsB[5]) |
		(BIT(r, 6) << bitsB[6]) |
		(BIT(r, 7) << bitsB[7]);
}



void optimise_sboxes(optimised_sbox* out, const sbox* in)
{
	for (int box = 0; box < 4; ++box)
		out[box].optimise(in[box]);
}

} // anonymous namespace


void cps2_decrypt(running_machine &machine, uint16_t *rom, uint16_t *dec, int length, const uint32_t *master_key, uint32_t lower_limit, uint32_t upper_limit)
{
	optimised_sbox sboxes1[4*4];
	optimise_sboxes(&sboxes1[0*4], fn1_r1_boxes);
	optimise_sboxes(&sboxes1[1*4], fn1_r2_boxes);
	optimise_sboxes(&sboxes1[2*4], fn1_r3_boxes);
	optimise_sboxes(&sboxes1[3*4], fn1_r4_boxes);

	optimised_sbox sboxes2[4*4];
	optimise_sboxes(&sboxes2[0*4], fn2_r1_boxes);
	optimise_sboxes(&sboxes2[1*4], fn2_r2_boxes);
	optimise_sboxes(&sboxes2[2*4], fn2_r3_boxes);
	optimise_sboxes(&sboxes2[3*4], fn2_r4_boxes);


	// expand master key to 1st FN 96-bit key
	uint32_t key1[4];
	expand_1st_key(key1, master_key);

	// add extra bits for s-boxes with less than 6 inputs
	key1[0] ^= BIT(key1[0], 1) <<  4;
	key1[0] ^= BIT(key1[0], 2) <<  5;
	key1[0] ^= BIT(key1[0], 8) << 11;
	key1[1] ^= BIT(key1[1], 0) <<  5;
	key1[1] ^= BIT(key1[1], 8) << 11;
	key1[2] ^= BIT(key1[2], 1) <<  5;
	key1[2] ^= BIT(key1[2], 8) << 11;

	for (int i = 0; i < 0x10000; ++i)
	{
		if ((i & 0xff) == 0)
		{
// 修改的 (加斯顿90)
/******************************************************************************************
			auto loadingMessage = util::string_format("Decrypting %d%%", i*100/0x10000);
			machine.ui().set_startup_text(loadingMessage.c_str(), false);
******************************************************************************************/
		}

		// pass the address through FN1
		uint16_t const seed = feistel(i, fn1_groupA, fn1_groupB,
				&sboxes1[0*4], &sboxes1[1*4], &sboxes1[2*4], &sboxes1[3*4],
				key1[0], key1[1], key1[2], key1[3]);

		// expand the result to 64-bit
		uint32_t subkey[2];
		expand_subkey(subkey, seed);

		// XOR with the master key
		subkey[0] ^= master_key[0];
		subkey[1] ^= master_key[1];

		// expand key to 2nd FN 96-bit key
		uint32_t key2[4];
		expand_2nd_key(key2, subkey);

		// add extra bits for s-boxes with less than 6 inputs
		key2[0] ^= BIT(key2[0], 0) <<  5;
		key2[0] ^= BIT(key2[0], 6) << 11;
		key2[1] ^= BIT(key2[1], 0) <<  5;
		key2[1] ^= BIT(key2[1], 1) <<  4;
		key2[2] ^= BIT(key2[2], 2) <<  5;
		key2[2] ^= BIT(key2[2], 3) <<  4;
		key2[2] ^= BIT(key2[2], 7) << 11;
		key2[3] ^= BIT(key2[3], 1) <<  5;

		// decrypt the opcodes
		for (int a = i; a < length/2; a += 0x10000)
		{
			if (a >= lower_limit && a <= upper_limit)
			{
				dec[a] = feistel(rom[a], fn2_groupA, fn2_groupB,
					&sboxes2[0 * 4], &sboxes2[1 * 4], &sboxes2[2 * 4], &sboxes2[3 * 4],
					key2[0], key2[1], key2[2], key2[3]);
			}
			else
			{
				dec[a] = rom[a];
			}
		}
	}
}

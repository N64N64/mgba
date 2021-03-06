/* This code is mostly from ctrulib, which contains the following license:

	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	* The origin of this software must not be misrepresented; you must not
	  claim that you wrote the original software. If you use this software in
	  a product, an acknowledgment in the product documentation would be
	  appreciated but is not required.

	* Altered source versions must be plainly marked as such, and must not be
	  misrepresented as being the original software.

	* This notice may not be removed or altered from any source distribution.
*/

#include <3ds/types.h>
#include <3ds/svc.h>

#include "util/common.h"

extern char* fake_heap_start;
extern char* fake_heap_end;
extern u32 __ctru_linear_heap;
extern u32 __ctru_heap;
extern u32 __ctru_heap_size;
extern u32 __ctru_linear_heap_size;
static u32 __custom_heap_size = 0x02400000;
static u32 __custom_linear_heap_size = 0x01400000;

uint32_t* romBuffer;
size_t romBufferSize;

bool allocateRomBuffer(void) {
	/*
	romBuffer = malloc(0x02000000);
	if (romBuffer) {
		romBufferSize = 0x02000000;
		return true;
	}
	*/
	romBuffer = malloc(0x01000000);
	if (romBuffer) {
		romBufferSize = 0x01000000;
		return true;
	}
	return false;
}

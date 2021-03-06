/*
 * Copyright (c) 2018 p-sam
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <malloc.h>

#include <switch.h>
#include <stratosphere.hpp>

#ifdef HAVE_NSVM_SAFE
#include "nsvm_mitm_service.hpp"
#endif

#include "file_utils.hpp"

extern "C" {
	extern u32 __start__;

	u32 __nx_applet_type = AppletType_None;

	#define INNER_HEAP_SIZE 0x20000
	size_t nx_inner_heap_size = INNER_HEAP_SIZE;
	char   nx_inner_heap[INNER_HEAP_SIZE];
	
	void __libnx_initheap(void);
	void __appInit(void);
	void __appExit(void);
}


void __libnx_initheap(void) {
	void*  addr = nx_inner_heap;
	size_t size = nx_inner_heap_size;

	/* Newlib */
	extern char* fake_heap_start;
	extern char* fake_heap_end;

	fake_heap_start = (char*)addr;
	fake_heap_end   = (char*)addr + size;
}

void __appInit(void) {
	Result rc;
	
	rc = smInitialize();
	if (R_FAILED(rc)) {
		fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));
	}

#ifdef HAVE_NSVM_SAFE
	rc = nsvmInitialize();
	if (R_FAILED(rc)) {
		fatalSimple(rc);
	}
#endif
}

void __appExit(void) {
	smExit();
#ifdef HAVE_NSVM_SAFE
	nsvmExit();
#endif
}

struct MitmManagerOptions {
	static const size_t PointerBufferSize = 0x100;
	static const size_t MaxDomains = 4;
	static const size_t MaxDomainObjects = 0x100;
};

using MitmManager = WaitableManager<MitmManagerOptions>;

int main(int argc, char **argv)
{
	consoleDebugInit(debugDevice_SVC);
	FileUtils::InitializeAsync();
		
	/* TODO: What's a good timeout value to use here? */
	auto server_manager = new MitmManager(1);

#ifdef HAVE_NSVM_SAFE
	NsVmMitmService::AddToManager(server_manager);
#endif

	/* Loop forever, servicing our services. */
	server_manager->Process();
	
	delete server_manager;

	return 0;
}


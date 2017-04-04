/* Copyright (c) 2013-2016 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "lr35902.h"

#include "isa-lr35902.h"
#include <third-party/uthash.h>

void LR35902Init(struct LR35902Core* cpu) {
	cpu->master->init(cpu, cpu->master);
	size_t i;
	for (i = 0; i < cpu->numComponents; ++i) {
		if (cpu->components[i] && cpu->components[i]->init) {
			cpu->components[i]->init(cpu, cpu->components[i]);
		}
	}
}

void LR35902Deinit(struct LR35902Core* cpu) {
	if (cpu->master->deinit) {
		cpu->master->deinit(cpu->master);
	}
	size_t i;
	for (i = 0; i < cpu->numComponents; ++i) {
		if (cpu->components[i] && cpu->components[i]->deinit) {
			cpu->components[i]->deinit(cpu->components[i]);
		}
	}
}

void LR35902SetComponents(struct LR35902Core* cpu, struct mCPUComponent* master, int extra, struct mCPUComponent** extras) {
	cpu->master = master;
	cpu->numComponents = extra;
	cpu->components = extras;
}


void LR35902HotplugAttach(struct LR35902Core* cpu, size_t slot) {
	if (slot >= cpu->numComponents) {
		return;
	}
	cpu->components[slot]->init(cpu, cpu->components[slot]);
}

void LR35902HotplugDetach(struct LR35902Core* cpu, size_t slot) {
	if (slot >= cpu->numComponents) {
		return;
	}
	cpu->components[slot]->deinit(cpu->components[slot]);
}

void LR35902Reset(struct LR35902Core* cpu) {
	cpu->af = 0;
	cpu->bc = 0;
	cpu->de = 0;
	cpu->hl = 0;

	cpu->sp = 0;
	cpu->pc = 0;

	cpu->instruction = 0;

	cpu->cycles = 0;
	cpu->nextEvent = 0;
	cpu->executionState = LR35902_CORE_FETCH;
	cpu->halted = 0;

	cpu->irqPending = false;
	cpu->irqh.reset(cpu);
}

void LR35902RaiseIRQ(struct LR35902Core* cpu, uint8_t vector) {
	cpu->irqPending = true;
	cpu->irqVector = vector;
}

static void _LR35902InstructionIRQStall(struct LR35902Core* cpu) {
	cpu->executionState = LR35902_CORE_STALL;
}

static void _LR35902InstructionIRQFinish(struct LR35902Core* cpu) {
	cpu->executionState = LR35902_CORE_OP2;
	cpu->instruction = _LR35902InstructionIRQStall;
}

static void _LR35902InstructionIRQDelay(struct LR35902Core* cpu) {
	cpu->index = cpu->sp + 1;
	cpu->bus = cpu->pc >> 8;
	cpu->executionState = LR35902_CORE_MEMORY_STORE;
	cpu->instruction = _LR35902InstructionIRQFinish;
	cpu->pc = cpu->irqVector;
	cpu->memory.setActiveRegion(cpu, cpu->pc);
}

static void _LR35902InstructionIRQ(struct LR35902Core* cpu) {
	cpu->sp -= 2; /* TODO: Atomic incrementing? */
	cpu->index = cpu->sp;
	cpu->bus = cpu->pc;
	cpu->executionState = LR35902_CORE_MEMORY_STORE;
	cpu->instruction = _LR35902InstructionIRQDelay;
}

static void _LR35902Step(struct LR35902Core* cpu) {
	++cpu->cycles;
	enum LR35902ExecutionState state = cpu->executionState;
	cpu->executionState = LR35902_CORE_IDLE_0;
	switch (state) {
	case LR35902_CORE_FETCH:
		if (cpu->irqPending) {
			cpu->index = cpu->sp;
			cpu->irqPending = false;
			cpu->instruction = _LR35902InstructionIRQ;
			cpu->irqh.setInterrupts(cpu, false);
			break;
		}
		cpu->bus = cpu->memory.cpuLoad8(cpu, cpu->pc);
		cpu->instruction = _lr35902InstructionTable[cpu->bus];
		++cpu->pc;
		break;
	case LR35902_CORE_MEMORY_LOAD:
		cpu->bus = cpu->memory.load8(cpu, cpu->index);
		break;
	case LR35902_CORE_MEMORY_STORE:
		cpu->memory.store8(cpu, cpu->index, cpu->bus);
		break;
	case LR35902_CORE_READ_PC:
		cpu->bus = cpu->memory.cpuLoad8(cpu, cpu->pc);
		++cpu->pc;
		break;
	case LR35902_CORE_STALL:
		cpu->instruction = _lr35902InstructionTable[0]; // NOP
		break;
	default:
		break;
	}
}

void LR35902Tick(struct LR35902Core* cpu) {
	_LR35902Step(cpu);
	if (cpu->cycles + 2 >= cpu->nextEvent) {
		int32_t diff = cpu->nextEvent - cpu->cycles;
		cpu->cycles = cpu->nextEvent;
		cpu->executionState += diff;
		cpu->irqh.processEvents(cpu);
		cpu->cycles += 2 - diff;
	} else {
		cpu->cycles += 2;
	}
	cpu->executionState = LR35902_CORE_FETCH;
	cpu->instruction(cpu);
	++cpu->cycles;
	if (cpu->cycles >= cpu->nextEvent) {
		cpu->irqh.processEvents(cpu);
	}
}

#ifdef _3DS
bool aaas_call_callback(int callback);
#endif

#define LUA_NOREF (-2)
#define hookz_size 0x10000

struct hook_t {
    struct hook_t *next;
#ifdef _3DS
    int callback;
#else
    bool (*callback)();
#endif
};
static bool (*call_callback)(int);
static struct hook_t *hookz[hookz_size];
static bool initted_hookz = false;
static inline void init_hookz()
{
    if(!initted_hookz) {
        for(int i = 0; i < hookz_size; i++) {
            hookz[i] = NULL;
        }
        initted_hookz = true;
    }
}

#include <gb/gb.h>
#include <gb/memory.h>

bool mgba_should_print = false;

static inline bool check_hook_pc(struct LR35902Core *cpu)
{
    init_hookz();

    struct GB *gb = (struct GB *)cpu->master;
    int bank = gb->memory.currentBank;

    bool should_halt = false;
    for(struct hook_t *hook = hookz[cpu->pc]; hook != NULL; hook = hook->next) {
        bool k;
#ifdef _3DS
        k = aaas_call_callback(hook->callback);
#else
        k = hook->callback();
#endif
        should_halt = k || should_halt;
    }
    return should_halt;
}

#ifdef _3DS
void aaas_add_pc_hook(int bank, int pc, int callback)
#else
void PC_HOOK(int bank, int pc, bool (*callback)())
#endif
{
    init_hookz();

    struct hook_t *hook = malloc(sizeof(struct hook_t));
    hook->callback = callback;
    hook->next = hookz[pc];

    hookz[pc] = hook;
}

bool LR35902Run(struct LR35902Core* cpu) {
	bool running = true;
	while (running || cpu->executionState != LR35902_CORE_FETCH) {
		if(check_hook_pc(cpu)) {
            return true;
        }

		_LR35902Step(cpu);
		if (cpu->cycles + 2 >= cpu->nextEvent) {
			int32_t diff = cpu->nextEvent - cpu->cycles;
			cpu->cycles = cpu->nextEvent;
			cpu->executionState += diff;
			cpu->irqh.processEvents(cpu);
			cpu->cycles += 2 - diff;
			running = false;
		} else {
			cpu->cycles += 2;
		}

		cpu->executionState = LR35902_CORE_FETCH;
		cpu->instruction(cpu);
		++cpu->cycles;
		if (cpu->cycles >= cpu->nextEvent) {
			cpu->irqh.processEvents(cpu);
			running = false;
		}
	}
    return false;
}

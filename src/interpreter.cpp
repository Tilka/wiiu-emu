#include <sstream>
#include "disassembler.h"
#include "interpreter.h"
#include "instructiondata.h"
#include "memory.h"
#include "processor.h"
#include "trace.h"
#include "log.h"
#include "debugcontrol.h"
#include "statedbg.h"

Interpreter
gInterpreter;

static std::vector<instrfptr_t>
sInstructionMap;

// Address used to signify a return to emulator-land.
const uint32_t CALLBACK_ADDR = 0xFBADCDE0;

void Interpreter::RegisterFunctions()
{
   static bool didInit = false;

   if (!didInit) {
      // Reserve instruction map
      sInstructionMap.resize(static_cast<size_t>(InstructionID::InstructionCount), nullptr);

      // Register instruction handlers
      registerBranchInstructions();
      registerConditionInstructions();
      registerFloatInstructions();
      registerIntegerInstructions();
      registerLoadStoreInstructions();
      registerPairedInstructions();
      registerSystemInstructions();

      didInit = true;
   }
}

void Interpreter::registerInstruction(InstructionID id, instrfptr_t fptr)
{
   sInstructionMap[static_cast<size_t>(id)] = fptr;
}

bool Interpreter::hasInstruction(InstructionID id)
{
   return sInstructionMap[static_cast<size_t>(id)] != nullptr;
}

void Interpreter::setJitMode(InterpJitMode val) {
   mJitMode = val;
}

void
Interpreter::execute(ThreadState *state)
{
   bool hasJumped = false;
   bool forceJit = false;

   while (state->nia != CALLBACK_ADDR) {
      // TankTankTank decryptor fn
      //forceJit = state->nia >= 0x0250B648 && state->nia < 0x0250B8B8;

      // Handle interrupts
      gProcessor.handleInterrupt();

      // JIT Attempt!
      if (forceJit || mJitMode == InterpJitMode::Enabled) {
         if (forceJit || state->nia != state->cia + 4) {
            // We jumped, try to enter JIT
            JitCode jitFn = gJitManager.get(state->nia);
            if (jitFn) {
               auto newNia = gJitManager.execute(state, jitFn);
               state->cia = 0;
               state->nia = newNia;
               continue;
            }
         }
      }

      // Interpreter Loop!
      state->cia = state->nia;
      state->nia = state->cia + 4;

      gDebugControl.maybeBreak(state->cia, state, gProcessor.getCoreID());

      auto instr = gMemory.read<Instruction>(state->cia);
      auto data = gInstructionTable.decode(instr);

      if (!data) {
         gLog->error("Could not decode instruction at {:08x} = {:08x}", state->cia, instr.value);
      }
      assert(data);

      auto trace = traceInstructionStart(instr, data, state);
      auto fptr = sInstructionMap[static_cast<size_t>(data->id)];

      if (!fptr) {
         gLog->error("Unimplemented interpreter instruction {}", data->name);
      }
      assert(fptr);

      if (mJitMode != InterpJitMode::Debug) {
         fptr(state, instr);
      } else {
         // Save original state for debugging
         ThreadState ostate = *state;

         // Save thread-state for JIT run.
         ThreadState jstate = *state;
         uint32_t jReserveBytes = 0;
         uint32_t jReserveAddress = 0;
         if (jstate.reserve) {
            jReserveAddress = jstate.reserveAddress;
            jReserveBytes = gMemory.read<uint32_t>(jReserveAddress);
         }

         // Fetch the JIT instruction block
         JitCode jitInstr = gJitManager.getSingle(ostate.cia);
         if (jitInstr == nullptr) {
         	gLog->error("Failed to JIT debug instr {} @ {:08x}", data->name, ostate.cia);
            DebugBreak();
         }

         // Execute with Interpreter
         fptr(state, instr);

         // Kernel calls are not stateless
         if (data->id != InstructionID::kc) {
            // Restore reserve data
            if (jReserveAddress != 0) {
               gMemory.write(jReserveAddress, jReserveBytes);
            }

            // Execute with JIT
            jstate.nia = gJitManager.execute(&jstate, jitInstr);

            // Ensure compliance!
            std::vector<std::string> errors;
            if (!dbgStateCmp(&jstate, state, errors)) {
               gLog->error("JIT Compliance errors w/ {} at {:08x}", data->name, ostate.cia);
               for (auto &err : errors) {
                  gLog->error(err);
               }
               DebugBreak();
            }
         }
      }

      traceInstructionEnd(trace, instr, data, state);
   }
}

void
Interpreter::executeSub(ThreadState *state)
{
   auto lr = state->lr;
   state->lr = CALLBACK_ADDR;
   
   execute(state);

   state->lr = lr;
}

bool jit_fallback(PPCEmuAssembler& a, Instruction instr)
{
   auto data = gInstructionTable.decode(instr);
   auto fptr = sInstructionMap[static_cast<size_t>(data->id)];
   if (!fptr) {
      assert(0);
   }

   //printf("JIT Fallback for `%s`\n", data->name);

   a.mov(a.zcx, a.state);
   a.mov(a.edx, (uint32_t)instr);
   a.call(asmjit::Ptr(fptr));
   
   return true;
}

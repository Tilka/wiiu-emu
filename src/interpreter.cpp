#include "disassembler.h"
#include "interpreter.h"
#include "instructiondata.h"
#include "memory.h"
#include "log.h"

Interpreter
gInterpreter;

static std::vector<instrfptr_t>
sInstructionMap;

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

void Interpreter::execute(ThreadState *state, uint32_t addr)
{
   bool mTraceEnabled = false;
   auto saveLR = state->lr;
   auto saveCIA = state->cia;
   auto saveNIA = state->nia;
   state->lr = 0;

   while(state->cia) {
      Disassembly dis;
      auto instr = gMemory.read<Instruction>(state->cia);
      auto data = gInstructionTable.decode(instr);

      if (!data) {
         xLog() << Log::hex(state->cia) << " " << Log::hex(instr.value);
      }

      auto fptr = sInstructionMap[static_cast<size_t>(data->id)];
      auto bpitr = std::find(mBreakpoints.begin(), mBreakpoints.end(), state->cia);

      if (bpitr != mBreakpoints.end()) {
         xLog() << "Hit breakpoint, starting trace!";
         mTraceEnabled = true;
      }

      if (mTraceEnabled) {
         dis.address = state->cia;
         gDisassembler.disassemble(instr, dis);
         xLog() << Log::hex(state->cia) << " " << Log::hex(instr.value) << " " << dis.text << "\t";
      }

      if (!fptr) {
         xLog() << "Unimplemented interpreter instruction!";
      } else {
         fptr(state, instr);
      }

      if (mTraceEnabled) {
         for (auto &field : data->write) {
            switch (field) {
            case Field::rA:
               xLog() << "r" << instr.rA << " = " << Log::hex(state->gpr[instr.rA]);
               break;
            case Field::rD:
               xLog() << "r" << instr.rD << " = " << Log::hex(state->gpr[instr.rD]);
               break;
            case Field::frD:
               xLog() << "fr" << instr.frD << " = " << state->fpr[instr.frD].value;
               break;
               // mtspr
               // crb
               // crf
            }
         }
      }

      state->cia = state->nia;
      state->nia = state->cia + 4;
   }

   state->lr = saveLR;
   state->cia = saveCIA;
   state->nia = saveNIA;
   xLog() << "Finished!";
}

void Interpreter::addBreakpoint(uint32_t addr)
{
   mBreakpoints.push_back(addr);
}
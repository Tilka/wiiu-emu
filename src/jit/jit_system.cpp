#include <cassert>
#include "bitutils.h"
#include "jit.h"
#include "log.h"
#include "system.h"
#include "kernelfunction.h"
#include "usermodule.h"

static SprEncoding
decodeSPR(Instruction instr)
{
   return static_cast<SprEncoding>(((instr.spr << 5) & 0x3E0) | ((instr.spr >> 5) & 0x1F));
}

// Enforce In-Order Execution of I/O
static bool
eieio(PPCEmuAssembler& a, Instruction instr)
{
   return true;
}

// Synchronise
static bool
sync(PPCEmuAssembler& a, Instruction instr)
{
   return true;
}

// Instruction Synchronise
static bool
isync(PPCEmuAssembler& a, Instruction instr)
{
   return true;
}

// Move from Special Purpose Register
static bool
mfspr(PPCEmuAssembler& a, Instruction instr)
{
   auto spr = decodeSPR(instr);
   switch (spr) {
   case SprEncoding::XER:
      a.mov(a.eax, a.ppcxer);
      break;
   case SprEncoding::LR:
      a.mov(a.eax, a.ppclr);
      break;
   case SprEncoding::CTR:
      a.mov(a.eax, a.ppcctr);
      break;
   case SprEncoding::GQR0:
      a.mov(a.eax, a.ppcgpr[0]);
      break;
   case SprEncoding::GQR1:
      a.mov(a.eax, a.ppcgpr[1]);
      break;
   case SprEncoding::GQR2:
      a.mov(a.eax, a.ppcgpr[2]);
      break;
   case SprEncoding::GQR3:
      a.mov(a.eax, a.ppcgpr[3]);
      break;
   case SprEncoding::GQR4:
      a.mov(a.eax, a.ppcgpr[4]);
      break;
   case SprEncoding::GQR5:
      a.mov(a.eax, a.ppcgpr[5]);
      break;
   case SprEncoding::GQR6:
      a.mov(a.eax, a.ppcgpr[6]);
      break;
   case SprEncoding::GQR7:
      a.mov(a.eax, a.ppcgpr[7]);
      break;
   default:
      gLog->error("Invalid mfspr SPR {}", static_cast<uint32_t>(spr));
   }

   a.mov(a.ppcgpr[instr.rD], a.eax);
   return true;
}

// Move to Special Purpose Register
static bool
mtspr(PPCEmuAssembler& a, Instruction instr)
{
   a.mov(a.eax, a.ppcgpr[instr.rD]);

   auto spr = decodeSPR(instr);
   switch (spr) {
   case SprEncoding::XER:
      a.mov(a.ppcxer, a.eax);
      break;
   case SprEncoding::LR:
      a.mov(a.ppclr, a.eax);
      break;
   case SprEncoding::CTR:
      a.mov(a.ppcctr, a.eax);
      break;
   case SprEncoding::GQR0:
      a.mov(a.ppcgqr[0], a.eax);
      break;
   case SprEncoding::GQR1:
      a.mov(a.ppcgqr[1], a.eax);
      break;
   case SprEncoding::GQR2:
      a.mov(a.ppcgqr[2], a.eax);
      break;
   case SprEncoding::GQR3:
      a.mov(a.ppcgqr[3], a.eax);
      break;
   case SprEncoding::GQR4:
      a.mov(a.ppcgqr[4], a.eax);
      break;
   case SprEncoding::GQR5:
      a.mov(a.ppcgqr[5], a.eax);
      break;
   case SprEncoding::GQR6:
      a.mov(a.ppcgqr[6], a.eax);
      break;
   case SprEncoding::GQR7:
      a.mov(a.ppcgqr[7], a.eax);
      break;
   default:
      gLog->error("Invalid mtspr SPR {}", static_cast<uint32_t>(spr));
   }

   return true;
}

static void
kcstub(ThreadState *state, KernelFunction *func)
{
   func->call(state);
}

// Kernel call
static bool
kc(PPCEmuAssembler& a, Instruction instr)
{
   auto id = instr.kcn;
   auto implemented = instr.kci;

   auto sym = gSystem.getSyscall(id);
   if (!implemented) {
      gLog->debug("unimplemented kernel function {}", sym->name);

      a.int3();
      return true;
   }

   a.mov(a.zcx, a.state);
   a.mov(a.zdx, asmjit::Ptr(sym));
   a.call(asmjit::Ptr(kcstub));
   return true;
}

void
JitManager::registerSystemInstructions()
{
   RegisterInstructionFallback(dcbf);
   RegisterInstructionFallback(dcbi);
   RegisterInstructionFallback(dcbst);
   RegisterInstructionFallback(dcbt);
   RegisterInstructionFallback(dcbtst);
   RegisterInstructionFallback(dcbz);
   RegisterInstructionFallback(dcbz_l);
   RegisterInstruction(eieio);
   RegisterInstruction(isync);
   RegisterInstruction(sync);
   RegisterInstruction(mfspr);
   RegisterInstruction(mtspr);
   RegisterInstructionFallback(mftb);
   RegisterInstructionFallback(mfmsr);
   RegisterInstructionFallback(mtmsr);
   RegisterInstructionFallback(mfsr);
   RegisterInstructionFallback(mfsrin);
   RegisterInstructionFallback(mtsr);
   RegisterInstructionFallback(mtsrin);
   RegisterInstruction(kc);
}

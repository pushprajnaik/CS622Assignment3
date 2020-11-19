#include <stdio.h>
#include "pin.H"

FILE * trace;
PIN_LOCK pinLock;
UINT32 globalcount =0;

unsigned long long int machineaccess;
// Print a memory  record
VOID RecordMem(VOID * ip, VOID * addr, THREADID tid, UINT32 size, UINT32 rw)
{
    unsigned long long int address = (unsigned long long int)addr;
    unsigned long long int boundary = address | 0x3F;

    PIN_GetLock(&pinLock, tid+1);
    
    while(size)
    {
        if(size >= 8 && (address + 7) <= boundary)
        {
            fwrite(&tid,sizeof(int),1,trace);
            fwrite(&address,sizeof(unsigned long long int),1,trace);
            fwrite(&rw,sizeof(int),1,trace);
            fwrite(&globalcount,sizeof(int),1,trace);
            machineaccess++;
            globalcount++;
            address += 8;
            size -= 8;
        }else if(size >= 4 && (address + 3) <= boundary)
        {
            fwrite(&tid,sizeof(int),1,trace);
            fwrite(&address,sizeof(unsigned long long int),1,trace);
            fwrite(&rw,sizeof(int),1,trace);
            fwrite(&globalcount,sizeof(int),1,trace);
            machineaccess++;
            globalcount++;
            address += 4;
            size -= 4;
        }else if(size >= 2 && (address + 1) <= boundary)
        {
            fwrite(&tid,sizeof(int),1,trace);
            fwrite(&address,sizeof(unsigned long long int),1,trace);
            fwrite(&rw,sizeof(int),1,trace);
            fwrite(&globalcount,sizeof(int),1,trace);
            machineaccess++;
            globalcount++;
            address += 2;
            size -= 2;
        }else if(size >= 1 && address <= boundary)
        {
            fwrite(&tid,sizeof(int),1,trace);
            fwrite(&address,sizeof(unsigned long long int),1,trace);
            fwrite(&rw,sizeof(int),1,trace);
            fwrite(&globalcount,sizeof(int),1,trace);
            machineaccess++;
            globalcount++;
            address += 1;
            size -= 1;
        }else
        {
            boundary += 64;
        }

    }

    fflush(trace);
    PIN_ReleaseLock(&pinLock);
}



// This routine is executed every time a thread is created.
VOID ThreadStart(THREADID tid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    PIN_GetLock(&pinLock, tid+1);
    fprintf(stdout, "thread begin %d\n",tid);
    fflush(stdout);
    PIN_ReleaseLock(&pinLock);
}

// This routine is executed every time a thread is destroyed.
VOID ThreadFini(THREADID tid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
    PIN_GetLock(&pinLock, tid+1);
    fprintf(stdout, "thread end %d code %d\n",tid, code);
    fflush(stdout);
    PIN_ReleaseLock(&pinLock);
}

// Pin calls this function every time a new instruction is encountered
VOID Instruction(INS ins, VOID *v)
{
   
    UINT32 memOperands = INS_MemoryOperandCount(ins);
    UINT32 size = 0;
    UINT32 read = 0, write =1;


   
    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        size = INS_MemoryOperandSize(ins, memOp);
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMem,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp,
                IARG_THREAD_ID,
                IARG_UINT32, size,
                IARG_UINT32, read,                  // pass 0 if read memory operand
                IARG_END);
        }
        // Note that in some architectures a single memory operand can be 
        // both read and written (for instance incl (%eax) on IA-32)
        // In that case we instrument it once for read and once for write.
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMem,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp,
                IARG_THREAD_ID,
                IARG_UINT32, size,
                IARG_UINT32, write,             // pass 1 if write memory operand
                IARG_END);
        }
    }
}

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{
    //fprintf(trace, "#eof\n");
    fprintf(stdout, "Total machine accesses are : %llu\n",machineaccess);
    fclose(trace);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR("This Pintool prints the IPs of every instruction executed\n" 
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char * argv[])
{
    trace = fopen("addrtrace1.out", "wb");
    
    PIN_InitLock(&pinLock);

    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
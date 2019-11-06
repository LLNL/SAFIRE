#include "faultinjection.h"
#include <string.h>
#include <stdio.h>
#include "pin.H"

#include "mt64.h"
#include "utils.h"
#include "instselector.h"
#include <inttypes.h>
//#include <stdatomic.h>

//#define VERBOSE

#define MAX_THREADS 256
// 64B alignment TODO: use sysconf or similar to set padding at runtime
static union { UINT64 v; char pad[64]; } fi_iterator[MAX_THREADS] __attribute__((aligned(64))) = { 0 };
// XXX: instr_iterator is global, *NOT* per thread, static instrumentation
static UINT64 instr_iterator = 0;
int gtid = 0;

// XXX: detach is faulty on Mac, emulate as closely possibly
static bool detach = false;

// FI variables
static int fi_thread = -1;

VOID injectReg(THREADID tid, VOID *ip, UINT64 idx, UINT32 op, REG reg, CONTEXT *ctx)
{
    if((int)tid != fi_thread)
      return;

    if(detach_knob)
        if(detach)
          return;

    fi_iterator[tid].v++;

    if ( fi_iterator[tid].v != fi_index )
        return;

    if(action == DO_RANDOM) {
        UINT32 size_bits = REG_Size(reg)*8;
        fi_bit_flip = genrand64_int64()%size_bits;

        fi_output_fstream.open(injection_file.Value().c_str(), std::fstream::out);
        ASSERTX(fi_output_fstream.is_open() && "Cannot open injection output file\n");

        fi_output_fstream << "thread=" << tid << ", fi_index=" << fi_iterator[tid].v << ", op=" << op << ", size=" << REG_Size(reg)
            << ", bitflip=" << fi_bit_flip << ", fi_instr_index=" << idx << ", reg=" << REG_StringShort(reg) << ", addr=" << hexstr(ip) << std::endl;

        fi_output_fstream.close();
    }

    /*cerr << "INJECT fi_index=" << fi_iterator[tid].v << ", fi_instr_index=" << idx << ", op=" << op
        << ", reg=" << REG_StringShort(reg) << ", bitflip=" << fi_bit_flip << ", addr=" << hexstr(ip) << std::endl;*/

    UINT32 inject_byte = fi_bit_flip/8;
    UINT32 inject_bit = fi_bit_flip%8;


    PIN_REGISTER val;
    PIN_GetContextRegval(ctx, reg, val.byte);
    //val->byte[inject_byte] = (val->byte[inject_byte] ^ (1U << inject_bit));
    val.byte[inject_byte] = (val.byte[inject_byte] ^ (1U << inject_bit));
    PIN_SetContextRegval(ctx, reg, val.byte);

    if(detach_knob) {
        detach = true;
        PIN_RemoveInstrumentation();
        PIN_Detach();
        fprintf(stderr, "DETACH thread=%d fi_thread %d fi_iterator %" PRIu64 "\n", tid, fi_thread, fi_iterator[tid].v); //ggout
    }
}

// Memory injection is work-in-progress
#if 0
// XXX: CAUTION: Globals for mem injection assume single-threaded execution
// TODO: Use per-thread data instead of globals
VOID *g_ip = 0;
VOID *g_mem_addr = 0;
UINT32 g_size = 0;

// XXX TODO FIXME: add operand number to mem destinations
VOID inject_Mem(VOID *ip) {
    UINT64 fi_iterator_local = atomic_fetch_add(&fi_iterator, 1);
    if(fi_iterator_local != fi_index)
        return;
    // XXX: !!! CAUTION !!! WARNING !!! CALLING PIN_Detach breaks injection that changes
    // registers. It seems Pin does not transfer the changed state after detaching
#if 0 // FAULTY
    if(fi_iterator_local > fi_index) {
        // XXX: One fault per run, thus remove instr. and detach to speedup execution
        // This can be changed to allow multiple errors
        PIN_Detach();
        return;
    }
#endif

    ASSERTX(ip == g_ip && "Analyze ip unequal to inject ip!\n");
    UINT8 *p = (UINT8 *) g_mem_addr;
#ifdef VERBOSE
    LOG("Instruction: " + ptrstr(ip) +", memory " + ptrstr(g_mem_addr) + ", ");
    LOG("hex: ");
    for(UINT32 i = 0; i<g_size; i++)
        LOG(hexstr(*(p + i)) + " ");
    LOG("\n");
#endif

    UINT32 size_bits = g_size*8;

    UINT32 bit_flip = genrand64_int64()%size_bits;
    UINT32 inject_byte = bit_flip/8;
    UINT32 inject_bit = bit_flip%8;

    fi_output_fstream << "fi_index=" << fi_iterator_local << ", mem_addr=" << hexstr(g_mem_addr)
        << ", bitflip=" << bit_flip << ", addr=" << hexstr(ip) << std::endl;

    *(p + inject_byte) = (*(p + inject_byte) ^ (1U << inject_bit));

#ifdef VERBOSE
    LOG(" bitflip:" + decstr(bit_flip) +" ");
    LOG("hex: ");
    for(UINT32 i = 0; i<g_size; i++)
        LOG(hexstr(*(p + i)) + " ");
    LOG("\n");
#endif

}

VOID analyze_Mem(VOID *ip, VOID *mem_addr, UINT32 size) {
    g_ip = ip;
    g_mem_addr = mem_addr;
    g_size = size;
}
#endif

VOID InstrumentIns(INS ins, VOID *v) {
    instr_iterator++;

    struct {
        enum OP_TYPE type;
        REG reg;
    } FI_Ops[MAX_OPS];
    int numOps = 0;

    REG reg;

#ifdef VERBOSE
    LOG("==============\n");
    LOG("Checking: " + hexstr(INS_Address(ins)) + " \"" + INS_Disassemble(ins) + "\", ");
#endif
#ifdef FI_SRC_REG
    int numR = INS_MaxNumRRegs(ins);
    for(int i = 0; i<numR; i++) {
        if(!REGx_IsInstrPtr(INS_RegR(ins, i))) {
            FI_Ops[numOps].type = REG_SRC;
            FI_Ops[numOps].reg = INS_RegR(ins, i);
#ifdef VERBOSE
            LOG("regR " + decstr(i) + ": " + REG_StringShort(FI_Ops[numOps].reg) + ", size " + decstr(REG_Size(FI_Ops[numOps].reg)));
#endif
            numOps++;
        }
    }
#endif
#ifdef FI_DST_REG
    int numW = INS_MaxNumWRegs(ins);
    for(int i = 0; i<numW; i++) {
        // Skip IP dst: cannot modify directly IP
        // TODO: Instead, modifying IP could be achieved by using a JMP instruction
        if(!REGx_IsInstrPtr(INS_RegW(ins, i))) {
            FI_Ops[numOps].type = REG_DST;
            FI_Ops[numOps].reg = INS_RegW(ins, i);
#ifdef VERBOSE
            LOG("regW " + decstr(i) + ": " + REG_StringShort(FI_Ops[numOps].reg) + ", size " + decstr(REG_Size(FI_Ops[numOps].reg)));
#endif
            numOps++;
        }
    }
#endif
#ifdef FI_DST_MEM
    // TODO: Think about scatter/gather operations, ignore for now
    if(INS_IsMemoryWrite(ins) && INS_hasKnownMemorySize(ins)) {
        FI_Ops[numOps].type = MEM_DST;
        numOps++;
    }
#endif
#ifdef VERBOSE
    LOG("\n");
#endif

    ASSERTX((numOps > 0) && "No ops to FI!\n");

    UINT32 op;
    if(action == DO_RANDOM)
        op = genrand64_int64()%numOps;
    else /* DO_REPRODUCE */ {
        //cerr << "instr_iterator: " << instr_iterator <<" , fi_instr_index: " << fi_instr_index << "\n"; //ggout
        if(instr_iterator == fi_instr_index) {
            op = fi_op;
            //cerr << "setting op=" << op << "\n"; //ggout
        }
        else
            op = 0; // 0 is always valid
    }

    if(FI_Ops[op].type == REG_DST || FI_Ops[op].type == REG_SRC) {
        reg = FI_Ops[op].reg;
#ifdef VERBOSE
        LOG("FI instrumentation at: " + REG_StringShort(reg) + ", " + (FI_Ops[op].type == REG_SRC?"SRC":"DST") + "\n");
#endif

        IPOINT IPoint;
        if(FI_Ops[op].type == REG_SRC || INSx_MayChangeControlFlow(ins))
            IPoint = IPOINT_BEFORE;
        else
            IPoint = IPOINT_AFTER;

        ASSERTX(IPoint == IPOINT_AFTER && "It should always be after!\n");
        // XXX: Use IARG_PARTIAL_CONTEXT instead of IARG_REG_REFERENCE because
        // some registers are unavailable from the latter
        REGSET regsIn, regsOut;
        REGSET_Clear(regsIn);
        REGSET_Clear(regsOut);
        REGSET_Insert(regsIn, reg);
        REGSET_Insert(regsOut, reg);
        // XXX: Adding super registers as well, otherwise Pin may complain
        REGSET_Insert(regsIn, REG_FullRegName(reg) );
        REGSET_Insert(regsOut, REG_FullRegName(reg) );
        INS_InsertCall(ins, IPoint, AFUNPTR(injectReg),
                IARG_THREAD_ID,
                IARG_ADDRINT, INS_Address(ins),
                IARG_UINT64, instr_iterator,
                IARG_UINT32, op,
                IARG_UINT32, reg,
                IARG_PARTIAL_CONTEXT, &regsIn, &regsOut,
                IARG_END);
    }
    // Inject to MEM
#if 0 // Memory injection is work-in-progress
    // XXX: Address can be resolved only at IPOINT_BEFORE. Split fault injection: 1) analyze to
    // get the address and size, and 2) injecton happens at the next instruction (if possible)
    else if(FI_Ops[op].type == MEM_DST) {
        ASSERTX(false && "should never be here\n");
        // XXX: CALL instructions modify the stack: PUSH(IP). However they are considered non-fall 
        // through. Hence, they have no next instruction and FI is not possible
        if(INS_Next(ins) != INS_Invalid()) {
            INS_InsertPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(analyze_Mem),
                    IARG_ADDRINT, INS_Address(ins),
                    IARG_MEMORYWRITE_EA,
                    IARG_MEMORYWRITE_SIZE,
                    IARG_END);

#ifdef VERBOSE
            LOG("FI instrumentation at MEM_DST\n");
#endif
            INS_InsertPredicatedCall(INS_Next(ins), IPOINT_BEFORE, AFUNPTR(inject_Mem),
                    IARG_ADDRINT, INS_Address(ins),
                    IARG_END);
        }

    }
#endif
    else
        ASSERTX(false && "FI type is invalid!\n");

#ifdef VERBOSE
    LOG("==============\n");
#endif

}

VOID InstrumentTrace(TRACE trace, VOID *v)
{
    // XXX: emulate detach
    /*if(detach)
        return;*/

    if(isValidTrace(trace))
        for( BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl) ) {
            // Instrument an Instruction
            for(INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins) )
                if(isValidInst(ins))
                    InstrumentIns(ins, v);
        }
}

VOID ThreadStart(THREADID tid, CONTEXT *ctx, INT32 flags, VOID *v)
{
  //cerr << "PIN tid: " << tid << "\n"; //ggout
  __atomic_add_fetch(&gtid, 1, __ATOMIC_SEQ_CST);
}

VOID Init()
{
    FILE *fp;
    /*if( ( fp = fopen(injection_file.Value().c_str(), "r") ) != NULL) {
        cerr << "REPRODUCE FI" << endl;
        ASSERTX(false && "Reproducing experiments is work-in-progress for parallel programs\n");

        //int ret = fscanf(fp, "fi_index=%"PRIu64", fi_instr_index=%"PRIu64", op=%u, reg=%*[a-z0-9], bitflip=%u, addr=%*x\n", &fi_index, &fi_instr_index, &fi_op, &fi_bit_flip);
        int ret = fscanf(fp, "fi_index=%"PRIu64", op=%u, size=%*u, bitflip=%u, fi_instr_index=%"PRIu64, &fi_index, &fi_op, &fi_bit_flip, &fi_instr_index);
        fprintf(stderr, "fi_index=%"PRIu64", fi_instr_index=%"PRIu64", op=%u, bitflip=%u\n", fi_index, fi_instr_index, fi_op, fi_bit_flip);
        ASSERTX(ret == 4 && "fscanf failed!\n");
        action = DO_REPRODUCE;
        ASSERTX(fi_index > 0 && "fi_index <= 0!\n");
        ASSERTX(fi_instr_index > 0 && "fi_instr_index <= 0!\n");
    }
    else */if( (fp = fopen(target_file.Value().c_str(), "r") ) != NULL) {
        int ret = fscanf(fp, "thread=%d, fi_index=%"PRIu64"\n", &fi_thread, &fi_index);
        //cerr << "TARGET fi_thread=" << fi_thread << ", fi_index=" << fi_index <<" RANDOM INJECTION" << endl;
        ASSERTX(ret == 2 && "fscanf failed!\n");
        action = DO_RANDOM;
        ASSERTX(fi_thread >= 0 && "fi_thread < 0!\n");
        ASSERTX(fi_index > 0 && "fi_index <= 0!\n");
    }
    else {
        ASSERTX(false && "PINFI needs either a target or injection file\n");
    }

    fclose(fp);

    // init ranodm
    uint64_t seed;
    FILE* urandom = fopen("/dev/urandom", "r");
    fread(&seed, sizeof(seed), 1, urandom);
    fclose(urandom);
    init_genrand64(seed);
}

VOID Fini(INT32 code, VOID *v)
{
    /*UINT64 sum = 0;
    for(int i = 0; i < gtid; i++) {
        cerr << "thread=" << i << ", fi_index=" << fi_iterator[i].v << endl;
        sum += fi_iterator[i].v;
    }
    cerr << "fi_index=" << sum << endl;*/

}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR( "This Pintool does fault injection\n"
            + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

int main(int argc, char *argv[])
{
    PIN_InitSymbols();

    if (PIN_Init(argc, argv)) return Usage();

    //configInstSelector();

    Init();

    PIN_AddThreadStartFunction(ThreadStart, 0);
    TRACE_AddInstrumentFunction(InstrumentTrace, 0);

    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}


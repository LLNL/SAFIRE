#include "faultinjection.h"
#include <string.h>
#include <assert.h>
#include "pin.H"

#include "mt64.h"
#include "utils.h"
#include "instselector.h"
#include <inttypes.h>

static UINT64 fi_iterator = 0;
static UINT64 instr_iterator = 0;

//MPI
static int rank = -1;
static int fi_rank = -1;

#define VERBOSE

VOID inject_Reg(VOID *ip, UINT64 idx, UINT32 op, REG reg, PIN_REGISTER *val) {
    fi_iterator++;

    if(fi_iterator != fi_index)
        return;

    //assert(rank == fi_rank && "Pin should execute only on the target rank!\n"); // ggout ggin: avoid pin?
    if(rank != fi_rank)
        return;

    // XXX: !!! CAUTION !!! WARNING !!! CALLING PIN_Detach breaks injection that changes
    // registers. It seems Pin does not transfer the changed state after detaching
#if 0 // FAULTY
    if(fi_iterator > fi_index) {
        // XXX: One fault per run, thus remove instr. and detach to speedup execution
        // This can be changed to allow multiple errors
        PIN_RemoveInstrumentation();
        PIN_Detach();
        return;
    }
#endif

    if(action == DO_RANDOM) {
        UINT32 size_bits = REG_Size(reg)*8;
        fi_bit_flip = genrand64_int64()%size_bits;

        fi_output_fstream.open(injection_file.Value().c_str(), std::fstream::out);
        assert(fi_output_fstream.is_open() && "Cannot open injection output file\n");
        fi_output_fstream << "rank=" << rank << ", fi_index=" << fi_iterator << ", fi_instr_index=" << idx << ", op=" << op << ", reg=" << REG_StringShort(reg)
            << ", bitflip=" << fi_bit_flip << ", addr=" << hexstr(ip) << std::endl;
        fi_output_fstream.close();
    }

    cerr << "INJECT rank=" << rank << ", fi_index=" << fi_iterator << ", fi_instr_index=" << idx << ", op=" << op << ", reg=" << REG_StringShort(reg)
        << ", bitflip=" << fi_bit_flip << ", addr=" << hexstr(ip) << std::endl;

    UINT32 inject_byte = fi_bit_flip/8;
    UINT32 inject_bit = fi_bit_flip%8;

#ifdef VERBOSE
    LOG("INJECT Instruction:" + ptrstr(ip) +", reg " + REG_StringShort(reg) + ", val ");
    for(UINT32 i = 0; i<REG_Size(reg); i++)
        LOG(hexstr(val->byte[i]) + " ");
#endif

    val->byte[inject_byte] = (val->byte[inject_byte] ^ (1U << inject_bit));

#ifdef VERBOSE
    LOG(" bitflip:" + decstr(fi_bit_flip) +", reg " + REG_StringShort(reg) + ", val ");
    for(UINT32 i = 0; i<REG_Size(reg); i++)
        LOG(hexstr(val->byte[i]) + " ");
    LOG("\n");
#endif

}

// XXX: CAUTION: Globals for mem injection assume single-threaded execution
// TODO: Use per-thread data instead of globals
VOID *g_ip = 0;
VOID *g_mem_addr = 0;
UINT32 g_size = 0;

// XXX TODO FIXME: 1) add operand number to mem destinations, 2) reproducible injection (fi_index, op and bit_flip)
VOID inject_Mem(VOID *ip) {
    fi_iterator++;
    if(fi_iterator != fi_index)
        return;

    // XXX: !!! CAUTION !!! WARNING !!! CALLING PIN_Detach breaks injection that changes
    // registers. It seems Pin does not transfer the changed state after detaching
#if 0 // FAULTY
    if(fi_iterator > fi_index) {
        // XXX: One fault per run, thus remove instr. and detach to speedup execution
        // This can be changed to allow multiple errors
        PIN_RemoveInstrumentation();
        PIN_Detach();
        return;
    }
#endif

    assert(ip == g_ip && "Analyze ip unequal to inject ip!\n");
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

    fi_output_fstream.open(injection_file.Value().c_str(), std::fstream::out);
    assert(fi_output_fstream.is_open() && "Cannot open injection output file\n");
    fi_output_fstream << "fi_index=" << fi_iterator << ", mem_addr=" << hexstr(g_mem_addr)
        << ", bitflip=" << bit_flip << ", addr=" << hexstr(ip) << std::endl;
    fi_output_fstream.close();

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

VOID instruction_Instrumentation(INS ins, VOID *v) {
    // decides where to insert the injection calls and what calls to inject
    if (!isValidInst(ins))
        return;

    //assert(rank != fi_rank && "Pin should be called on the fi_rank!\n"); //ggout ggin: avoid Pin if possible?
    if(rank != fi_rank)
        return;

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

    assert((numOps > 0) && "No ops to FI!\n");

    IARG_TYPE ArgType = IARG_REG_REFERENCE;

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

        INS_InsertPredicatedCall(ins, IPoint, AFUNPTR(inject_Reg),
                IARG_ADDRINT, INS_Address(ins),
                IARG_UINT64, instr_iterator,
                IARG_UINT32, op,
                IARG_UINT32, reg,
                ArgType , reg,
                IARG_END);

    }
    // Inject to MEM
    // XXX: Address can be resolved only at IPOINT_BEFORE. Split fault injection: 1) analyze to
    // get the address and size, and 2) injecton happens at the next instruction (if possible)
    else if(FI_Ops[op].type == MEM_DST) {
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
    else
        assert(false && "FI type is invalid!\n");

#ifdef VERBOSE
    LOG("==============\n");
#endif
}

VOID get_instance_number()
{
    // TODO: Fix instruction type selection
    /*if(!(fioption.Value() == CCS_INST || fioption.Value() == FP_INST || fioption.Value() == SP_INST || fioption.Value() == ALL_INST)){
      fprintf(stderr, "ERROR, Specify one of valid options\n");
      exit(1);
      }*/

    FILE *fp;
    if( ( fp = fopen(injection_file.Value().c_str(), "r") ) != NULL) {
        cerr << "REPRODUCE FI" << endl;

        int ret = fscanf(fp, "rank=%d, fi_index=%"PRIu64", fi_instr_index=%"PRIu64", op=%u, reg=%*[a-z0-9], bitflip=%u, addr=%*x\n", &fi_rank, &fi_index, &fi_instr_index, &fi_op, &fi_bit_flip);
        fprintf(stderr, "rank=%d, fi_index=%"PRIu64", fi_instr_index=%"PRIu64", op=%u, bitflip=%u\n", fi_rank, fi_index, fi_instr_index, fi_op, fi_bit_flip);
        assert(ret == 5 && "fscanf failed!\n");
        action = DO_REPRODUCE;
        assert(rank >= 0 && "rank < 0!\n");
        assert(fi_index > 0 && "fi_index <= 0!\n");
        assert(fi_instr_index > 0 && "fi_instr_index <= 0!\n");
        //assert(false && "early abort");
    }
    else if( (fp = fopen(target_file.Value().c_str(), "r") ) != NULL) {
        int ret = fscanf(fp, "rank=%d, fi_index=%"PRIu64"\n", &fi_rank, &fi_index);
        assert(ret == 2 && "fscanf failed!\n");
        cerr << "TARGET rank=" << fi_rank << ", fi_index=" << fi_index <<" RANDOM INJECTION" << endl;
        action = DO_RANDOM;
        //assert(false && "early abort");
    }
    else {
        assert(false && "PINFI needs either a target or injection file\n");
    }

    fclose(fp);

    uint64_t seed;
    FILE* urandom = fopen("/dev/urandom", "r");
    fread(&seed, sizeof(seed), 1, urandom);
    fclose(urandom);
    init_genrand64(seed);
}

VOID Fini(INT32 code, VOID *v)
{
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

    configInstSelector();

    char *rank_str = getenv("OMPI_COMM_WORLD_RANK");
    assert(rank_str != NULL && "rank_str = NULL, cannot read env variable for MPI rank\n");
    rank = atoi(rank_str);
    cout << "rank: " << rank << endl; //ggout

    get_instance_number();

    INS_AddInstrumentFunction(instruction_Instrumentation, 0);

    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}


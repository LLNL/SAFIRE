#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include <execinfo.h>
#include <string.h>
#include <omp.h>
#include <pthread.h>
#include <dlfcn.h>
#include <stdatomic.h>
#include "mt64.h"

// XXX: No need to __attribute__((preserve_all )) caller site saves needed registers
void selInst(uint64_t *, uint8_t *);
void selMBB(uint64_t *, uint64_t);
void doInject(unsigned , uint64_t *, uint64_t *, uint8_t *);

void init() __attribute__((constructor));
void fini() __attribute__((destructor));

// per-thread variables
#define MAX_THREADS 256
static union { uint64_t v; char pad[64]; } fi_iterator[MAX_THREADS]  __attribute__((aligned(64)))= { { 0 }  };
__thread int tid = -1;
atomic_int gtid = 0;

// FI
static enum {
    DO_PROFILING,
    DO_REPRODUCTION,
    DO_RANDOM
} action;

enum {
    INSTRUMENT_BB=0,
    INSTRUMENT_INST=1,
    INSTRUMENT_DETACH=2
};

int fi_thread = -1;
uint64_t fi_index = 0;
uint64_t op_num = 0;
uint64_t op_size = 0;
unsigned bit_pos = 0;

char inscount_fname[64];
const char *target_fname = "fi-target.txt";
const char *inject_fname = "fi-inject.txt";
FILE *ins_fp, *tgt_fp, *inj_fp;

void selMBB(uint64_t *ret, uint64_t num_insts)
{
    *ret = INSTRUMENT_BB;

    if(tid < 0) {
        tid = atomic_fetch_add(&gtid, 1);
    }

    if(fi_index > 0){
        if(fi_thread != tid || fi_index <= fi_iterator[tid].v) {
            *ret = INSTRUMENT_DETACH;
            //printf("DETACH thread %d fi_index %"PRIu64" fi_iterator %"PRIu64"\n", tid, fi_index, fi_iterator[tid].v);
        }
        else if(  ( tid == fi_thread ) && ( fi_iterator[tid].v < fi_index ) && fi_index <= ( fi_iterator[tid].v + num_insts ) ) {
            *ret = INSTRUMENT_INST;
        }
        else {
            fi_iterator[tid].v += num_insts;
        }
    }
    else {
        fi_iterator[tid].v += num_insts;
    }
}

void selInst(uint64_t *ret, uint8_t *instr_str)
{
    *ret = 0;

    fi_iterator[tid].v++;
    if( ( fi_thread == tid ) && (fi_iterator[tid].v == fi_index) ) {
        *ret = 1;
        //printf("INJECT thread=%d, fi_index=%"PRIu64", ins=%s\n", tid, fi_iterator[tid].v, instr_str);
    }
}

void doInject(unsigned num_ops, uint64_t *op, uint64_t *size, uint8_t *bitmask)
{
    assert( ( ( action == DO_REPRODUCTION ) || ( action == DO_RANDOM ) ) && "action is neither DO_REPRODUCTION nor DO_RANDOM!\n");
    unsigned bitflip;
    // Reproduce FI
    if(action == DO_REPRODUCTION) {
        //printf("DO REPRODUCIBLE INJECTION!\n");
        *op = op_num;
        bitflip = bit_pos;

    }
    // Random operands and bit pos FI
    else if(action == DO_RANDOM) {
        //printf("DO RANDOM INJECTION\n");
        *op = genrand64_int64()%num_ops;
        // XXX: size is in bytes, multiply by 8 for bits
        bitflip = (genrand64_int64()%(8*size[*op]));
        op_num = *op;
        op_size = size[*op];
        bit_pos = bitflip;

        inj_fp = fopen(inject_fname, "w");
        fprintf(inj_fp, "thread=%d, fi_index=%"PRIu64", op=%"PRIu64", size=%"PRIu64", bitflip=%u\n", \
                fi_thread, fi_index, op_num, op_size, bit_pos);
        //TODO: for multiple faults, fflush and fclose at fini
        fclose(inj_fp);
    }

    //printf("op_size %llu size[*op] %llu\n", op_size, size[*op]);
    assert( ( op_size == size[*op] ) && "op_size != size[*op]");

    unsigned i;
    for(i=0; i<op_size; i++)
        bitmask[i] = 0;

    unsigned bit_i = bitflip/8;
    unsigned bit_j = bitflip%8;

    bitmask[bit_i] = (1U << bit_j);

    /*printf("INJECTING FAULT: thread=%d, fi_index=%"PRIu64", op=%"PRIu64", size=%"PRIu64", bitflip=%u\n", \
            fi_thread, fi_index, op_num, op_size, bitflip);*/
    fflush(stdout);
}

void init()
{
    // XXX: First try to reproduce a specific injection, next try to read a target instruction for FI. If netheir holds, do a profiling run
    // This is specific injection, including operands, produced after a FI experiment
    if( ( inj_fp = fopen(inject_fname, "r") ) ) {
        // reproduce injection
        printf("REPRODUCE INJECTION\n");
        fscanf(inj_fp, "thread=%d, fi_index=%"PRIu64", op=%"PRIu64", size=%"PRIu64", bitflip=%u", \
                &fi_thread, &fi_index, &op_num, &op_size, &bit_pos);
        fprintf(stdout, "thread=%d, fi_index=%"PRIu64", op=%"PRIu64", size=%"PRIu64", bitflip=%u\n", \
                fi_thread, fi_index, op_num, op_size, bit_pos);
        assert(fi_thread >= 0 && "fi_thread < 0\n");
        assert(fi_index > 0 && "fi_index <= 0!\n");
        assert(op_num >= 0 && "op_num < 0!\n");
        assert(op_size > 0 && "op_size <=0!\n");
        assert(bit_pos >= 0 && "bit_pos < 0!\n");

        fclose(inj_fp);

        action = DO_REPRODUCTION;
    }
    // This is targeted injection, selecting the instruction to run a random experiment
    else if ( ( tgt_fp = fopen(target_fname, "r") ) ) {
        int ret = fscanf(tgt_fp, "thread=%d, fi_index=%"PRIu64"\n", &fi_thread, &fi_index);
        assert(ret == 2 && "fscanf failed to parse input\n");
        assert(fi_thread >= 0 && "fi_thread < 0\n");
        assert(fi_index > 0 && "fi_index <= 0\n");

        fclose(tgt_fp);

        action = DO_RANDOM;

        // Initialize the random generator
        uint64_t seed;
        FILE *fp = fopen("/dev/urandom", "r");
        assert(fp != NULL && "Error opening /dev/urandom\n");
        fread(&seed, sizeof(seed), 1, fp);
        init_genrand64(seed);
        assert(ferror(fp) == 0 && "Error reading /dev/urandom\n");
        fclose(fp);

        printf("RANDOM INJECTION RUN!\n");

    }
    // This is a profiling run to get the number of target instructions
    else {
        printf("PROFILING RUN\n");
        action = DO_PROFILING;
    }
}

void fini()
{
    // XXX: This is a profiling run
    if(action == DO_PROFILING) {
        //fprintf(stderr, "Count of dynamic FI target instructions\n");
        sprintf(inscount_fname, "%s", "fi-inscount.txt");
        ins_fp = fopen(inscount_fname, "w");
        assert(ins_fp != NULL && "Error opening inscount file\n");
        uint64_t sum = 0;
        int i;
        for(i=0; i < gtid; i++) {
            fprintf(ins_fp, "thread=%d, fi_index=%"PRIu64"\n", i, fi_iterator[i].v);
            //fprintf(stderr, "thread=%d, fi_index=%"PRIu64"\n", i, fi_iterator[i].v);
            sum += fi_iterator[i].v;
        }
        fprintf(ins_fp, "fi_index=%"PRIu64"\n", sum);
        //fprintf(stderr, "sum : %"PRIu64"\n", sum);
        fclose(ins_fp);
    }
}


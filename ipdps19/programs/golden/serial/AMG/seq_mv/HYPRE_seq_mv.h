/*BHEADER**********************************************************************
 * Copyright (c) 2017,  Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * Written by Ulrike Yang (yang11@llnl.gov) et al. CODE-LLNL-738-322.
 * This file is part of AMG.  See files README and COPYRIGHT for details.
 *
 * AMG is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License (as published by the Free
 * Software Foundation) version 2.1 dated February 1999.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTIBILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the
 * GNU General Public License for more details.
 *
 ***********************************************************************EHEADER*/




/******************************************************************************
 *
 * Header file for HYPRE_mv library
 *
 *****************************************************************************/

#ifndef HYPRE_SEQ_MV_HEADER
#define HYPRE_SEQ_MV_HEADER

#include "../utilities/HYPRE_utilities.h"

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------
 * Structures
 *--------------------------------------------------------------------------*/

struct hypre_CSRMatrix_struct;
typedef struct hypre_CSRMatrix_struct *HYPRE_CSRMatrix;
#ifndef HYPRE_VECTOR_STRUCT
#define HYPRE_VECTOR_STRUCT
struct hypre_Vector_struct;
typedef struct hypre_Vector_struct *HYPRE_Vector;
#endif

/*--------------------------------------------------------------------------
 * Prototypes
 *--------------------------------------------------------------------------*/

/* HYPRE_csr_matrix.c */
HYPRE_CSRMatrix HYPRE_CSRMatrixCreate( HYPRE_Int num_rows , HYPRE_Int num_cols , HYPRE_Int *row_sizes );
HYPRE_Int HYPRE_CSRMatrixDestroy( HYPRE_CSRMatrix matrix );
HYPRE_Int HYPRE_CSRMatrixInitialize( HYPRE_CSRMatrix matrix );
HYPRE_CSRMatrix HYPRE_CSRMatrixRead( char *file_name );
void HYPRE_CSRMatrixPrint( HYPRE_CSRMatrix matrix , char *file_name );
HYPRE_Int HYPRE_CSRMatrixGetNumRows( HYPRE_CSRMatrix matrix , HYPRE_Int *num_rows );

/* HYPRE_vector.c */
HYPRE_Vector HYPRE_VectorCreate( HYPRE_Int size );
HYPRE_Int HYPRE_VectorDestroy( HYPRE_Vector vector );
HYPRE_Int HYPRE_VectorInitialize( HYPRE_Vector vector );
HYPRE_Int HYPRE_VectorPrint( HYPRE_Vector vector , char *file_name );
HYPRE_Vector HYPRE_VectorRead( char *file_name );

typedef enum HYPRE_TimerID
{
   // timers for solver phase
   HYPRE_TIMER_ID_MATVEC = 0,
   HYPRE_TIMER_ID_BLAS1,
   HYPRE_TIMER_ID_RELAX,
   HYPRE_TIMER_ID_GS_ELIM_SOLVE,

   // timers for solve MPI
   HYPRE_TIMER_ID_PACK_UNPACK, // copying data to/from send/recv buf
   HYPRE_TIMER_ID_HALO_EXCHANGE, // halo exchange in matvec and relax
   HYPRE_TIMER_ID_ALL_REDUCE,

   // timers for setup phase
   // coarsening
   HYPRE_TIMER_ID_CREATES,
   HYPRE_TIMER_ID_CREATE_2NDS,
   HYPRE_TIMER_ID_PMIS,

   // interpolation
   HYPRE_TIMER_ID_EXTENDED_I_INTERP,
   HYPRE_TIMER_ID_PARTIAL_INTERP,
   HYPRE_TIMER_ID_MULTIPASS_INTERP,
   HYPRE_TIMER_ID_INTERP_TRUNC,
   HYPRE_TIMER_ID_MATMUL, // matrix-matrix multiplication
   HYPRE_TIMER_ID_COARSE_PARAMS,

   // rap
   HYPRE_TIMER_ID_RAP,

   // timers for setup MPI
   HYPRE_TIMER_ID_RENUMBER_COLIDX,
   HYPRE_TIMER_ID_EXCHANGE_INTERP_DATA,

   // setup etc
   HYPRE_TIMER_ID_GS_ELIM_SETUP,

   // temporaries
   HYPRE_TIMER_ID_BEXT_A,
   HYPRE_TIMER_ID_BEXT_S,
   HYPRE_TIMER_ID_RENUMBER_COLIDX_RAP,
   HYPRE_TIMER_ID_MERGE,

   HYPRE_TIMER_ID_COUNT
} HYPRE_TimerID;

extern HYPRE_Real hypre_profile_times[HYPRE_TIMER_ID_COUNT];

#ifdef __cplusplus
}

#endif

#endif

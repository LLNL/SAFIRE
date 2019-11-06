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




/*---------------------------------------------------- 
 * Functions for the IJ assumed partition fir IJ_Matrix
 *-----------------------------------------------------*/

#include "_hypre_IJ_mv.h"

/*------------------------------------------------------------------
 * hypre_IJMatrixCreateAssumedPartition -
 * Each proc gets it own range. Then 
 * each needs to reconcile its actual range with its assumed
 * range - the result is essentila a partition of its assumed range -
 * this is the assumed partition.   
 *--------------------------------------------------------------------*/


HYPRE_Int
hypre_IJMatrixCreateAssumedPartition( hypre_IJMatrix *matrix) 
{


   HYPRE_Int global_num_rows;
   HYPRE_Int global_first_row;
   HYPRE_Int myid;
   HYPRE_Int  row_start = 0, row_end = 0;
   HYPRE_Int *row_partitioning = hypre_IJMatrixRowPartitioning(matrix);

   MPI_Comm   comm;
   
   hypre_IJAssumedPart *apart;

   global_num_rows = hypre_IJMatrixGlobalNumRows(matrix); 
   global_first_row = hypre_IJMatrixGlobalFirstRow(matrix); 
   comm = hypre_IJMatrixComm(matrix);
   
   /* find out my actual range of rows and rowumns */
   row_start = row_partitioning[0];
   row_end = row_partitioning[1]-1;
   hypre_MPI_Comm_rank(comm, &myid );

   /* allocate space */
   apart = hypre_CTAlloc(hypre_IJAssumedPart, 1);

  /* get my assumed partitioning  - we want row partitioning of the matrix
      for off processor values - so we use the row start and end 
     Note that this is different from the assumed partitioning for the parcsr matrix
     which needs it for matvec multiplications and therefore needs to do it for
     the col partitioning */
   hypre_GetAssumedPartitionRowRange( comm, myid, global_first_row, 
			global_num_rows, &(apart->row_start), &(apart->row_end));

  /*allocate some space for the partition of the assumed partition */
    apart->length = 0;
    /*room for 10 owners of the assumed partition*/ 
    apart->storage_length = 10; /*need to be >=1 */ 
    apart->proc_list = hypre_TAlloc(HYPRE_Int, apart->storage_length);
    apart->row_start_list =   hypre_TAlloc(HYPRE_Int, apart->storage_length);
    apart->row_end_list =   hypre_TAlloc(HYPRE_Int, apart->storage_length);


    /* now we want to reconcile our actual partition with the assumed partition */
    hypre_LocateAssummedPartition(comm, row_start, row_end, global_first_row,
			global_num_rows, apart, myid);

    /* this partition will be saved in the matrix data structure until the matrix is destroyed */
    hypre_IJMatrixAssumedPart(matrix) = apart;
   
    return hypre_error_flag;


}

/*--------------------------------------------------------------------
 * hypre_IJVectorCreateAssumedPartition -

 * Essentially the same as for a matrix!

 * Each proc gets it own range. Then 
 * each needs to reconcile its actual range with its assumed
 * range - the result is essentila a partition of its assumed range -
 * this is the assumed partition.   
 *--------------------------------------------------------------------*/


HYPRE_Int
hypre_IJVectorCreateAssumedPartition( hypre_IJVector *vector) 
{


   HYPRE_Int global_num, global_first_row;
   HYPRE_Int myid;
   HYPRE_Int  start=0, end=0;
   HYPRE_Int  *partitioning = hypre_IJVectorPartitioning(vector);

   MPI_Comm   comm;
   
   hypre_IJAssumedPart *apart;

   global_num = hypre_IJVectorGlobalNumRows(vector); 
   global_first_row = hypre_IJVectorGlobalFirstRow(vector); 
   comm = hypre_ParVectorComm(vector);
   
   /* find out my actualy range of rows */
   start =  partitioning[0];
   end = partitioning[1]-1;
   
   hypre_MPI_Comm_rank(comm, &myid );

   /* allocate space */
   apart = hypre_CTAlloc(hypre_IJAssumedPart, 1);

  /* get my assumed partitioning  - we want partitioning of the vector that the
      matrix multiplies - so we use the col start and end */
   hypre_GetAssumedPartitionRowRange( comm, myid, global_first_row, 
				global_num, &(apart->row_start), &(apart->row_end));

  /*allocate some space for the partition of the assumed partition */
    apart->length = 0;
    /*room for 10 owners of the assumed partition*/ 
    apart->storage_length = 10; /*need to be >=1 */ 
    apart->proc_list = hypre_TAlloc(HYPRE_Int, apart->storage_length);
    apart->row_start_list =   hypre_TAlloc(HYPRE_Int, apart->storage_length);
    apart->row_end_list =   hypre_TAlloc(HYPRE_Int, apart->storage_length);


    /* now we want to reconcile our actual partition with the assumed partition */
    hypre_LocateAssummedPartition(comm, start, end, global_first_row, 
				global_num, apart, myid);

    /* this partition will be saved in the vector data structure until the vector is destroyed */
    hypre_IJVectorAssumedPart(vector) = apart;
   
    return hypre_error_flag;


}


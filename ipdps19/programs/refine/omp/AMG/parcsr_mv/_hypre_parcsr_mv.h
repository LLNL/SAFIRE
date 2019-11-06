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

#include "HYPRE_parcsr_mv.h"

#ifndef hypre_PARCSR_MV_HEADER
#define hypre_PARCSR_MV_HEADER

#include "_hypre_utilities.h"
#include "seq_mv.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifndef HYPRE_PAR_CSR_COMMUNICATION_HEADER
#define HYPRE_PAR_CSR_COMMUNICATION_HEADER

/*--------------------------------------------------------------------------
 * hypre_ParCSRCommPkg:
 *   Structure containing information for doing communications
 *--------------------------------------------------------------------------*/

/*#define HYPRE_USING_PERSISTENT_COMM*/ // JSP: can be defined by configure
#ifdef HYPRE_USING_PERSISTENT_COMM
typedef enum CommPkgJobType
{
   HYPRE_COMM_PKG_JOB_COMPLEX = 0,
   HYPRE_COMM_PKG_JOB_COMPLEX_TRANSPOSE,
   HYPRE_COMM_PKG_JOB_INT,
   HYPRE_COMM_PKG_JOB_INT_TRANSPOSE,
   NUM_OF_COMM_PKG_JOB_TYPE,
} CommPkgJobType;

typedef struct
{
   void     *send_data;
   void     *recv_data;

   HYPRE_Int             num_requests;
   hypre_MPI_Request    *requests;

   HYPRE_Int own_send_data, own_recv_data;

} hypre_ParCSRPersistentCommHandle;
#endif

typedef struct
{
   MPI_Comm               comm;

   HYPRE_Int                    num_sends;
   HYPRE_Int                   *send_procs;
   HYPRE_Int			 *send_map_starts;
   HYPRE_Int			 *send_map_elmts;

   HYPRE_Int                    num_recvs;
   HYPRE_Int                   *recv_procs;
   HYPRE_Int                   *recv_vec_starts;

   /* remote communication information */
   hypre_MPI_Datatype          *send_mpi_types;
   hypre_MPI_Datatype          *recv_mpi_types;

#ifdef HYPRE_USING_PERSISTENT_COMM
   hypre_ParCSRPersistentCommHandle *persistent_comm_handles[NUM_OF_COMM_PKG_JOB_TYPE];
#endif
} hypre_ParCSRCommPkg;

/*--------------------------------------------------------------------------
 * hypre_ParCSRCommHandle:
 *--------------------------------------------------------------------------*/

typedef struct
{
   hypre_ParCSRCommPkg  *comm_pkg;
   void 	  *send_data;
   void 	  *recv_data;

   HYPRE_Int             num_requests;
   hypre_MPI_Request    *requests;

} hypre_ParCSRCommHandle;

/*--------------------------------------------------------------------------
 * Accessor macros: hypre_ParCSRCommPkg
 *--------------------------------------------------------------------------*/
 
#define hypre_ParCSRCommPkgComm(comm_pkg)          (comm_pkg -> comm)
                                               
#define hypre_ParCSRCommPkgNumSends(comm_pkg)      (comm_pkg -> num_sends)
#define hypre_ParCSRCommPkgSendProcs(comm_pkg)     (comm_pkg -> send_procs)
#define hypre_ParCSRCommPkgSendProc(comm_pkg, i)   (comm_pkg -> send_procs[i])
#define hypre_ParCSRCommPkgSendMapStarts(comm_pkg) (comm_pkg -> send_map_starts)
#define hypre_ParCSRCommPkgSendMapStart(comm_pkg,i)(comm_pkg -> send_map_starts[i])
#define hypre_ParCSRCommPkgSendMapElmts(comm_pkg)  (comm_pkg -> send_map_elmts)
#define hypre_ParCSRCommPkgSendMapElmt(comm_pkg,i) (comm_pkg -> send_map_elmts[i])

#define hypre_ParCSRCommPkgNumRecvs(comm_pkg)      (comm_pkg -> num_recvs)
#define hypre_ParCSRCommPkgRecvProcs(comm_pkg)     (comm_pkg -> recv_procs)
#define hypre_ParCSRCommPkgRecvProc(comm_pkg, i)   (comm_pkg -> recv_procs[i])
#define hypre_ParCSRCommPkgRecvVecStarts(comm_pkg) (comm_pkg -> recv_vec_starts)
#define hypre_ParCSRCommPkgRecvVecStart(comm_pkg,i)(comm_pkg -> recv_vec_starts[i])

#define hypre_ParCSRCommPkgSendMPITypes(comm_pkg)  (comm_pkg -> send_mpi_types)
#define hypre_ParCSRCommPkgSendMPIType(comm_pkg,i) (comm_pkg -> send_mpi_types[i])

#define hypre_ParCSRCommPkgRecvMPITypes(comm_pkg)  (comm_pkg -> recv_mpi_types)
#define hypre_ParCSRCommPkgRecvMPIType(comm_pkg,i) (comm_pkg -> recv_mpi_types[i])

/*--------------------------------------------------------------------------
 * Accessor macros: hypre_ParCSRCommHandle
 *--------------------------------------------------------------------------*/
 
#define hypre_ParCSRCommHandleCommPkg(comm_handle)     (comm_handle -> comm_pkg)
#define hypre_ParCSRCommHandleSendData(comm_handle)    (comm_handle -> send_data)
#define hypre_ParCSRCommHandleRecvData(comm_handle)    (comm_handle -> recv_data)
#define hypre_ParCSRCommHandleNumRequests(comm_handle) (comm_handle -> num_requests)
#define hypre_ParCSRCommHandleRequests(comm_handle)    (comm_handle -> requests)
#define hypre_ParCSRCommHandleRequest(comm_handle, i)  (comm_handle -> requests[i])

#endif /* HYPRE_PAR_CSR_COMMUNICATION_HEADER */

#ifndef hypre_PARCSR_ASSUMED_PART
#define  hypre_PARCSR_ASSUMED_PART

typedef struct
{
   HYPRE_Int                   length;
   HYPRE_Int                   row_start;
   HYPRE_Int                   row_end;
   HYPRE_Int                   storage_length;
   HYPRE_Int                   *proc_list;
   HYPRE_Int		         *row_start_list;
   HYPRE_Int                   *row_end_list;  
  HYPRE_Int                    *sort_index;
} hypre_IJAssumedPart;


#endif /* hypre_PARCSR_ASSUMED_PART */


#ifndef hypre_NEW_COMMPKG
#define hypre_NEW_COMMPKG

typedef struct
{
   HYPRE_Int       length;
   HYPRE_Int       storage_length; 
   HYPRE_Int      *id;
   HYPRE_Int      *vec_starts;
   HYPRE_Int       element_storage_length; 
   HYPRE_Int      *elements;
   HYPRE_Real     *d_elements; /* Is this used anywhere? */
   void           *v_elements;
   
}  hypre_ProcListElements;   

#endif /* hypre_NEW_COMMPKG */



/******************************************************************************
 *
 * Header info for Parallel Vector data structure
 *
 *****************************************************************************/

#ifndef hypre_PAR_VECTOR_HEADER
#define hypre_PAR_VECTOR_HEADER


/*--------------------------------------------------------------------------
 * hypre_ParVector
 *--------------------------------------------------------------------------*/

#ifndef HYPRE_PAR_VECTOR_STRUCT
#define HYPRE_PAR_VECTOR_STRUCT
#endif

typedef struct hypre_ParVector_struct
{
   MPI_Comm	 comm;

   HYPRE_Int      	 global_size;
   HYPRE_Int      	 first_index;
   HYPRE_Int             last_index;
   HYPRE_Int      	*partitioning;
   HYPRE_Int      	 actual_local_size; /* stores actual length of data in local vector
			to allow memory manipulations for temporary vectors*/
   hypre_Vector	*local_vector; 

   /* Does the Vector create/destroy `data'? */
   HYPRE_Int      	 owns_data;
   HYPRE_Int      	 owns_partitioning;

   hypre_IJAssumedPart *assumed_partition; /* only populated if no_global_partition option
                                              is used (compile-time option) AND this partition
                                              needed
                                              (for setting off-proc elements, for example)*/


} hypre_ParVector;

/*--------------------------------------------------------------------------
 * Accessor functions for the Vector structure
 *--------------------------------------------------------------------------*/

#define hypre_ParVectorComm(vector)  	        ((vector) -> comm)
#define hypre_ParVectorGlobalSize(vector)       ((vector) -> global_size)
#define hypre_ParVectorFirstIndex(vector)       ((vector) -> first_index)
#define hypre_ParVectorLastIndex(vector)        ((vector) -> last_index)
#define hypre_ParVectorPartitioning(vector)     ((vector) -> partitioning)
#define hypre_ParVectorActualLocalSize(vector)  ((vector) -> actual_local_size)
#define hypre_ParVectorLocalVector(vector)      ((vector) -> local_vector)
#define hypre_ParVectorOwnsData(vector)         ((vector) -> owns_data)
#define hypre_ParVectorOwnsPartitioning(vector) ((vector) -> owns_partitioning)
#define hypre_ParVectorNumVectors(vector)\
 (hypre_VectorNumVectors( hypre_ParVectorLocalVector(vector) ))

#define hypre_ParVectorAssumedPartition(vector) ((vector) -> assumed_partition)


#endif

/******************************************************************************
 *
 * Header info for Parallel CSR Matrix data structures
 *
 * Note: this matrix currently uses 0-based indexing.
 *
 *****************************************************************************/

#ifndef hypre_PAR_CSR_MATRIX_HEADER
#define hypre_PAR_CSR_MATRIX_HEADER

/*--------------------------------------------------------------------------
 * Parallel CSR Matrix
 *--------------------------------------------------------------------------*/

#ifndef HYPRE_PAR_CSR_MATRIX_STRUCT
#define HYPRE_PAR_CSR_MATRIX_STRUCT
#endif

typedef struct hypre_ParCSRMatrix_struct
{
   MPI_Comm              comm;

   HYPRE_Int             global_num_rows;
   HYPRE_Int             global_num_cols;
   HYPRE_Int             first_row_index;
   HYPRE_Int             first_col_diag;
   /* need to know entire local range in case row_starts and col_starts 
      are null  (i.e., bgl) AHB 6/05*/
   HYPRE_Int             last_row_index;
   HYPRE_Int             last_col_diag;

   hypre_CSRMatrix      *diag;
   hypre_CSRMatrix      *offd;
   hypre_CSRMatrix      *diagT, *offdT;
        /* JSP: transposed matrices are created lazily and optional */
   HYPRE_Int            *col_map_offd; 
        /* maps columns of offd to global columns */
   HYPRE_Int            *row_starts; 
        /* array of length num_procs+1, row_starts[i] contains the 
           global number of the first row on proc i,  
           first_row_index = row_starts[my_id],
           row_starts[num_procs] = global_num_rows */
   HYPRE_Int            *col_starts;
        /* array of length num_procs+1, col_starts[i] contains the 
           global number of the first column of diag on proc i,  
           first_col_diag = col_starts[my_id],
           col_starts[num_procs] = global_num_cols */

   hypre_ParCSRCommPkg  *comm_pkg;
   hypre_ParCSRCommPkg  *comm_pkgT;
   
   /* Does the ParCSRMatrix create/destroy `diag', `offd', `col_map_offd'? */
   HYPRE_Int             owns_data;
   /* Does the ParCSRMatrix create/destroy `row_starts', `col_starts'? */
   HYPRE_Int             owns_row_starts;
   HYPRE_Int             owns_col_starts;

   HYPRE_Int             num_nonzeros;
   HYPRE_Real            d_num_nonzeros;

   /* Buffers used by GetRow to hold row currently being accessed. AJC, 4/99 */
   HYPRE_Int            *rowindices;
   HYPRE_Complex        *rowvalues;
   HYPRE_Int             getrowactive;

   hypre_IJAssumedPart  *assumed_partition; /* only populated if
                                              no_global_partition option is used
                                              (compile-time option)*/

} hypre_ParCSRMatrix;

/*--------------------------------------------------------------------------
 * Accessor functions for the Parallel CSR Matrix structure
 *--------------------------------------------------------------------------*/

#define hypre_ParCSRMatrixComm(matrix)            ((matrix) -> comm)
#define hypre_ParCSRMatrixGlobalNumRows(matrix)   ((matrix) -> global_num_rows)
#define hypre_ParCSRMatrixGlobalNumCols(matrix)   ((matrix) -> global_num_cols)
#define hypre_ParCSRMatrixFirstRowIndex(matrix)   ((matrix) -> first_row_index)
#define hypre_ParCSRMatrixFirstColDiag(matrix)    ((matrix) -> first_col_diag)
#define hypre_ParCSRMatrixLastRowIndex(matrix)    ((matrix) -> last_row_index)
#define hypre_ParCSRMatrixLastColDiag(matrix)     ((matrix) -> last_col_diag)
#define hypre_ParCSRMatrixDiag(matrix)            ((matrix) -> diag)
#define hypre_ParCSRMatrixOffd(matrix)            ((matrix) -> offd)
#define hypre_ParCSRMatrixDiagT(matrix)            ((matrix) -> diagT)
#define hypre_ParCSRMatrixOffdT(matrix)            ((matrix) -> offdT)
#define hypre_ParCSRMatrixColMapOffd(matrix)      ((matrix) -> col_map_offd)
#define hypre_ParCSRMatrixRowStarts(matrix)       ((matrix) -> row_starts)
#define hypre_ParCSRMatrixColStarts(matrix)       ((matrix) -> col_starts)
#define hypre_ParCSRMatrixCommPkg(matrix)         ((matrix) -> comm_pkg)
#define hypre_ParCSRMatrixCommPkgT(matrix)        ((matrix) -> comm_pkgT)
#define hypre_ParCSRMatrixOwnsData(matrix)        ((matrix) -> owns_data)
#define hypre_ParCSRMatrixOwnsRowStarts(matrix)   ((matrix) -> owns_row_starts)
#define hypre_ParCSRMatrixOwnsColStarts(matrix)   ((matrix) -> owns_col_starts)
#define hypre_ParCSRMatrixNumRows(matrix) \
hypre_CSRMatrixNumRows(hypre_ParCSRMatrixDiag(matrix))
#define hypre_ParCSRMatrixNumCols(matrix) \
hypre_CSRMatrixNumCols(hypre_ParCSRMatrixDiag(matrix))
#define hypre_ParCSRMatrixNumNonzeros(matrix)     ((matrix) -> num_nonzeros)
#define hypre_ParCSRMatrixDNumNonzeros(matrix)    ((matrix) -> d_num_nonzeros)
#define hypre_ParCSRMatrixRowindices(matrix)      ((matrix) -> rowindices)
#define hypre_ParCSRMatrixRowvalues(matrix)       ((matrix) -> rowvalues)
#define hypre_ParCSRMatrixGetrowactive(matrix)    ((matrix) -> getrowactive)
#define hypre_ParCSRMatrixAssumedPartition(matrix) ((matrix) -> assumed_partition)

#endif

/* HYPRE_parcsr_matrix.c */
HYPRE_Int HYPRE_ParCSRMatrixCreate ( MPI_Comm comm , HYPRE_Int global_num_rows , HYPRE_Int global_num_cols , HYPRE_Int *row_starts , HYPRE_Int *col_starts , HYPRE_Int num_cols_offd , HYPRE_Int num_nonzeros_diag , HYPRE_Int num_nonzeros_offd , HYPRE_ParCSRMatrix *matrix );
HYPRE_Int HYPRE_ParCSRMatrixDestroy ( HYPRE_ParCSRMatrix matrix );
HYPRE_Int HYPRE_ParCSRMatrixInitialize ( HYPRE_ParCSRMatrix matrix );
HYPRE_Int HYPRE_ParCSRMatrixRead ( MPI_Comm comm , const char *file_name , HYPRE_ParCSRMatrix *matrix );
HYPRE_Int HYPRE_ParCSRMatrixPrint ( HYPRE_ParCSRMatrix matrix , const char *file_name );
HYPRE_Int HYPRE_ParCSRMatrixGetComm ( HYPRE_ParCSRMatrix matrix , MPI_Comm *comm );
HYPRE_Int HYPRE_ParCSRMatrixGetDims ( HYPRE_ParCSRMatrix matrix , HYPRE_Int *M , HYPRE_Int *N );
HYPRE_Int HYPRE_ParCSRMatrixGetRowPartitioning ( HYPRE_ParCSRMatrix matrix , HYPRE_Int **row_partitioning_ptr );
HYPRE_Int HYPRE_ParCSRMatrixGetColPartitioning ( HYPRE_ParCSRMatrix matrix , HYPRE_Int **col_partitioning_ptr );
HYPRE_Int HYPRE_ParCSRMatrixGetLocalRange ( HYPRE_ParCSRMatrix matrix , HYPRE_Int *row_start , HYPRE_Int *row_end , HYPRE_Int *col_start , HYPRE_Int *col_end );
HYPRE_Int HYPRE_ParCSRMatrixGetRow ( HYPRE_ParCSRMatrix matrix , HYPRE_Int row , HYPRE_Int *size , HYPRE_Int **col_ind , HYPRE_Complex **values );
HYPRE_Int HYPRE_ParCSRMatrixRestoreRow ( HYPRE_ParCSRMatrix matrix , HYPRE_Int row , HYPRE_Int *size , HYPRE_Int **col_ind , HYPRE_Complex **values );
HYPRE_Int HYPRE_CSRMatrixToParCSRMatrix ( MPI_Comm comm , HYPRE_CSRMatrix A_CSR , HYPRE_Int *row_partitioning , HYPRE_Int *col_partitioning , HYPRE_ParCSRMatrix *matrix );
HYPRE_Int HYPRE_CSRMatrixToParCSRMatrix_WithNewPartitioning ( MPI_Comm comm , HYPRE_CSRMatrix A_CSR , HYPRE_ParCSRMatrix *matrix );
HYPRE_Int HYPRE_ParCSRMatrixMatvec ( HYPRE_Complex alpha , HYPRE_ParCSRMatrix A , HYPRE_ParVector x , HYPRE_Complex beta , HYPRE_ParVector y );
HYPRE_Int HYPRE_ParCSRMatrixMatvecT ( HYPRE_Complex alpha , HYPRE_ParCSRMatrix A , HYPRE_ParVector x , HYPRE_Complex beta , HYPRE_ParVector y );

/* HYPRE_parcsr_vector.c */
HYPRE_Int HYPRE_ParVectorCreate ( MPI_Comm comm , HYPRE_Int global_size , HYPRE_Int *partitioning , HYPRE_ParVector *vector );
HYPRE_Int HYPRE_ParMultiVectorCreate ( MPI_Comm comm , HYPRE_Int global_size , HYPRE_Int *partitioning , HYPRE_Int number_vectors , HYPRE_ParVector *vector );
HYPRE_Int HYPRE_ParVectorDestroy ( HYPRE_ParVector vector );
HYPRE_Int HYPRE_ParVectorInitialize ( HYPRE_ParVector vector );
HYPRE_Int HYPRE_ParVectorRead ( MPI_Comm comm , const char *file_name , HYPRE_ParVector *vector );
HYPRE_Int HYPRE_ParVectorPrint ( HYPRE_ParVector vector , const char *file_name );
HYPRE_Int HYPRE_ParVectorSetConstantValues ( HYPRE_ParVector vector , HYPRE_Complex value );
HYPRE_Int HYPRE_ParVectorSetRandomValues ( HYPRE_ParVector vector , HYPRE_Int seed );
HYPRE_Int HYPRE_ParVectorCopy ( HYPRE_ParVector x , HYPRE_ParVector y );
HYPRE_ParVector HYPRE_ParVectorCloneShallow ( HYPRE_ParVector x );
HYPRE_Int HYPRE_ParVectorScale ( HYPRE_Complex value , HYPRE_ParVector x );
HYPRE_Int HYPRE_ParVectorAxpy ( HYPRE_Complex alpha , HYPRE_ParVector x , HYPRE_ParVector y );
HYPRE_Int HYPRE_ParVectorInnerProd ( HYPRE_ParVector x , HYPRE_ParVector y , HYPRE_Real *prod );
HYPRE_Int HYPRE_VectorToParVector ( MPI_Comm comm , HYPRE_Vector b , HYPRE_Int *partitioning , HYPRE_ParVector *vector );

/* new_commpkg.c */
HYPRE_Int hypre_PrintCommpkg ( hypre_ParCSRMatrix *A , const char *file_name );
HYPRE_Int hypre_NewCommPkgCreate_core ( MPI_Comm comm , HYPRE_Int *col_map_off_d , HYPRE_Int first_col_diag , HYPRE_Int col_start , HYPRE_Int col_end , HYPRE_Int num_cols_off_d , HYPRE_Int global_num_cols , HYPRE_Int *p_num_recvs , HYPRE_Int **p_recv_procs , HYPRE_Int **p_recv_vec_starts , HYPRE_Int *p_num_sends , HYPRE_Int **p_send_procs , HYPRE_Int **p_send_map_starts , HYPRE_Int **p_send_map_elements , hypre_IJAssumedPart *apart );
HYPRE_Int hypre_NewCommPkgCreate ( hypre_ParCSRMatrix *parcsr_A );
HYPRE_Int hypre_NewCommPkgDestroy ( hypre_ParCSRMatrix *parcsr_A );
HYPRE_Int hypre_RangeFillResponseIJDetermineRecvProcs ( void *p_recv_contact_buf , HYPRE_Int contact_size , HYPRE_Int contact_proc , void *ro , MPI_Comm comm , void **p_send_response_buf , HYPRE_Int *response_message_size );
HYPRE_Int hypre_FillResponseIJDetermineSendProcs ( void *p_recv_contact_buf , HYPRE_Int contact_size , HYPRE_Int contact_proc , void *ro , MPI_Comm comm , void **p_send_response_buf , HYPRE_Int *response_message_size );

/* par_csr_assumed_part.c */
HYPRE_Int hypre_LocateAssummedPartition ( MPI_Comm comm , HYPRE_Int row_start , HYPRE_Int row_end , HYPRE_Int global_first_row , HYPRE_Int global_num_rows , hypre_IJAssumedPart *part , HYPRE_Int myid );
HYPRE_Int hypre_ParCSRMatrixCreateAssumedPartition ( hypre_ParCSRMatrix *matrix );
HYPRE_Int hypre_AssumedPartitionDestroy ( hypre_IJAssumedPart *apart );
HYPRE_Int hypre_GetAssumedPartitionProcFromRow ( MPI_Comm comm , HYPRE_Int row , HYPRE_Int global_first_row , HYPRE_Int global_num_rows , HYPRE_Int *proc_id );
HYPRE_Int hypre_GetAssumedPartitionRowRange ( MPI_Comm comm , HYPRE_Int proc_id , HYPRE_Int global_first_row , HYPRE_Int global_num_rows , HYPRE_Int *row_start , HYPRE_Int *row_end );
HYPRE_Int hypre_ParVectorCreateAssumedPartition ( hypre_ParVector *vector );

/* par_csr_communication.c */
hypre_ParCSRCommHandle *hypre_ParCSRCommHandleCreate ( HYPRE_Int job , hypre_ParCSRCommPkg *comm_pkg , void *send_data , void *recv_data );
HYPRE_Int hypre_ParCSRCommHandleDestroy ( hypre_ParCSRCommHandle *comm_handle );
void hypre_MatvecCommPkgCreate_core ( MPI_Comm comm , HYPRE_Int *col_map_offd , HYPRE_Int first_col_diag , HYPRE_Int *col_starts , HYPRE_Int num_cols_diag , HYPRE_Int num_cols_offd , HYPRE_Int firstColDiag , HYPRE_Int *colMapOffd , HYPRE_Int data , HYPRE_Int *p_num_recvs , HYPRE_Int **p_recv_procs , HYPRE_Int **p_recv_vec_starts , HYPRE_Int *p_num_sends , HYPRE_Int **p_send_procs , HYPRE_Int **p_send_map_starts , HYPRE_Int **p_send_map_elmts );
HYPRE_Int hypre_MatvecCommPkgCreate ( hypre_ParCSRMatrix *A );
HYPRE_Int hypre_MatvecCommPkgDestroy ( hypre_ParCSRCommPkg *comm_pkg );
HYPRE_Int hypre_BuildCSRMatrixMPIDataType ( HYPRE_Int num_nonzeros , HYPRE_Int num_rows , HYPRE_Complex *a_data , HYPRE_Int *a_i , HYPRE_Int *a_j , hypre_MPI_Datatype *csr_matrix_datatype );
HYPRE_Int hypre_BuildCSRJDataType ( HYPRE_Int num_nonzeros , HYPRE_Complex *a_data , HYPRE_Int *a_j , hypre_MPI_Datatype *csr_jdata_datatype );

/* par_csr_matop.c */
void hypre_ParMatmul_RowSizes ( HYPRE_Int **C_diag_i , HYPRE_Int **C_offd_i , HYPRE_Int *A_diag_i , HYPRE_Int *A_diag_j , HYPRE_Int *A_offd_i , HYPRE_Int *A_offd_j , HYPRE_Int *B_diag_i , HYPRE_Int *B_diag_j , HYPRE_Int *B_offd_i , HYPRE_Int *B_offd_j , HYPRE_Int *B_ext_diag_i , HYPRE_Int *B_ext_diag_j , HYPRE_Int *B_ext_offd_i , HYPRE_Int *B_ext_offd_j , HYPRE_Int *map_B_to_C , HYPRE_Int *C_diag_size , HYPRE_Int *C_offd_size , HYPRE_Int num_rows_diag_A , HYPRE_Int num_cols_offd_A , HYPRE_Int allsquare , HYPRE_Int num_cols_diag_B , HYPRE_Int num_cols_offd_B , HYPRE_Int num_cols_offd_C );
hypre_ParCSRMatrix *hypre_ParMatmul ( hypre_ParCSRMatrix *A , hypre_ParCSRMatrix *B );
void hypre_ParCSRMatrixExtractBExt_Arrays ( HYPRE_Int **pB_ext_i , HYPRE_Int **pB_ext_j , HYPRE_Complex **pB_ext_data , HYPRE_Int **pB_ext_row_map , HYPRE_Int *num_nonzeros , HYPRE_Int data , HYPRE_Int find_row_map , MPI_Comm comm , hypre_ParCSRCommPkg *comm_pkg , HYPRE_Int num_cols_B , HYPRE_Int num_recvs , HYPRE_Int num_sends , HYPRE_Int first_col_diag , HYPRE_Int *row_starts , HYPRE_Int *recv_vec_starts , HYPRE_Int *send_map_starts , HYPRE_Int *send_map_elmts , HYPRE_Int *diag_i , HYPRE_Int *diag_j , HYPRE_Int *offd_i , HYPRE_Int *offd_j , HYPRE_Int *col_map_offd , HYPRE_Real *diag_data , HYPRE_Real *offd_data );
void hypre_ParCSRMatrixExtractBExt_Arrays_Overlap ( HYPRE_Int **pB_ext_i , HYPRE_Int **pB_ext_j , HYPRE_Complex **pB_ext_data , HYPRE_Int **pB_ext_row_map , HYPRE_Int *num_nonzeros , HYPRE_Int data , HYPRE_Int find_row_map , MPI_Comm comm , hypre_ParCSRCommPkg *comm_pkg , HYPRE_Int num_cols_B , HYPRE_Int num_recvs , HYPRE_Int num_sends , HYPRE_Int first_col_diag , HYPRE_Int *row_starts , HYPRE_Int *recv_vec_starts , HYPRE_Int *send_map_starts , HYPRE_Int *send_map_elmts , HYPRE_Int *diag_i , HYPRE_Int *diag_j , HYPRE_Int *offd_i , HYPRE_Int *offd_j , HYPRE_Int *col_map_offd , HYPRE_Real *diag_data , HYPRE_Real *offd_data, hypre_ParCSRCommHandle **comm_handle_idx, hypre_ParCSRCommHandle **comm_handle_data, HYPRE_Int *CF_marker, HYPRE_Int *CF_marker_offd, HYPRE_Int skip_fine, HYPRE_Int skip_same_sign );
hypre_CSRMatrix *hypre_ParCSRMatrixExtractBExt ( hypre_ParCSRMatrix *B , hypre_ParCSRMatrix *A , HYPRE_Int data );
hypre_CSRMatrix *hypre_ParCSRMatrixExtractBExt_Overlap ( hypre_ParCSRMatrix *B , hypre_ParCSRMatrix *A , HYPRE_Int data, hypre_ParCSRCommHandle **comm_handle_idx, hypre_ParCSRCommHandle **comm_handle_data, HYPRE_Int *CF_marker, HYPRE_Int *CF_marker_offd, HYPRE_Int skip_fine, HYPRE_Int skip_same_sign );
HYPRE_Int hypre_ParCSRMatrixTranspose ( hypre_ParCSRMatrix *A , hypre_ParCSRMatrix **AT_ptr , HYPRE_Int data );
void hypre_ParCSRMatrixGenSpanningTree ( hypre_ParCSRMatrix *G_csr , HYPRE_Int **indices , HYPRE_Int G_type );
void hypre_ParCSRMatrixExtractSubmatrices ( hypre_ParCSRMatrix *A_csr , HYPRE_Int *indices2 , hypre_ParCSRMatrix ***submatrices );
void hypre_ParCSRMatrixExtractRowSubmatrices ( hypre_ParCSRMatrix *A_csr , HYPRE_Int *indices2 , hypre_ParCSRMatrix ***submatrices );
HYPRE_Complex hypre_ParCSRMatrixLocalSumElts ( hypre_ParCSRMatrix *A );
HYPRE_Int hypre_ParCSRMatrixAminvDB ( hypre_ParCSRMatrix *A , hypre_ParCSRMatrix *B , HYPRE_Complex *d , hypre_ParCSRMatrix **C_ptr );
hypre_ParCSRMatrix *hypre_ParTMatmul ( hypre_ParCSRMatrix *A , hypre_ParCSRMatrix *B );
#ifdef HYPRE_USING_PERSISTENT_COMM
hypre_ParCSRPersistentCommHandle *
hypre_ParCSRCommPkgGetPersistentCommHandle(HYPRE_Int job,
             hypre_ParCSRCommPkg *comm_pkg);

hypre_ParCSRPersistentCommHandle *
hypre_ParCSRPersistentCommHandleCreate(HYPRE_Int job,
             hypre_ParCSRCommPkg *comm_pkg,
             void *send_data,
             void *recv_data);
void
hypre_ParCSRPersistentCommHandleDestroy(hypre_ParCSRPersistentCommHandle *comm_handle);
void hypre_ParCSRPersistentCommHandleStart(hypre_ParCSRPersistentCommHandle *comm_handle);
void hypre_ParCSRPersistentCommHandleWait(hypre_ParCSRPersistentCommHandle *comm_handle);
#endif

/* par_csr_matop_marked.c */
void hypre_ParMatmul_RowSizes_Marked ( HYPRE_Int **C_diag_i , HYPRE_Int **C_offd_i , HYPRE_Int **B_marker , HYPRE_Int *A_diag_i , HYPRE_Int *A_diag_j , HYPRE_Int *A_offd_i , HYPRE_Int *A_offd_j , HYPRE_Int *B_diag_i , HYPRE_Int *B_diag_j , HYPRE_Int *B_offd_i , HYPRE_Int *B_offd_j , HYPRE_Int *B_ext_diag_i , HYPRE_Int *B_ext_diag_j , HYPRE_Int *B_ext_offd_i , HYPRE_Int *B_ext_offd_j , HYPRE_Int *map_B_to_C , HYPRE_Int *C_diag_size , HYPRE_Int *C_offd_size , HYPRE_Int num_rows_diag_A , HYPRE_Int num_cols_offd_A , HYPRE_Int allsquare , HYPRE_Int num_cols_diag_B , HYPRE_Int num_cols_offd_B , HYPRE_Int num_cols_offd_C , HYPRE_Int *CF_marker , HYPRE_Int *dof_func , HYPRE_Int *dof_func_offd );
hypre_ParCSRMatrix *hypre_ParMatmul_FC ( hypre_ParCSRMatrix *A , hypre_ParCSRMatrix *P , HYPRE_Int *CF_marker , HYPRE_Int *dof_func , HYPRE_Int *dof_func_offd );
void hypre_ParMatScaleDiagInv_F ( hypre_ParCSRMatrix *C , hypre_ParCSRMatrix *A , HYPRE_Complex weight , HYPRE_Int *CF_marker );
hypre_ParCSRMatrix *hypre_ParMatMinus_F ( hypre_ParCSRMatrix *P , hypre_ParCSRMatrix *C , HYPRE_Int *CF_marker );
void hypre_ParCSRMatrixZero_F ( hypre_ParCSRMatrix *P , HYPRE_Int *CF_marker );
void hypre_ParCSRMatrixCopy_C ( hypre_ParCSRMatrix *P , hypre_ParCSRMatrix *C , HYPRE_Int *CF_marker );
void hypre_ParCSRMatrixDropEntries ( hypre_ParCSRMatrix *C , hypre_ParCSRMatrix *P , HYPRE_Int *CF_marker );

/* par_csr_matrix.c */
hypre_ParCSRMatrix *hypre_ParCSRMatrixCreate ( MPI_Comm comm , HYPRE_Int global_num_rows , HYPRE_Int global_num_cols , HYPRE_Int *row_starts , HYPRE_Int *col_starts , HYPRE_Int num_cols_offd , HYPRE_Int num_nonzeros_diag , HYPRE_Int num_nonzeros_offd );
HYPRE_Int hypre_ParCSRMatrixDestroy ( hypre_ParCSRMatrix *matrix );
HYPRE_Int hypre_ParCSRMatrixInitialize ( hypre_ParCSRMatrix *matrix );
HYPRE_Int hypre_ParCSRMatrixSetNumNonzeros ( hypre_ParCSRMatrix *matrix );
HYPRE_Int hypre_ParCSRMatrixSetDNumNonzeros ( hypre_ParCSRMatrix *matrix );
HYPRE_Int hypre_ParCSRMatrixSetDataOwner ( hypre_ParCSRMatrix *matrix , HYPRE_Int owns_data );
HYPRE_Int hypre_ParCSRMatrixSetRowStartsOwner ( hypre_ParCSRMatrix *matrix , HYPRE_Int owns_row_starts );
HYPRE_Int hypre_ParCSRMatrixSetColStartsOwner ( hypre_ParCSRMatrix *matrix , HYPRE_Int owns_col_starts );
hypre_ParCSRMatrix *hypre_ParCSRMatrixRead ( MPI_Comm comm , const char *file_name );
HYPRE_Int hypre_ParCSRMatrixPrint ( hypre_ParCSRMatrix *matrix , const char *file_name );
HYPRE_Int hypre_ParCSRMatrixPrintIJ ( const hypre_ParCSRMatrix *matrix , const HYPRE_Int base_i , const HYPRE_Int base_j , const char *filename );
HYPRE_Int hypre_ParCSRMatrixReadIJ ( MPI_Comm comm , const char *filename , HYPRE_Int *base_i_ptr , HYPRE_Int *base_j_ptr , hypre_ParCSRMatrix **matrix_ptr );
HYPRE_Int hypre_ParCSRMatrixGetLocalRange ( hypre_ParCSRMatrix *matrix , HYPRE_Int *row_start , HYPRE_Int *row_end , HYPRE_Int *col_start , HYPRE_Int *col_end );
HYPRE_Int hypre_ParCSRMatrixGetRow ( hypre_ParCSRMatrix *mat , HYPRE_Int row , HYPRE_Int *size , HYPRE_Int **col_ind , HYPRE_Complex **values );
HYPRE_Int hypre_ParCSRMatrixRestoreRow ( hypre_ParCSRMatrix *matrix , HYPRE_Int row , HYPRE_Int *size , HYPRE_Int **col_ind , HYPRE_Complex **values );
hypre_ParCSRMatrix *hypre_CSRMatrixToParCSRMatrix ( MPI_Comm comm , hypre_CSRMatrix *A , HYPRE_Int *row_starts , HYPRE_Int *col_starts );
HYPRE_Int GenerateDiagAndOffd ( hypre_CSRMatrix *A , hypre_ParCSRMatrix *matrix , HYPRE_Int first_col_diag , HYPRE_Int last_col_diag );
hypre_CSRMatrix *hypre_MergeDiagAndOffd ( hypre_ParCSRMatrix *par_matrix );
hypre_CSRMatrix *hypre_ParCSRMatrixToCSRMatrixAll ( hypre_ParCSRMatrix *par_matrix );
HYPRE_Int hypre_ParCSRMatrixCopy ( hypre_ParCSRMatrix *A , hypre_ParCSRMatrix *B , HYPRE_Int copy_data );
HYPRE_Int hypre_FillResponseParToCSRMatrix ( void *p_recv_contact_buf , HYPRE_Int contact_size , HYPRE_Int contact_proc , void *ro , MPI_Comm comm , void **p_send_response_buf , HYPRE_Int *response_message_size );
hypre_ParCSRMatrix *hypre_ParCSRMatrixCompleteClone ( hypre_ParCSRMatrix *A );
hypre_ParCSRMatrix *hypre_ParCSRMatrixUnion ( hypre_ParCSRMatrix *A , hypre_ParCSRMatrix *B );


/* par_csr_matvec.c */
// y = alpha*A*x + beta*b
HYPRE_Int hypre_ParCSRMatrixMatvecOutOfPlace ( HYPRE_Complex alpha , hypre_ParCSRMatrix *A , hypre_ParVector *x , HYPRE_Complex beta , hypre_ParVector *b, hypre_ParVector *y );
// y = alpha*A*x + beta*y
HYPRE_Int hypre_ParCSRMatrixMatvec ( HYPRE_Complex alpha , hypre_ParCSRMatrix *A , hypre_ParVector *x , HYPRE_Complex beta , hypre_ParVector *y );
HYPRE_Int hypre_ParCSRMatrixMatvecT ( HYPRE_Complex alpha , hypre_ParCSRMatrix *A , hypre_ParVector *x , HYPRE_Complex beta , hypre_ParVector *y );
HYPRE_Int hypre_ParCSRMatrixMatvec_FF ( HYPRE_Complex alpha , hypre_ParCSRMatrix *A , hypre_ParVector *x , HYPRE_Complex beta , hypre_ParVector *y , HYPRE_Int *CF_marker , HYPRE_Int fpt );

/* par_vector.c */
hypre_ParVector *hypre_ParVectorCreate ( MPI_Comm comm , HYPRE_Int global_size , HYPRE_Int *partitioning );
hypre_ParVector *hypre_ParMultiVectorCreate ( MPI_Comm comm , HYPRE_Int global_size , HYPRE_Int *partitioning , HYPRE_Int num_vectors );
HYPRE_Int hypre_ParVectorDestroy ( hypre_ParVector *vector );
HYPRE_Int hypre_ParVectorInitialize ( hypre_ParVector *vector );
HYPRE_Int hypre_ParVectorSetDataOwner ( hypre_ParVector *vector , HYPRE_Int owns_data );
HYPRE_Int hypre_ParVectorSetPartitioningOwner ( hypre_ParVector *vector , HYPRE_Int owns_partitioning );
HYPRE_Int hypre_ParVectorSetNumVectors ( hypre_ParVector *vector , HYPRE_Int num_vectors );
hypre_ParVector *hypre_ParVectorRead ( MPI_Comm comm , const char *file_name );
HYPRE_Int hypre_ParVectorPrint ( hypre_ParVector *vector , const char *file_name );
HYPRE_Int hypre_ParVectorSetConstantValues ( hypre_ParVector *v , HYPRE_Complex value );
HYPRE_Int hypre_ParVectorSetRandomValues ( hypre_ParVector *v , HYPRE_Int seed );
HYPRE_Int hypre_ParVectorCopy ( hypre_ParVector *x , hypre_ParVector *y );
hypre_ParVector *hypre_ParVectorCloneShallow ( hypre_ParVector *x );
HYPRE_Int hypre_ParVectorScale ( HYPRE_Complex alpha , hypre_ParVector *y );
HYPRE_Int hypre_ParVectorAxpy ( HYPRE_Complex alpha , hypre_ParVector *x , hypre_ParVector *y );
HYPRE_Real hypre_ParVectorInnerProd ( hypre_ParVector *x , hypre_ParVector *y );
hypre_ParVector *hypre_VectorToParVector ( MPI_Comm comm , hypre_Vector *v , HYPRE_Int *vec_starts );
hypre_Vector *hypre_ParVectorToVectorAll ( hypre_ParVector *par_v );
HYPRE_Int hypre_ParVectorPrintIJ ( hypre_ParVector *vector , HYPRE_Int base_j , const char *filename );
HYPRE_Int hypre_ParVectorReadIJ ( MPI_Comm comm , const char *filename , HYPRE_Int *base_j_ptr , hypre_ParVector **vector_ptr );
HYPRE_Int hypre_FillResponseParToVectorAll ( void *p_recv_contact_buf , HYPRE_Int contact_size , HYPRE_Int contact_proc , void *ro , MPI_Comm comm , void **p_send_response_buf , HYPRE_Int *response_message_size );
HYPRE_Complex hypre_ParVectorLocalSumElts ( hypre_ParVector *vector );

#ifdef __cplusplus
}
#endif

#endif


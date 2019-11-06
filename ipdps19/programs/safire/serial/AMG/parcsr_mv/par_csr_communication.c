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

#include "_hypre_parcsr_mv.h"

/*==========================================================================*/

#ifdef HYPRE_USING_PERSISTENT_COMM
static CommPkgJobType getJobTypeOf(HYPRE_Int job)
{
   CommPkgJobType job_type = HYPRE_COMM_PKG_JOB_COMPLEX;
   switch (job)
   {
   case  1:
      job_type = HYPRE_COMM_PKG_JOB_COMPLEX;
      break;
   case  2:
      job_type = HYPRE_COMM_PKG_JOB_COMPLEX_TRANSPOSE;
      break;
   case  11:
      job_type = HYPRE_COMM_PKG_JOB_INT;
      break;
   case  12:
      job_type = HYPRE_COMM_PKG_JOB_INT_TRANSPOSE;
      break;
   } // switch (job)

   return job_type;
}

/**
 * When send_data and recv_data are NULL, buffers are internally allocated
 * and CommHandle owns the buffer
 */
hypre_ParCSRPersistentCommHandle *
hypre_ParCSRPersistentCommHandleCreate( HYPRE_Int job,
			                               hypre_ParCSRCommPkg *comm_pkg,
                                        void *send_data,
                                        void *recv_data)
{
   HYPRE_Int                  i;

   hypre_ParCSRPersistentCommHandle *comm_handle = hypre_CTAlloc(hypre_ParCSRPersistentCommHandle, 1);

   if (!send_data) comm_handle->own_send_data = 1;
   if (!recv_data) comm_handle->own_recv_data = 1;

   CommPkgJobType job_type = getJobTypeOf(job);

   HYPRE_Int num_sends = hypre_ParCSRCommPkgNumSends(comm_pkg);
   HYPRE_Int num_recvs = hypre_ParCSRCommPkgNumRecvs(comm_pkg);
   MPI_Comm  comm      = hypre_ParCSRCommPkgComm(comm_pkg);

   HYPRE_Int num_requests = num_sends + num_recvs;
   hypre_MPI_Request *requests = hypre_CTAlloc(hypre_MPI_Request, num_requests);

   hypre_ParCSRCommHandleNumRequests(comm_handle) = num_requests;
   hypre_ParCSRCommHandleRequests(comm_handle)    = requests;

   switch (job_type)
   {
      case HYPRE_COMM_PKG_JOB_COMPLEX:
         if (!send_data)
         {
            send_data = hypre_TAlloc(HYPRE_Complex, hypre_ParCSRCommPkgSendMapStart(comm_pkg, num_sends));
         }
         if (!recv_data)
         {
            recv_data = hypre_TAlloc(HYPRE_Complex, hypre_ParCSRCommPkgRecvVecStart(comm_pkg, num_recvs));  
         }
         for (i = 0; i < num_recvs; ++i)
         {
            HYPRE_Int ip = hypre_ParCSRCommPkgRecvProc(comm_pkg, i);
            HYPRE_Int vec_start = hypre_ParCSRCommPkgRecvVecStart(comm_pkg, i);
            HYPRE_Int vec_len = hypre_ParCSRCommPkgRecvVecStart(comm_pkg, i + 1) - vec_start;
            hypre_MPI_Recv_init((HYPRE_Complex *)recv_data + vec_start, vec_len, HYPRE_MPI_COMPLEX,
                          ip, 0, comm, requests + i);
         }
         for (i = 0; i < num_sends; ++i)
         {
            HYPRE_Int ip = hypre_ParCSRCommPkgSendProc(comm_pkg, i);
            HYPRE_Int vec_start = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i);
            HYPRE_Int vec_len = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i + 1) - vec_start;

            hypre_MPI_Send_init((HYPRE_Complex *)send_data + vec_start, vec_len, HYPRE_MPI_COMPLEX,
                          ip, 0, comm, requests + num_recvs + i);
         }
         break;

      case HYPRE_COMM_PKG_JOB_COMPLEX_TRANSPOSE:
         if (!recv_data)
         {
            recv_data = hypre_TAlloc(HYPRE_Complex, hypre_ParCSRCommPkgSendMapStart(comm_pkg, num_sends));  
         }
         if (!send_data)
         {
            send_data = hypre_TAlloc(HYPRE_Complex, hypre_ParCSRCommPkgRecvVecStart(comm_pkg, num_recvs));  
         }
         for (i = 0; i < num_sends; ++i)
         {
            HYPRE_Int ip = hypre_ParCSRCommPkgSendProc(comm_pkg, i);
            HYPRE_Int vec_start = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i);
            HYPRE_Int vec_len = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i + 1) - vec_start;
            hypre_MPI_Recv_init((HYPRE_Complex *)recv_data + vec_start, vec_len, HYPRE_MPI_COMPLEX,
                          ip, 0, comm, requests + i);
         }
         for (i = 0; i < num_recvs; ++i)
         {
            HYPRE_Int ip = hypre_ParCSRCommPkgRecvProc(comm_pkg, i);
            HYPRE_Int vec_start = hypre_ParCSRCommPkgRecvVecStart(comm_pkg, i);
            HYPRE_Int vec_len = hypre_ParCSRCommPkgRecvVecStart(comm_pkg, i + 1) - vec_start;

            hypre_MPI_Send_init((HYPRE_Complex *)send_data + vec_start, vec_len, HYPRE_MPI_COMPLEX,
                          ip, 0, comm, requests + num_sends + i);
         }
         break;

      case HYPRE_COMM_PKG_JOB_INT:
         if (!send_data)
         {
            send_data = hypre_TAlloc(HYPRE_Int, hypre_ParCSRCommPkgSendMapStart(comm_pkg, num_sends));  
         }
         if (!recv_data)
         {
            recv_data = hypre_TAlloc(HYPRE_Int, hypre_ParCSRCommPkgRecvVecStart(comm_pkg, num_recvs));  
         }
         for (i = 0; i < num_recvs; ++i)
         {
            HYPRE_Int ip = hypre_ParCSRCommPkgRecvProc(comm_pkg, i);
            HYPRE_Int vec_start = hypre_ParCSRCommPkgRecvVecStart(comm_pkg, i);
            HYPRE_Int vec_len = hypre_ParCSRCommPkgRecvVecStart(comm_pkg, i + 1) - vec_start;
            hypre_MPI_Recv_init((HYPRE_Int *)recv_data + vec_start, vec_len, HYPRE_MPI_INT,
                          ip, 0, comm, requests + i);
         }
         for (i = 0; i < num_sends; ++i)
         {
            HYPRE_Int ip = hypre_ParCSRCommPkgSendProc(comm_pkg, i);
            HYPRE_Int vec_start = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i);
            HYPRE_Int vec_len = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i + 1) - vec_start;

            hypre_MPI_Send_init((HYPRE_Int *)send_data + vec_start, vec_len, HYPRE_MPI_INT,
                          ip, 0, comm, requests + num_recvs + i);
         }
         break;

      case HYPRE_COMM_PKG_JOB_INT_TRANSPOSE:
         if (!recv_data)
         {
            recv_data = hypre_TAlloc(HYPRE_Int, hypre_ParCSRCommPkgSendMapStart(comm_pkg, num_sends));  
         }
         if (!send_data)
         {
            send_data = hypre_TAlloc(HYPRE_Int, hypre_ParCSRCommPkgRecvVecStart(comm_pkg, num_recvs));  
         }
         for (i = 0; i < num_sends; ++i)
         {
            HYPRE_Int ip = hypre_ParCSRCommPkgSendProc(comm_pkg, i);
            HYPRE_Int vec_start = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i);
            HYPRE_Int vec_len = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i + 1) - vec_start;
            hypre_MPI_Recv_init((HYPRE_Int *)recv_data + vec_start, vec_len, HYPRE_MPI_INT,
                          ip, 0, comm, requests + i);
         }
         for (i = 0; i < num_recvs; ++i)
         {
            HYPRE_Int ip = hypre_ParCSRCommPkgRecvProc(comm_pkg, i);
            HYPRE_Int vec_start = hypre_ParCSRCommPkgRecvVecStart(comm_pkg, i);
            HYPRE_Int vec_len = hypre_ParCSRCommPkgRecvVecStart(comm_pkg, i + 1) - vec_start;

            hypre_MPI_Send_init((HYPRE_Int *)send_data + vec_start, vec_len, HYPRE_MPI_INT,
                          ip, 0, comm, requests + num_sends + i);
         }
         break;
      default:
         hypre_assert(1 == 0);
         break;
   } // switch (job_type)

   hypre_ParCSRCommHandleRecvData(comm_handle) = recv_data;
   hypre_ParCSRCommHandleSendData(comm_handle) = send_data;

   return ( comm_handle );
}


hypre_ParCSRPersistentCommHandle *
hypre_ParCSRCommPkgGetPersistentCommHandle( HYPRE_Int job,
                                            hypre_ParCSRCommPkg *comm_pkg )
{
   CommPkgJobType type = getJobTypeOf(job);
   if (!comm_pkg->persistent_comm_handles[type])
   {
      comm_pkg->persistent_comm_handles[type] =
         hypre_ParCSRPersistentCommHandleCreate(job, comm_pkg, NULL, NULL);
      // data is owned by persistent comm handle
   }
   return comm_pkg->persistent_comm_handles[type];
}

void
hypre_ParCSRPersistentCommHandleDestroy( hypre_ParCSRPersistentCommHandle *comm_handle )
{
   if (comm_handle->own_send_data)
   {
      hypre_TFree(comm_handle->send_data);
   }
   if (comm_handle->own_recv_data)
   {
      hypre_TFree(comm_handle->recv_data);
   }
   hypre_TFree(comm_handle->requests);
   hypre_TFree(comm_handle);
}

void hypre_ParCSRPersistentCommHandleStart( hypre_ParCSRPersistentCommHandle *comm_handle )
{
   if (hypre_ParCSRCommHandleNumRequests(comm_handle) > 0)
   {
      HYPRE_Int ret = hypre_MPI_Startall(hypre_ParCSRCommHandleNumRequests(comm_handle),
                                         hypre_ParCSRCommHandleRequests(comm_handle));
      if (hypre_MPI_SUCCESS != ret)
      {
         hypre_error_w_msg(HYPRE_ERROR_GENERIC,"MPI error\n");
         /*hypre_printf("MPI error %d in %s (%s, line %u)\n", ret, __FUNCTION__, __FILE__, __LINE__);*/
      }
   }
}

void hypre_ParCSRPersistentCommHandleWait( hypre_ParCSRPersistentCommHandle *comm_handle )
{
   if (hypre_ParCSRCommHandleNumRequests(comm_handle) > 0)
   {
      HYPRE_Int ret = hypre_MPI_Waitall(hypre_ParCSRCommHandleNumRequests(comm_handle),
                                        hypre_ParCSRCommHandleRequests(comm_handle),
                                        hypre_MPI_STATUSES_IGNORE);
      if (hypre_MPI_SUCCESS != ret)
      {
         hypre_error_w_msg(HYPRE_ERROR_GENERIC,"MPI error\n");
         /*hypre_printf("MPI error %d in %s (%s, line %u)\n", ret, __FUNCTION__, __FILE__, __LINE__);*/
      }
   }
}
#endif // HYPRE_USING_PERSISTENT_COMM

hypre_ParCSRCommHandle *
hypre_ParCSRCommHandleCreate ( HYPRE_Int            job,
                               hypre_ParCSRCommPkg *comm_pkg,
                               void                *send_data, 
                               void                *recv_data )
{
   HYPRE_Int                  num_sends = hypre_ParCSRCommPkgNumSends(comm_pkg);
   HYPRE_Int                  num_recvs = hypre_ParCSRCommPkgNumRecvs(comm_pkg);
   MPI_Comm                   comm      = hypre_ParCSRCommPkgComm(comm_pkg);

   hypre_ParCSRCommHandle    *comm_handle;
   HYPRE_Int                  num_requests;
   hypre_MPI_Request         *requests;

   HYPRE_Int                  i, j;
   HYPRE_Int                  my_id, num_procs;
   HYPRE_Int                  ip, vec_start, vec_len;
                  
   /*--------------------------------------------------------------------
    * hypre_Initialize sets up a communication handle,
    * posts receives and initiates sends. It always requires num_sends, 
    * num_recvs, recv_procs and send_procs to be set in comm_pkg.
    * There are different options for job:
    * job = 1 : is used to initialize communication exchange for the parts
    *           of vector needed to perform a Matvec,  it requires send_data 
    *           and recv_data to be doubles, recv_vec_starts and 
    *           send_map_starts need to be set in comm_pkg.
    * job = 2 : is used to initialize communication exchange for the parts
    *           of vector needed to perform a MatvecT,  it requires send_data 
    *           and recv_data to be doubles, recv_vec_starts and 
    *           send_map_starts need to be set in comm_pkg.
    * job = 11: similar to job = 1, but exchanges data of type HYPRE_Int (not HYPRE_Complex),
    *           requires send_data and recv_data to be ints
    *           recv_vec_starts and send_map_starts need to be set in comm_pkg.
    * job = 12: similar to job = 1, but exchanges data of type HYPRE_Int (not HYPRE_Complex),
    *           requires send_data and recv_data to be ints
    *           recv_vec_starts and send_map_starts need to be set in comm_pkg.
    * default: ignores send_data and recv_data, requires send_mpi_types
    *           and recv_mpi_types to be set in comm_pkg.
    *           datatypes need to point to absolute
    *           addresses, e.g. generated using hypre_MPI_Address . 
    *--------------------------------------------------------------------*/

   num_requests = num_sends + num_recvs;
   requests = hypre_CTAlloc(hypre_MPI_Request, num_requests);
 
   hypre_MPI_Comm_size(comm,&num_procs);
   hypre_MPI_Comm_rank(comm,&my_id);

   j = 0;
   switch (job)
   {
      case  1:
      {
         HYPRE_Complex *d_send_data = (HYPRE_Complex *) send_data;
         HYPRE_Complex *d_recv_data = (HYPRE_Complex *) recv_data;
         for (i = 0; i < num_recvs; i++)
         {
            ip = hypre_ParCSRCommPkgRecvProc(comm_pkg, i); 
            vec_start = hypre_ParCSRCommPkgRecvVecStart(comm_pkg,i);
            vec_len = hypre_ParCSRCommPkgRecvVecStart(comm_pkg,i+1)-vec_start;
            hypre_MPI_Irecv(&d_recv_data[vec_start], vec_len, HYPRE_MPI_COMPLEX,
                            ip, 0, comm, &requests[j++]);
         }
         for (i = 0; i < num_sends; i++)
         {
            vec_start = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i);
            vec_len = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i+1)-vec_start;
            ip = hypre_ParCSRCommPkgSendProc(comm_pkg, i); 
            hypre_MPI_Isend(&d_send_data[vec_start], vec_len, HYPRE_MPI_COMPLEX,
                            ip, 0, comm, &requests[j++]);
         }
         break;
      }
      case  2:
      {
         HYPRE_Complex *d_send_data = (HYPRE_Complex *) send_data;
         HYPRE_Complex *d_recv_data = (HYPRE_Complex *) recv_data;
         for (i = 0; i < num_sends; i++)
         {
            vec_start = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i);
            vec_len = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i+1) - vec_start;
            ip = hypre_ParCSRCommPkgSendProc(comm_pkg, i); 
            hypre_MPI_Irecv(&d_recv_data[vec_start], vec_len, HYPRE_MPI_COMPLEX,
                            ip, 0, comm, &requests[j++]);
         }
         for (i = 0; i < num_recvs; i++)
         {
            ip = hypre_ParCSRCommPkgRecvProc(comm_pkg, i); 
            vec_start = hypre_ParCSRCommPkgRecvVecStart(comm_pkg,i);
            vec_len = hypre_ParCSRCommPkgRecvVecStart(comm_pkg,i+1)-vec_start;
            hypre_MPI_Isend(&d_send_data[vec_start], vec_len, HYPRE_MPI_COMPLEX,
                            ip, 0, comm, &requests[j++]);
         }
         break;
      }
      case  11:
      {
         HYPRE_Int *i_send_data = (HYPRE_Int *) send_data;
         HYPRE_Int *i_recv_data = (HYPRE_Int *) recv_data;
         for (i = 0; i < num_recvs; i++)
         {
            ip = hypre_ParCSRCommPkgRecvProc(comm_pkg, i); 
            vec_start = hypre_ParCSRCommPkgRecvVecStart(comm_pkg,i);
            vec_len = hypre_ParCSRCommPkgRecvVecStart(comm_pkg,i+1)-vec_start;
            hypre_MPI_Irecv(&i_recv_data[vec_start], vec_len, HYPRE_MPI_INT,
                            ip, 0, comm, &requests[j++]);
         }
         for (i = 0; i < num_sends; i++)
         {
            vec_start = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i);
            vec_len = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i+1)-vec_start;
            ip = hypre_ParCSRCommPkgSendProc(comm_pkg, i); 
            hypre_MPI_Isend(&i_send_data[vec_start], vec_len, HYPRE_MPI_INT,
                            ip, 0, comm, &requests[j++]);
         }
         break;
      }
      case  12:
      {
         HYPRE_Int *i_send_data = (HYPRE_Int *) send_data;
         HYPRE_Int *i_recv_data = (HYPRE_Int *) recv_data;
         for (i = 0; i < num_sends; i++)
         {
            vec_start = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i);
            vec_len = hypre_ParCSRCommPkgSendMapStart(comm_pkg, i+1) - vec_start;
            ip = hypre_ParCSRCommPkgSendProc(comm_pkg, i); 
            hypre_MPI_Irecv(&i_recv_data[vec_start], vec_len, HYPRE_MPI_INT,
                            ip, 0, comm, &requests[j++]);
         }
         for (i = 0; i < num_recvs; i++)
         {
            ip = hypre_ParCSRCommPkgRecvProc(comm_pkg, i); 
            vec_start = hypre_ParCSRCommPkgRecvVecStart(comm_pkg,i);
            vec_len = hypre_ParCSRCommPkgRecvVecStart(comm_pkg,i+1)-vec_start;
            hypre_MPI_Isend(&i_send_data[vec_start], vec_len, HYPRE_MPI_INT,
                            ip, 0, comm, &requests[j++]);
         }
         break;
      }
   }
   /*--------------------------------------------------------------------
    * set up comm_handle and return
    *--------------------------------------------------------------------*/

   comm_handle = hypre_CTAlloc(hypre_ParCSRCommHandle, 1);

   hypre_ParCSRCommHandleCommPkg(comm_handle)     = comm_pkg;
   hypre_ParCSRCommHandleSendData(comm_handle)    = send_data;
   hypre_ParCSRCommHandleRecvData(comm_handle)    = recv_data;
   hypre_ParCSRCommHandleNumRequests(comm_handle) = num_requests;
   hypre_ParCSRCommHandleRequests(comm_handle)    = requests;

   return ( comm_handle );
}

HYPRE_Int
hypre_ParCSRCommHandleDestroy( hypre_ParCSRCommHandle *comm_handle )
{
   hypre_MPI_Status          *status0;

   if ( comm_handle==NULL ) return hypre_error_flag;
   if (hypre_ParCSRCommHandleNumRequests(comm_handle))
   {
      status0 = hypre_CTAlloc(hypre_MPI_Status,
                              hypre_ParCSRCommHandleNumRequests(comm_handle));
      hypre_MPI_Waitall(hypre_ParCSRCommHandleNumRequests(comm_handle),
                        hypre_ParCSRCommHandleRequests(comm_handle), status0);
      hypre_TFree(status0);
   }

   hypre_TFree(hypre_ParCSRCommHandleRequests(comm_handle));
   hypre_TFree(comm_handle);

   return hypre_error_flag;
}


/* hypre_MatCommPkgCreate_core does all the communications and computations for
   hypre_MatCommPkgCreate ( hypre_ParCSRMatrix *A) and
   hypre_BoolMatCommPkgCreate ( hypre_ParCSRBooleanMatrix *A) To support both
   data types, it has hardly any data structures other than HYPRE_Int*.
*/

void
hypre_MatvecCommPkgCreate_core(
   /* input args: */
   MPI_Comm comm,
   HYPRE_Int * col_map_offd,
   HYPRE_Int first_col_diag,
   HYPRE_Int * col_starts,
   HYPRE_Int num_cols_diag,
   HYPRE_Int num_cols_offd,
   HYPRE_Int firstColDiag,
   HYPRE_Int * colMapOffd,
   HYPRE_Int data,  /* = 1 for a matrix with floating-point data,
                       = 0 for Boolean matrix */
   /* pointers to output args: */
   HYPRE_Int * p_num_recvs,
   HYPRE_Int ** p_recv_procs,
   HYPRE_Int ** p_recv_vec_starts,
   HYPRE_Int * p_num_sends,
   HYPRE_Int ** p_send_procs,
   HYPRE_Int ** p_send_map_starts,
   HYPRE_Int ** p_send_map_elmts
   )
{
   HYPRE_Int    i, j;
   HYPRE_Int    num_procs, my_id, proc_num, num_elmts;
   HYPRE_Int    local_info, offd_col;
   HYPRE_Int    *proc_mark, *proc_add, *tmp, *recv_buf, *displs, *info;
   /* outputs: */
   HYPRE_Int  num_recvs, * recv_procs, * recv_vec_starts;
   HYPRE_Int  num_sends, * send_procs, * send_map_starts, * send_map_elmts;
   HYPRE_Int  ip, vec_start, vec_len, num_requests;

   hypre_MPI_Request *requests;
   hypre_MPI_Status *status; 

   hypre_MPI_Comm_size(comm, &num_procs);  
   hypre_MPI_Comm_rank(comm, &my_id);

   proc_mark = hypre_CTAlloc(HYPRE_Int, num_procs);
   proc_add = hypre_CTAlloc(HYPRE_Int, num_procs);
   info = hypre_CTAlloc(HYPRE_Int, num_procs);

   /* ----------------------------------------------------------------------
    * determine which processors to receive from (set proc_mark) and num_recvs,
    * at the end of the loop proc_mark[i] contains the number of elements to be
    * received from Proc. i
    * ---------------------------------------------------------------------*/

   for (i=0; i < num_procs; i++)
      proc_add[i] = 0;

   proc_num = 0;
   if (num_cols_offd) offd_col = col_map_offd[0];
   num_recvs=0;
   j = 0;
   for (i=0; i < num_cols_offd; i++)
   {
      if (num_cols_diag) proc_num = hypre_min(num_procs-1,offd_col / 
                                              num_cols_diag);
      while (col_starts[proc_num] > offd_col )
         proc_num = proc_num-1;
      while (col_starts[proc_num+1]-1 < offd_col )
         proc_num = proc_num+1;
      proc_mark[num_recvs] = proc_num;
      j = i;
      while (col_starts[proc_num+1] > offd_col)
      {
         proc_add[num_recvs]++;
         if (j < num_cols_offd-1) 
         {
            j++;
            offd_col = col_map_offd[j];
         }
         else
         {
            j++;
            offd_col = col_starts[num_procs];
         }
      }
      num_recvs++;
      if (j < num_cols_offd) i = j-1;
      else i=j;
   }

   local_info = 2*num_recvs;
                        
   hypre_MPI_Allgather(&local_info, 1, HYPRE_MPI_INT, info, 1, HYPRE_MPI_INT, comm); 

   /* ----------------------------------------------------------------------
    * generate information to be sent: tmp contains for each recv_proc:
    * id of recv_procs, number of elements to be received for this processor,
    * indices of elements (in this order)
    * ---------------------------------------------------------------------*/

   displs = hypre_CTAlloc(HYPRE_Int, num_procs+1);
   displs[0] = 0;
   for (i=1; i < num_procs+1; i++)
      displs[i] = displs[i-1]+info[i-1]; 
   recv_buf = hypre_CTAlloc(HYPRE_Int, displs[num_procs]); 

   recv_procs = NULL;
   tmp = NULL;
   if (num_recvs)
   {
      recv_procs = hypre_CTAlloc(HYPRE_Int, num_recvs);
      tmp = hypre_CTAlloc(HYPRE_Int, local_info);
   }
   recv_vec_starts = hypre_CTAlloc(HYPRE_Int, num_recvs+1);

   j = 0;
   if (num_recvs) recv_vec_starts[0] = 0;
   for (i=0; i < num_recvs; i++)
   {
      num_elmts = proc_add[i];
      recv_procs[i] = proc_mark[i];
      recv_vec_starts[i+1] = recv_vec_starts[i]+num_elmts;
      tmp[j++] = proc_mark[i];
      tmp[j++] = num_elmts;
   }

   hypre_MPI_Allgatherv(tmp,local_info,HYPRE_MPI_INT,recv_buf,info,
                        displs,HYPRE_MPI_INT,comm);

   /* ----------------------------------------------------------------------
    * determine num_sends and number of elements to be sent
    * ---------------------------------------------------------------------*/

   num_sends = 0;
   num_elmts = 0;
   proc_add[0] = 0;
   for (i=0; i < num_procs; i++)
   {
      j = displs[i];
      while ( j < displs[i+1])
      {
         if (recv_buf[j++] == my_id)
         {
            proc_mark[num_sends] = i;
            num_sends++;
            proc_add[num_sends] = proc_add[num_sends-1]+recv_buf[j];
            break;
         }
         j++;
      } 
   }
                
   /* ----------------------------------------------------------------------
    * determine send_procs and actual elements to be send (in send_map_elmts)
    * and send_map_starts whose i-th entry points to the beginning of the 
    * elements to be send to proc. i
    * ---------------------------------------------------------------------*/

   send_procs = NULL;
   send_map_elmts = NULL;

   if (num_sends)
   {
      send_procs = hypre_CTAlloc(HYPRE_Int, num_sends);
      send_map_elmts = hypre_CTAlloc(HYPRE_Int, proc_add[num_sends]);
   }
   send_map_starts = hypre_CTAlloc(HYPRE_Int, num_sends+1);
   num_requests = num_recvs+num_sends;
   if (num_requests)
   {
      requests = hypre_CTAlloc(hypre_MPI_Request, num_requests);
      status = hypre_CTAlloc(hypre_MPI_Status, num_requests);
   }

   if (num_sends) send_map_starts[0] = 0;
   for (i=0; i < num_sends; i++)
   {
      send_map_starts[i+1] = proc_add[i+1];
      send_procs[i] = proc_mark[i];
   }

   j=0;
   for (i=0; i < num_sends; i++)
   {
      vec_start = send_map_starts[i];
      vec_len = send_map_starts[i+1] - vec_start;
      ip = send_procs[i];
      hypre_MPI_Irecv(&send_map_elmts[vec_start], vec_len, HYPRE_MPI_INT,
                      ip, 0, comm, &requests[j++]);
   }
   for (i=0; i < num_recvs; i++)
   {
      vec_start = recv_vec_starts[i];
      vec_len = recv_vec_starts[i+1] - vec_start;
      ip = recv_procs[i];
      hypre_MPI_Isend(&col_map_offd[vec_start], vec_len, HYPRE_MPI_INT,
                      ip, 0, comm, &requests[j++]);
   }

   if (num_requests)
   {
      hypre_MPI_Waitall(num_requests, requests, status);
      hypre_TFree(requests);
      hypre_TFree(status);
   }

   if (num_sends)
   {
      for (i=0; i<send_map_starts[num_sends]; i++)
         send_map_elmts[i] -= first_col_diag;
   }

   hypre_TFree(proc_add);
   hypre_TFree(proc_mark); 
   hypre_TFree(tmp);
   hypre_TFree(recv_buf);
   hypre_TFree(displs);
   hypre_TFree(info);
 
   /* finish up with the hand-coded call-by-reference... */
   *p_num_recvs = num_recvs;
   *p_recv_procs = recv_procs;
   *p_recv_vec_starts = recv_vec_starts;
   *p_num_sends = num_sends;
   *p_send_procs = send_procs;
   *p_send_map_starts = send_map_starts;
   *p_send_map_elmts = send_map_elmts;
}

/* ----------------------------------------------------------------------
 * hypre_MatvecCommPkgCreate
 * generates the comm_pkg for A 
 * if no row and/or column partitioning is given, the routine determines
 * them with MPE_Decomp1d 
 * ---------------------------------------------------------------------*/

HYPRE_Int
hypre_MatvecCommPkgCreate ( hypre_ParCSRMatrix *A )
{
   HYPRE_Int            num_sends = 0;
   HYPRE_Int           *send_procs = NULL;
   HYPRE_Int           *send_map_starts = NULL;
   HYPRE_Int           *send_map_elmts = NULL;
 
   HYPRE_Int            num_recvs = 0;
   HYPRE_Int           *recv_procs = NULL;
   HYPRE_Int           *recv_vec_starts = NULL;
   
   MPI_Comm             comm = hypre_ParCSRMatrixComm(A);
   hypre_ParCSRCommPkg *comm_pkg;

   HYPRE_Int    first_col_diag = hypre_ParCSRMatrixFirstColDiag(A);

   HYPRE_Int   *col_map_offd = hypre_ParCSRMatrixColMapOffd(A);

   HYPRE_Int    num_cols_offd = hypre_CSRMatrixNumCols(hypre_ParCSRMatrixOffd(A));
 
   HYPRE_Int    num_procs;

   hypre_MPI_Comm_size(comm,&num_procs);

   if (num_procs > 1)
   {
#ifdef HYPRE_NO_GLOBAL_PARTITION

      HYPRE_Int             row_start=0, row_end=0, col_start = 0, col_end = 0;
      HYPRE_Int             global_num_cols;
      hypre_IJAssumedPart  *apart;
   
   /*-----------------------------------------------------------
    * get parcsr_A information 
    *----------------------------------------------------------*/

      hypre_ParCSRMatrixGetLocalRange(A, &row_start, &row_end, &col_start, &col_end);

      global_num_cols = hypre_ParCSRMatrixGlobalNumCols(A); 

      /* Create the assumed partition */
      if  (hypre_ParCSRMatrixAssumedPartition(A) == NULL)
      {
         hypre_ParCSRMatrixCreateAssumedPartition(A);
      }

      apart = hypre_ParCSRMatrixAssumedPartition(A);
   
      /*-----------------------------------------------------------
       * get commpkg info information 
       *----------------------------------------------------------*/

      hypre_NewCommPkgCreate_core( comm, col_map_offd, first_col_diag, 
                                col_start, col_end, 
                                num_cols_offd, global_num_cols,
                                &num_recvs, &recv_procs, &recv_vec_starts,
                                &num_sends, &send_procs, &send_map_starts, 
                                &send_map_elmts, apart);

#else
   
      HYPRE_Int  *col_starts = hypre_ParCSRMatrixColStarts(A);
      HYPRE_Int    num_cols_diag = hypre_CSRMatrixNumCols(hypre_ParCSRMatrixDiag(A));

      hypre_MatvecCommPkgCreate_core
      ( comm, col_map_offd, first_col_diag, col_starts,
        num_cols_diag, num_cols_offd,
        first_col_diag, col_map_offd,
        1,
        &num_recvs, &recv_procs, &recv_vec_starts,
        &num_sends, &send_procs, &send_map_starts,
        &send_map_elmts );

#endif

   /*-----------------------------------------------------------
    * setup commpkg
    *----------------------------------------------------------*/
   }
   else
   {
      send_map_starts = hypre_CTAlloc(HYPRE_Int,1);
      recv_vec_starts = hypre_CTAlloc(HYPRE_Int,1);
   }
   comm_pkg = hypre_CTAlloc(hypre_ParCSRCommPkg, 1);

   hypre_ParCSRCommPkgComm(comm_pkg) = comm;

   hypre_ParCSRCommPkgNumRecvs(comm_pkg) = num_recvs;
   hypre_ParCSRCommPkgRecvProcs(comm_pkg) = recv_procs;
   hypre_ParCSRCommPkgRecvVecStarts(comm_pkg) = recv_vec_starts;
   hypre_ParCSRCommPkgNumSends(comm_pkg) = num_sends;
   hypre_ParCSRCommPkgSendProcs(comm_pkg) = send_procs;
   hypre_ParCSRCommPkgSendMapStarts(comm_pkg) = send_map_starts;
   hypre_ParCSRCommPkgSendMapElmts(comm_pkg) = send_map_elmts;

   hypre_ParCSRMatrixCommPkg(A) = comm_pkg;

   return hypre_error_flag;
}


HYPRE_Int
hypre_MatvecCommPkgDestroy( hypre_ParCSRCommPkg *comm_pkg )
{
#ifdef HYPRE_USING_PERSISTENT_COMM
   HYPRE_Int i;
   for (i = HYPRE_COMM_PKG_JOB_COMPLEX; i < NUM_OF_COMM_PKG_JOB_TYPE; ++i)
   {
      if (comm_pkg->persistent_comm_handles[i])
      {
         hypre_ParCSRPersistentCommHandleDestroy(comm_pkg->persistent_comm_handles[i]);
      }
   }
#endif

   if (hypre_ParCSRCommPkgNumSends(comm_pkg))
   {
      hypre_TFree(hypre_ParCSRCommPkgSendProcs(comm_pkg));
      hypre_TFree(hypre_ParCSRCommPkgSendMapElmts(comm_pkg));
   }
   hypre_TFree(hypre_ParCSRCommPkgSendMapStarts(comm_pkg));
   /* if (hypre_ParCSRCommPkgSendMPITypes(comm_pkg))
      hypre_TFree(hypre_ParCSRCommPkgSendMPITypes(comm_pkg)); */ 
   if (hypre_ParCSRCommPkgNumRecvs(comm_pkg))
   {
      hypre_TFree(hypre_ParCSRCommPkgRecvProcs(comm_pkg));
   }
   hypre_TFree(hypre_ParCSRCommPkgRecvVecStarts(comm_pkg));
   /* if (hypre_ParCSRCommPkgRecvMPITypes(comm_pkg))
      hypre_TFree(hypre_ParCSRCommPkgRecvMPITypes(comm_pkg)); */
   hypre_TFree(comm_pkg);

   return hypre_error_flag;
}

HYPRE_Int
hypre_BuildCSRMatrixMPIDataType( HYPRE_Int num_nonzeros,
                                 HYPRE_Int num_rows,
                                 HYPRE_Complex *a_data,
                                 HYPRE_Int *a_i,
                                 HYPRE_Int *a_j, 
                                 hypre_MPI_Datatype *csr_matrix_datatype )
{
   HYPRE_Int            block_lens[3];
   hypre_MPI_Aint       displ[3];
   hypre_MPI_Datatype   types[3];

   block_lens[0] = num_nonzeros;
   block_lens[1] = num_rows+1;
   block_lens[2] = num_nonzeros;

   types[0] = HYPRE_MPI_COMPLEX;
   types[1] = HYPRE_MPI_INT;
   types[2] = HYPRE_MPI_INT;

   hypre_MPI_Address(a_data, &displ[0]);
   hypre_MPI_Address(a_i, &displ[1]);
   hypre_MPI_Address(a_j, &displ[2]);
   hypre_MPI_Type_struct(3,block_lens,displ,types,csr_matrix_datatype);
   hypre_MPI_Type_commit(csr_matrix_datatype);

   return hypre_error_flag;
}

HYPRE_Int
hypre_BuildCSRJDataType( HYPRE_Int num_nonzeros,
                         HYPRE_Complex *a_data,
                         HYPRE_Int *a_j,
                         hypre_MPI_Datatype *csr_jdata_datatype )
{
   HYPRE_Int          block_lens[2];
   hypre_MPI_Aint     displs[2];
   hypre_MPI_Datatype types[2];
 
   block_lens[0] = num_nonzeros;
   block_lens[1] = num_nonzeros;
 
   types[0] = HYPRE_MPI_COMPLEX;
   types[1] = HYPRE_MPI_INT;
 
   hypre_MPI_Address(a_data, &displs[0]);
   hypre_MPI_Address(a_j, &displs[1]);
 
   hypre_MPI_Type_struct(2,block_lens,displs,types,csr_jdata_datatype);
   hypre_MPI_Type_commit(csr_jdata_datatype);
 
   return hypre_error_flag;
}

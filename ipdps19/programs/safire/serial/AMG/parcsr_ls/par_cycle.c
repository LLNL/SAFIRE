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
 * ParAMG cycling routine
 *
 *****************************************************************************/

#include "_hypre_parcsr_ls.h"
#include "par_amg.h"

#ifdef HYPRE_USING_CALIPER
#include <caliper/cali.h>
#endif

/*--------------------------------------------------------------------------
 * hypre_BoomerAMGCycle
 *--------------------------------------------------------------------------*/

HYPRE_Int
hypre_BoomerAMGCycle( void              *amg_vdata,
                   hypre_ParVector  **F_array,
                   hypre_ParVector  **U_array   )
{
   hypre_ParAMGData *amg_data = (hypre_ParAMGData*) amg_vdata;

   /* Data Structure variables */

   hypre_ParCSRMatrix    **A_array;
   hypre_ParCSRMatrix    **P_array;
   hypre_ParCSRMatrix    **R_array;
   hypre_ParVector    *Utemp;
   hypre_ParVector    *Vtemp;
   hypre_ParVector    *Rtemp;
   hypre_ParVector    *Ptemp;
   hypre_ParVector    *Ztemp;
   hypre_ParVector    *Aux_U;
   hypre_ParVector    *Aux_F;

   HYPRE_Real   *Ztemp_data;
   HYPRE_Real   *Ptemp_data;
   HYPRE_Int     **CF_marker_array;
   /* HYPRE_Int     **unknown_map_array;
   HYPRE_Int     **point_map_array;
   HYPRE_Int     **v_at_point_array; */

   HYPRE_Real    cycle_op_count;
   HYPRE_Int       cycle_type;
   HYPRE_Int       num_levels;
   HYPRE_Int       max_levels;

   HYPRE_Real   *num_coeffs;
   HYPRE_Int      *num_grid_sweeps;
   HYPRE_Int      *grid_relax_type;
   HYPRE_Int     **grid_relax_points;

   HYPRE_Int      cheby_order;

 /* Local variables  */
   HYPRE_Int      *lev_counter;
   HYPRE_Int       Solve_err_flag;
   HYPRE_Int       k;
   HYPRE_Int       i, j, jj;
   HYPRE_Int       level;
   HYPRE_Int       cycle_param;
   HYPRE_Int       coarse_grid;
   HYPRE_Int       fine_grid;
   HYPRE_Int       Not_Finished;
   HYPRE_Int       num_sweep;
   HYPRE_Int       cg_num_sweep = 1;
   HYPRE_Int       relax_type;
   HYPRE_Int       relax_points;
   HYPRE_Int       relax_order;
   HYPRE_Int       relax_local;
   HYPRE_Int       old_version = 0;
   HYPRE_Real   *relax_weight;
   HYPRE_Real   *omega;
   HYPRE_Real    alfa, beta, gammaold;
   HYPRE_Real    gamma = 1.0;
   HYPRE_Int       local_size;
   HYPRE_Int       num_threads, my_id;

   HYPRE_Real    alpha;
   HYPRE_Real  **l1_norms = NULL;
   HYPRE_Real   *l1_norms_level;
   HYPRE_Real   **ds = hypre_ParAMGDataChebyDS(amg_data);
   HYPRE_Real   **coefs = hypre_ParAMGDataChebyCoefs(amg_data);

   HYPRE_Int seq_cg = 0;

   MPI_Comm comm;

#if 0
   HYPRE_Real   *D_mat;
   HYPRE_Real   *S_vec;
#endif

#ifdef HYPRE_USING_CALIPER
   cali_id_t iter_attr =
     cali_create_attribute("hypre.par_cycle.level", CALI_TYPE_INT, CALI_ATTR_DEFAULT);
#endif
   
   /* Acquire data and allocate storage */

   num_threads = hypre_NumThreads();

   A_array           = hypre_ParAMGDataAArray(amg_data);
   P_array           = hypre_ParAMGDataPArray(amg_data);
   R_array           = hypre_ParAMGDataRArray(amg_data);
   CF_marker_array   = hypre_ParAMGDataCFMarkerArray(amg_data);
   Vtemp             = hypre_ParAMGDataVtemp(amg_data);
   Rtemp             = hypre_ParAMGDataRtemp(amg_data);
   Ptemp             = hypre_ParAMGDataPtemp(amg_data);
   Ztemp             = hypre_ParAMGDataZtemp(amg_data);
   num_levels        = hypre_ParAMGDataNumLevels(amg_data);
   max_levels        = hypre_ParAMGDataMaxLevels(amg_data);
   cycle_type        = hypre_ParAMGDataCycleType(amg_data);

   num_grid_sweeps     = hypre_ParAMGDataNumGridSweeps(amg_data);
   grid_relax_type     = hypre_ParAMGDataGridRelaxType(amg_data);
   grid_relax_points   = hypre_ParAMGDataGridRelaxPoints(amg_data);
   relax_order         = hypre_ParAMGDataRelaxOrder(amg_data);
   relax_weight        = hypre_ParAMGDataRelaxWeight(amg_data);
   omega               = hypre_ParAMGDataOmega(amg_data);
   l1_norms            = hypre_ParAMGDataL1Norms(amg_data);

   /*max_eig_est = hypre_ParAMGDataMaxEigEst(amg_data);
   min_eig_est = hypre_ParAMGDataMinEigEst(amg_data);
   cheby_fraction = hypre_ParAMGDataChebyFraction(amg_data);*/
   cheby_order = hypre_ParAMGDataChebyOrder(amg_data);

   cycle_op_count = hypre_ParAMGDataCycleOpCount(amg_data);

   lev_counter = hypre_CTAlloc(HYPRE_Int, num_levels);

   if (hypre_ParAMGDataParticipate(amg_data)) seq_cg = 1;

   /* Initialize */

   Solve_err_flag = 0;

   if (grid_relax_points) old_version = 1;

   num_coeffs = hypre_CTAlloc(HYPRE_Real, num_levels);
   num_coeffs[0]    = hypre_ParCSRMatrixDNumNonzeros(A_array[0]);
   comm = hypre_ParCSRMatrixComm(A_array[0]);
   hypre_MPI_Comm_rank(comm,&my_id);

   for (j = 1; j < num_levels; j++)
      num_coeffs[j] = hypre_ParCSRMatrixDNumNonzeros(A_array[j]);

   /*---------------------------------------------------------------------
    *    Initialize cycling control counter
    *
    *     Cycling is controlled using a level counter: lev_counter[k]
    *
    *     Each time relaxation is performed on level k, the
    *     counter is decremented by 1. If the counter is then
    *     negative, we go to the next finer level. If non-
    *     negative, we go to the next coarser level. The
    *     following actions control cycling:
    *
    *     a. lev_counter[0] is initialized to 1.
    *     b. lev_counter[k] is initialized to cycle_type for k>0.
    *
    *     c. During cycling, when going down to level k, lev_counter[k]
    *        is set to the max of (lev_counter[k],cycle_type)
    *---------------------------------------------------------------------*/

   Not_Finished = 1;

   lev_counter[0] = 1;
   for (k = 1; k < num_levels; ++k)
   {
      lev_counter[k] = cycle_type;
   }

   level = 0;
   cycle_param = 1;

   /*---------------------------------------------------------------------
    * Main loop of cycling
    *--------------------------------------------------------------------*/

#ifdef HYPRE_USING_CALIPER
   cali_set_int(iter_attr, level);
#endif
   
   while (Not_Finished)
   {
      if (num_levels > 1)
      {
        local_size
            = hypre_VectorSize(hypre_ParVectorLocalVector(F_array[level]));
        hypre_VectorSize(hypre_ParVectorLocalVector(Vtemp)) = local_size;
        cg_num_sweep = 1;
	num_sweep = num_grid_sweeps[cycle_param];
        Aux_U = U_array[level];
        Aux_F = F_array[level];
        relax_type = grid_relax_type[cycle_param];
      }
      else /* AB: 4/08: removed the max_levels > 1 check - should do this when max-levels = 1 also */
      {
        /* If no coarsening occurred, apply a simple smoother once */
        Aux_U = U_array[level];
        Aux_F = F_array[level];
        num_sweep = 1;
        /* TK: Use the user relax type (instead of 0) to allow for setting a
           convergent smoother (e.g. in the solution of singular problems). */
        relax_type = hypre_ParAMGDataUserRelaxType(amg_data);
        if (relax_type == -1) relax_type = 6;
      }

      if (l1_norms != NULL)
         l1_norms_level = l1_norms[level];
      else
         l1_norms_level = NULL;

      if (cycle_param == 3 && seq_cg)
      {
         hypre_seqAMGCycle(amg_data, level, F_array, U_array);
      }
      else
      {

        /*------------------------------------------------------------------
         * Do the relaxation num_sweep times
         *-----------------------------------------------------------------*/
         for (jj = 0; jj < cg_num_sweep; jj++)
         {
           for (j = 0; j < num_sweep; j++)
           {
              if (num_levels == 1 && max_levels > 1)
              {
                 relax_points = 0;
                 relax_local = 0;
              }
              else
              {
                 if (old_version)
		    relax_points = grid_relax_points[cycle_param][j];
                 relax_local = relax_order;
              }

              /*-----------------------------------------------
               * VERY sloppy approximation to cycle complexity
               *-----------------------------------------------*/
              if (old_version && level < num_levels -1)
              {
                 switch (relax_points)
                 {
                    case 1:
                    cycle_op_count += num_coeffs[level+1];
                    break;

                    case -1:
                    cycle_op_count += (num_coeffs[level]-num_coeffs[level+1]);
                    break;
                 }
              }
	      else
              {
                 cycle_op_count += num_coeffs[level];
              }
              /*-----------------------------------------------
                Choose Smoother
                -----------------------------------------------*/

              if (relax_type == 9 || relax_type == 99)
              { /* Gaussian elimination */
                 hypre_GaussElimSolve(amg_data, level, relax_type);
              }
              else if (relax_type == 18)
              {   /* L1 - Jacobi*/
                 if (relax_order == 1 && cycle_param < 3)
                 {
                    /* need to do CF - so can't use the AMS one */
                    HYPRE_Int i;
                    HYPRE_Int loc_relax_points[2];
                    if (cycle_type < 2)
                    {
                       loc_relax_points[0] = 1;
                       loc_relax_points[1] = -1;
                    }
                    else
                    {
                       loc_relax_points[0] = -1;
                       loc_relax_points[1] = 1;
                    }
                    for (i=0; i < 2; i++)
                       hypre_ParCSRRelax_L1_Jacobi(A_array[level],
                                                 Aux_F,
                                                 CF_marker_array[level],
                                                 loc_relax_points[i],
                                                 relax_weight[level],
                                                 l1_norms[level],
                                                 Aux_U,
                                                 Vtemp);
                 }
                 else /* not CF - so use through AMS */
                 {
                    if (num_threads == 1)
                       hypre_ParCSRRelax(A_array[level],
                                       Aux_F,
                                       1,
                                       1,
                                       l1_norms_level,
                                       relax_weight[level],
                                       omega[level],0,0,0,0,
                                       Aux_U,
                                       Vtemp,
                                       Ztemp);

                    else
                       hypre_ParCSRRelaxThreads(A_array[level],
                                              Aux_F,
                                              1,
                                              1,
                                              l1_norms_level,
                                              relax_weight[level],
                                              omega[level],
                                              Aux_U,
                                              Vtemp,
                                              Ztemp);
                 }
              }
              else if (relax_type == 16)
              { /* scaled Chebyshev */
                 HYPRE_Int scale = hypre_ParAMGDataChebyScale(amg_data);
                 HYPRE_Int variant = hypre_ParAMGDataChebyVariant(amg_data);
                 hypre_ParCSRRelax_Cheby_Solve(A_array[level], Aux_F,
                                       ds[level], coefs[level],
                                       cheby_order, scale,
                                       variant, Aux_U, Vtemp, Ztemp );
              }
              else if (relax_type ==17)
              {
                 hypre_BoomerAMGRelax_FCFJacobi(A_array[level],
                                              Aux_F,
                                              CF_marker_array[level],
                                              relax_weight[level],
                                              Aux_U,
                                              Vtemp);
              }
	      else if (old_version)
	      {
                 Solve_err_flag = hypre_BoomerAMGRelax(A_array[level],
                                                     Aux_F,
                                                     CF_marker_array[level],
                                                     relax_type, relax_points,
                                                     relax_weight[level],
                                                     omega[level],
                                                     l1_norms_level,
                                                     Aux_U,
                                                     Vtemp,
                                                     Ztemp);
	      }
	      else
	      {
                 /* smoother than can have CF ordering */
                 Solve_err_flag = hypre_BoomerAMGRelaxIF(A_array[level],
                                                          Aux_F,
                                                          CF_marker_array[level],
                                                          relax_type,
                                                          relax_local,
                                                          cycle_param,
                                                          relax_weight[level],
                                                          omega[level],
                                                          l1_norms_level,
                                                          Aux_U,
                                                          Vtemp,
                                                          Ztemp);
	      }

              if (Solve_err_flag != 0)
                 return(Solve_err_flag);
           }
        }
      }

      /*------------------------------------------------------------------
       * Decrement the control counter and determine which grid to visit next
       *-----------------------------------------------------------------*/

      --lev_counter[level];

      if (lev_counter[level] >= 0 && level != num_levels-1)
      {

         /*---------------------------------------------------------------
          * Visit coarser level next.
 	  * Compute residual using hypre_ParCSRMatrixMatvec.
          * Perform restriction using hypre_ParCSRMatrixMatvecT.
          * Reset counters and cycling parameters for coarse level
          *--------------------------------------------------------------*/

         fine_grid = level;
         coarse_grid = level + 1;

         hypre_ParVectorSetConstantValues(U_array[coarse_grid], 0.0);

         alpha = -1.0;
         beta = 1.0;

         // JSP: avoid unnecessary copy using out-of-place version of SpMV
         hypre_ParCSRMatrixMatvecOutOfPlace(alpha, A_array[fine_grid], U_array[fine_grid],
                                               beta, F_array[fine_grid], Vtemp);

         alpha = 1.0;
         beta = 0.0;

         hypre_ParCSRMatrixMatvecT(alpha,R_array[fine_grid],Vtemp,
                                      beta,F_array[coarse_grid]);

         ++level;
         lev_counter[level] = hypre_max(lev_counter[level],cycle_type);
         cycle_param = 1;
         if (level == num_levels-1) cycle_param = 3;

#ifdef HYPRE_USING_CALIPER
         cali_set_int(iter_attr, level);  /* set the level for caliper here */
#endif
      }

      else if (level != 0)
      {
         /*---------------------------------------------------------------
          * Visit finer level next.
          * Interpolate and add correction using hypre_ParCSRMatrixMatvec.
          * Reset counters and cycling parameters for finer level.
          *--------------------------------------------------------------*/

         fine_grid = level - 1;
         coarse_grid = level;
         alpha = 1.0;
         beta = 1.0;
         hypre_ParCSRMatrixMatvec(alpha, P_array[fine_grid],
                                     U_array[coarse_grid],
                                     beta, U_array[fine_grid]);

         --level;
         cycle_param = 2;
         
#ifdef HYPRE_USING_CALIPER
         cali_set_int(iter_attr, level);  /* set the level for caliper here */
#endif
      }
      else
      {
         Not_Finished = 0;
      }
   }

#ifdef HYPRE_USING_CALIPER
   cali_end(iter_attr);  /* unset "iter" */
#endif
   
   hypre_ParAMGDataCycleOpCount(amg_data) = cycle_op_count;

   hypre_TFree(lev_counter);
   hypre_TFree(num_coeffs);
   return(Solve_err_flag);
}

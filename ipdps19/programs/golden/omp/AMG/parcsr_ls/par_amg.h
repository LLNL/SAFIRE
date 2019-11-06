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





#ifndef hypre_ParAMG_DATA_HEADER
#define hypre_ParAMG_DATA_HEADER

#define CUMNUMIT

/*#include "par_csr_block_matrix.h"*/


/*--------------------------------------------------------------------------
 * hypre_ParAMGData
 *--------------------------------------------------------------------------*/

typedef struct
{

   /* setup params */
   HYPRE_Int      max_levels;
   HYPRE_Real   strong_threshold;
   HYPRE_Real   max_row_sum;
   HYPRE_Real   trunc_factor;
   HYPRE_Real   agg_trunc_factor;
   HYPRE_Real   agg_P12_trunc_factor;
   HYPRE_Real   jacobi_trunc_threshold;
   HYPRE_Real   S_commpkg_switch;
   HYPRE_Int      measure_type;
   HYPRE_Int      setup_type;
   HYPRE_Int      coarsen_type;
   HYPRE_Int      P_max_elmts;
   HYPRE_Int      interp_type;
   HYPRE_Int      sep_weight;
   HYPRE_Int      agg_interp_type;
   HYPRE_Int      agg_P_max_elmts;
   HYPRE_Int      agg_P12_max_elmts;
   HYPRE_Int      restr_par;
   HYPRE_Int      agg_num_levels;
   HYPRE_Int      num_paths;
   HYPRE_Int      post_interp_type;
   HYPRE_Int      max_coarse_size;
   HYPRE_Int      min_coarse_size;
   HYPRE_Int      seq_threshold;
   HYPRE_Int      redundant;
   HYPRE_Int      participate;

   /* solve params */
   HYPRE_Int      max_iter;
   HYPRE_Int      min_iter;
   HYPRE_Int      cycle_type;    
   HYPRE_Int     *num_grid_sweeps;  
   HYPRE_Int     *grid_relax_type;   
   HYPRE_Int    **grid_relax_points;
   HYPRE_Int      relax_order;
   HYPRE_Int      user_coarse_relax_type;   
   HYPRE_Int      user_relax_type;   
   HYPRE_Int      user_num_sweeps;   
   HYPRE_Real     user_relax_weight;   
   HYPRE_Real     outer_wt;
   HYPRE_Real  *relax_weight; 
   HYPRE_Real  *omega;
   HYPRE_Real   tol;

   /* problem data */
   hypre_ParCSRMatrix  *A;
   HYPRE_Int      num_variables;
   HYPRE_Int      num_functions;
   HYPRE_Int      nodal;
   HYPRE_Int      nodal_levels;
   HYPRE_Int      nodal_diag;
   HYPRE_Int      num_points;
   HYPRE_Int     *dof_func;
   HYPRE_Int     *dof_point;           
   HYPRE_Int     *point_dof_map;

   /* data generated in the setup phase */
   hypre_ParCSRMatrix **A_array;
   hypre_ParVector    **F_array;
   hypre_ParVector    **U_array;
   hypre_ParCSRMatrix **P_array;
   hypre_ParCSRMatrix **R_array;
   HYPRE_Int                **CF_marker_array;
   HYPRE_Int                **dof_func_array;
   HYPRE_Int                **dof_point_array;
   HYPRE_Int                **point_dof_map_array;
   HYPRE_Int                  num_levels;
   HYPRE_Real         **l1_norms;

   HYPRE_Int block_mode;

   HYPRE_Real          *max_eig_est;
   HYPRE_Real          *min_eig_est;
   HYPRE_Int            cheby_eig_est;
   HYPRE_Int            cheby_order;
   HYPRE_Int            cheby_variant;
   HYPRE_Int            cheby_scale;
   HYPRE_Real           cheby_fraction;
   HYPRE_Real         **cheby_ds;
   HYPRE_Real         **cheby_coefs;

   /* data needed for non-Galerkin option */
   HYPRE_Int           nongalerk_num_tol;
   HYPRE_Real         *nongalerk_tol;
   HYPRE_Real          nongalerkin_tol;
   HYPRE_Real         *nongal_tol_array;

   /* data generated in the solve phase */
   hypre_ParVector   *Vtemp;
   hypre_Vector      *Vtemp_local;
   HYPRE_Real        *Vtemp_local_data;
   HYPRE_Real         cycle_op_count;
   hypre_ParVector   *Rtemp;
   hypre_ParVector   *Ptemp;
   hypre_ParVector   *Ztemp;

   /* log info */
   HYPRE_Int      logging;
   HYPRE_Int      num_iterations;
#ifdef CUMNUMIT
   HYPRE_Int      cum_num_iterations;
#endif
   HYPRE_Real   cum_nnz_A_P;
   HYPRE_Real   rel_resid_norm;
   hypre_ParVector *residual; /* available if logging>1 */

   /* output params */
   HYPRE_Int      print_level;
   char     log_file_name[256];
   HYPRE_Int      debug_flag;

 /* enable redundant coarse grid solve */
   HYPRE_Solver   coarse_solver;
   hypre_ParCSRMatrix  *A_coarse;
   hypre_ParVector  *f_coarse;
   hypre_ParVector  *u_coarse;
   MPI_Comm   new_comm;

 /* store matrix, vector and communication info for Gaussian elimination */
   HYPRE_Real *A_mat;
   HYPRE_Real *b_vec;
   HYPRE_Int *comm_info;

 /* information for multiplication with Lambda - additive AMG */
   HYPRE_Int      additive;
   HYPRE_Int      mult_additive;
   HYPRE_Int      simple;
   HYPRE_Int      add_last_lvl;
   HYPRE_Int      add_P_max_elmts;
   HYPRE_Real     add_trunc_factor;
   HYPRE_Int      add_rlx_type;
   HYPRE_Real     add_rlx_wt;
   hypre_ParCSRMatrix *Lambda;
   hypre_ParCSRMatrix *Atilde;
   hypre_ParVector *Rtilde;
   hypre_ParVector *Xtilde;
   HYPRE_Real *D_inv;

 /* Use 2 mat-mat-muls instead of triple product*/
   HYPRE_Int rap2;

   HYPRE_Int keepTranspose;

} hypre_ParAMGData;

/*--------------------------------------------------------------------------
 * Accessor functions for the hypre_AMGData structure
 *--------------------------------------------------------------------------*/

/* setup params */
		  		      
#define hypre_ParAMGDataRestriction(amg_data) ((amg_data)->restr_par)
#define hypre_ParAMGDataMaxLevels(amg_data) ((amg_data)->max_levels)
#define hypre_ParAMGDataStrongThreshold(amg_data) \
((amg_data)->strong_threshold)
#define hypre_ParAMGDataMaxRowSum(amg_data) ((amg_data)->max_row_sum)
#define hypre_ParAMGDataTruncFactor(amg_data) ((amg_data)->trunc_factor)
#define hypre_ParAMGDataAggTruncFactor(amg_data) ((amg_data)->agg_trunc_factor)
#define hypre_ParAMGDataAggP12TruncFactor(amg_data) ((amg_data)->agg_P12_trunc_factor)
#define hypre_ParAMGDataJacobiTruncThreshold(amg_data) ((amg_data)->jacobi_trunc_threshold)
#define hypre_ParAMGDataSCommPkgSwitch(amg_data) ((amg_data)->S_commpkg_switch)
#define hypre_ParAMGDataInterpType(amg_data) ((amg_data)->interp_type)
#define hypre_ParAMGDataSepWeight(amg_data) ((amg_data)->sep_weight)
#define hypre_ParAMGDataAggInterpType(amg_data) ((amg_data)->agg_interp_type)
#define hypre_ParAMGDataCoarsenType(amg_data) ((amg_data)->coarsen_type)
#define hypre_ParAMGDataMeasureType(amg_data) ((amg_data)->measure_type)
#define hypre_ParAMGDataSetupType(amg_data) ((amg_data)->setup_type)
#define hypre_ParAMGDataPMaxElmts(amg_data) ((amg_data)->P_max_elmts)
#define hypre_ParAMGDataAggPMaxElmts(amg_data) ((amg_data)->agg_P_max_elmts)
#define hypre_ParAMGDataAggP12MaxElmts(amg_data) ((amg_data)->agg_P12_max_elmts)
#define hypre_ParAMGDataNumPaths(amg_data) ((amg_data)->num_paths)
#define hypre_ParAMGDataAggNumLevels(amg_data) ((amg_data)->agg_num_levels)
#define hypre_ParAMGDataPostInterpType(amg_data) ((amg_data)->post_interp_type)
#define hypre_ParAMGDataL1Norms(amg_data) ((amg_data)->l1_norms)
 #define hypre_ParAMGDataMaxCoarseSize(amg_data) ((amg_data)->max_coarse_size)
 #define hypre_ParAMGDataMinCoarseSize(amg_data) ((amg_data)->min_coarse_size)
 #define hypre_ParAMGDataSeqThreshold(amg_data) ((amg_data)->seq_threshold)

/* solve params */

#define hypre_ParAMGDataMinIter(amg_data) ((amg_data)->min_iter)
#define hypre_ParAMGDataMaxIter(amg_data) ((amg_data)->max_iter)
#define hypre_ParAMGDataCycleType(amg_data) ((amg_data)->cycle_type)
#define hypre_ParAMGDataTol(amg_data) ((amg_data)->tol)
#define hypre_ParAMGDataNumGridSweeps(amg_data) ((amg_data)->num_grid_sweeps)
#define hypre_ParAMGDataUserCoarseRelaxType(amg_data) ((amg_data)->user_coarse_relax_type)
#define hypre_ParAMGDataUserRelaxType(amg_data) ((amg_data)->user_relax_type)
#define hypre_ParAMGDataUserRelaxWeight(amg_data) ((amg_data)->user_relax_weight)
#define hypre_ParAMGDataUserNumSweeps(amg_data) ((amg_data)->user_num_sweeps)
#define hypre_ParAMGDataGridRelaxType(amg_data) ((amg_data)->grid_relax_type)
#define hypre_ParAMGDataGridRelaxPoints(amg_data) \
((amg_data)->grid_relax_points)
#define hypre_ParAMGDataRelaxOrder(amg_data) ((amg_data)->relax_order)
#define hypre_ParAMGDataRelaxWeight(amg_data) ((amg_data)->relax_weight)
#define hypre_ParAMGDataOmega(amg_data) ((amg_data)->omega)
#define hypre_ParAMGDataOuterWt(amg_data) ((amg_data)->outer_wt)

/* problem data parameters */
#define  hypre_ParAMGDataNumVariables(amg_data)  ((amg_data)->num_variables)
#define hypre_ParAMGDataNumFunctions(amg_data) ((amg_data)->num_functions)
#define hypre_ParAMGDataNodal(amg_data) ((amg_data)->nodal)
#define hypre_ParAMGDataNodalLevels(amg_data) ((amg_data)->nodal_levels)
#define hypre_ParAMGDataNodalDiag(amg_data) ((amg_data)->nodal_diag)
#define hypre_ParAMGDataNumPoints(amg_data) ((amg_data)->num_points)
#define hypre_ParAMGDataDofFunc(amg_data) ((amg_data)->dof_func)
#define hypre_ParAMGDataDofPoint(amg_data) ((amg_data)->dof_point)
#define hypre_ParAMGDataPointDofMap(amg_data) ((amg_data)->point_dof_map)

/* data generated by the setup phase */
#define hypre_ParAMGDataCFMarkerArray(amg_data) ((amg_data)-> CF_marker_array)
#define hypre_ParAMGDataAArray(amg_data) ((amg_data)->A_array)
#define hypre_ParAMGDataFArray(amg_data) ((amg_data)->F_array)
#define hypre_ParAMGDataUArray(amg_data) ((amg_data)->U_array)
#define hypre_ParAMGDataPArray(amg_data) ((amg_data)->P_array)
#define hypre_ParAMGDataRArray(amg_data) ((amg_data)->R_array)
#define hypre_ParAMGDataDofFuncArray(amg_data) ((amg_data)->dof_func_array)
#define hypre_ParAMGDataDofPointArray(amg_data) ((amg_data)->dof_point_array)
#define hypre_ParAMGDataPointDofMapArray(amg_data) \
((amg_data)->point_dof_map_array) 
#define hypre_ParAMGDataNumLevels(amg_data) ((amg_data)->num_levels)	

#define hypre_ParAMGDataMaxEigEst(amg_data) ((amg_data)->max_eig_est)	
#define hypre_ParAMGDataMinEigEst(amg_data) ((amg_data)->min_eig_est)	
#define hypre_ParAMGDataChebyEigEst(amg_data) ((amg_data)->cheby_eig_est)
#define hypre_ParAMGDataChebyVariant(amg_data) ((amg_data)->cheby_variant)
#define hypre_ParAMGDataChebyScale(amg_data) ((amg_data)->cheby_scale)
#define hypre_ParAMGDataChebyOrder(amg_data) ((amg_data)->cheby_order)
#define hypre_ParAMGDataChebyFraction(amg_data) ((amg_data)->cheby_fraction)
#define hypre_ParAMGDataChebyDS(amg_data) ((amg_data)->cheby_ds)
#define hypre_ParAMGDataChebyCoefs(amg_data) ((amg_data)->cheby_coefs)

#define hypre_ParAMGDataBlockMode(amg_data) ((amg_data)->block_mode)


/* data generated in the solve phase */
#define hypre_ParAMGDataVtemp(amg_data) ((amg_data)->Vtemp)
#define hypre_ParAMGDataVtempLocal(amg_data) ((amg_data)->Vtemp_local)
#define hypre_ParAMGDataVtemplocalData(amg_data) ((amg_data)->Vtemp_local_data)
#define hypre_ParAMGDataCycleOpCount(amg_data) ((amg_data)->cycle_op_count)
#define hypre_ParAMGDataRtemp(amg_data) ((amg_data)->Rtemp)
#define hypre_ParAMGDataPtemp(amg_data) ((amg_data)->Ptemp)
#define hypre_ParAMGDataZtemp(amg_data) ((amg_data)->Ztemp)

/* fields used by GSMG */
#define hypre_ParAMGDataGSMG(amg_data) ((amg_data)->gsmg)
#define hypre_ParAMGDataNumSamples(amg_data) ((amg_data)->num_samples)

/* log info data */
#define hypre_ParAMGDataLogging(amg_data) ((amg_data)->logging)
#define hypre_ParAMGDataNumIterations(amg_data) ((amg_data)->num_iterations)
#ifdef CUMNUMIT
#define hypre_ParAMGDataCumNumIterations(amg_data) ((amg_data)->cum_num_iterations)
#endif
#define hypre_ParAMGDataRelativeResidualNorm(amg_data) ((amg_data)->rel_resid_norm)
#define hypre_ParAMGDataResidual(amg_data) ((amg_data)->residual)

/* output parameters */
#define hypre_ParAMGDataPrintLevel(amg_data) ((amg_data)->print_level)
#define hypre_ParAMGDataLogFileName(amg_data) ((amg_data)->log_file_name)
#define hypre_ParAMGDataDebugFlag(amg_data)   ((amg_data)->debug_flag)
#define hypre_ParAMGDataCumNnzAP(amg_data)   ((amg_data)->cum_nnz_AP)

#define hypre_ParAMGDataCoarseSolver(amg_data) ((amg_data)->coarse_solver)
#define hypre_ParAMGDataACoarse(amg_data) ((amg_data)->A_coarse)
#define hypre_ParAMGDataFCoarse(amg_data) ((amg_data)->f_coarse)
#define hypre_ParAMGDataUCoarse(amg_data) ((amg_data)->u_coarse)
#define hypre_ParAMGDataNewComm(amg_data) ((amg_data)->new_comm)
#define hypre_ParAMGDataRedundant(amg_data) ((amg_data)->redundant)
#define hypre_ParAMGDataParticipate(amg_data) ((amg_data)->participate)

#define hypre_ParAMGDataAMat(amg_data) ((amg_data)->A_mat)
#define hypre_ParAMGDataBVec(amg_data) ((amg_data)->b_vec)
#define hypre_ParAMGDataCommInfo(amg_data) ((amg_data)->comm_info)

/* additive AMG parameters */
#define hypre_ParAMGDataAdditive(amg_data) ((amg_data)->additive)
#define hypre_ParAMGDataMultAdditive(amg_data) ((amg_data)->mult_additive)
#define hypre_ParAMGDataSimple(amg_data) ((amg_data)->simple)
#define hypre_ParAMGDataAddLastLvl(amg_data) ((amg_data)->add_last_lvl)
#define hypre_ParAMGDataMultAddPMaxElmts(amg_data) ((amg_data)->add_P_max_elmts)
#define hypre_ParAMGDataMultAddTruncFactor(amg_data) ((amg_data)->add_trunc_factor)
#define hypre_ParAMGDataAddRelaxType(amg_data) ((amg_data)->add_rlx_type)
#define hypre_ParAMGDataAddRelaxWt(amg_data) ((amg_data)->add_rlx_wt)
#define hypre_ParAMGDataLambda(amg_data) ((amg_data)->Lambda)
#define hypre_ParAMGDataAtilde(amg_data) ((amg_data)->Atilde)
#define hypre_ParAMGDataRtilde(amg_data) ((amg_data)->Rtilde)
#define hypre_ParAMGDataXtilde(amg_data) ((amg_data)->Xtilde)
#define hypre_ParAMGDataDinv(amg_data) ((amg_data)->D_inv)

/* non-Galerkin parameters */
#define hypre_ParAMGDataNonGalerkNumTol(amg_data) ((amg_data)->nongalerk_num_tol)
#define hypre_ParAMGDataNonGalerkTol(amg_data) ((amg_data)->nongalerk_tol)
#define hypre_ParAMGDataNonGalerkinTol(amg_data) ((amg_data)->nongalerkin_tol)
#define hypre_ParAMGDataNonGalTolArray(amg_data) ((amg_data)->nongal_tol_array)

#define hypre_ParAMGDataRAP2(amg_data) ((amg_data)->rap2)
#define hypre_ParAMGDataKeepTranspose(amg_data) ((amg_data)->keepTranspose)
#endif




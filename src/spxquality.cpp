/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*                  This file is part of the class library                   */
/*       SoPlex --- the Sequential object-oriented simPlex.                  */
/*                                                                           */
/*    Copyright (C) 1997-1999 Roland Wunderling                              */
/*                  1997-2002 Konrad-Zuse-Zentrum                            */
/*                            fuer Informationstechnik Berlin                */
/*                                                                           */
/*  SoPlex is distributed under the terms of the ZIB Academic Licence.       */
/*                                                                           */
/*  You should have received a copy of the ZIB Academic License              */
/*  along with SoPlex; see the file COPYING. If not email to soplex@zib.de.  */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#pragma ident "@(#) $Id: spxquality.cpp,v 1.4 2003/01/05 19:03:17 bzfkocht Exp $"

#include <assert.h>
#include <iostream>

#include "spxdefines.h"
#include "spxsolver.h"

namespace soplex
{

void SPxSolver::qualConstraintViolation(Real& maxviol, Real& sumviol) const
{
   maxviol = 0.0;
   sumviol = 0.0;

   DVector solu( nCols() );

   getPrimal( solu );

   for( int row = 0; row < nRows(); ++row )
   {
      const SVector& rowvec = rowVector( row );

      Real val = 0.0;         

      for( int col = 0; col < rowvec.size(); ++col )
         val += rowvec.value( col ) * solu[rowvec.index( col )];

      Real viol = 0.0;

      assert(lhs( row ) <= rhs( row ));

      if (val < lhs( row )) 
         viol = fabs(val - lhs( row ));
      else
         if (val > rhs( row ))
            viol = fabs(val - rhs( row ));

      if (viol > maxviol)
         maxviol = viol;

      sumviol += viol;
   }
}

void SPxSolver::qualBoundViolation(
   Real& maxviol, Real& sumviol) const
{
   maxviol = 0.0;
   sumviol = 0.0;

   DVector solu( nCols() );

   getPrimal( solu );

   for( int col = 0; col < nCols(); ++col )
   {
      assert( lower( col ) <= upper( col ));

      Real viol = 0.0;

      if (solu[col] < lower( col ))
         viol = fabs( solu[col] - lower( col ));
      else
         if (solu[col] > upper( col ))
            viol = fabs( solu[col] - upper( col ));
         
      if (viol > maxviol)
         maxviol = viol;

      sumviol += viol;
   }
}

void SPxSolver::qualSlackViolation(Real& maxviol, Real& sumviol) const
{
   maxviol = 0.0;
   sumviol = 0.0;

   DVector solu( nCols() );
   DVector slacks( nRows() );

   getPrimal( solu );
   getSlacks( slacks );

   for( int row = 0; row < nRows(); ++row )
   {
      const SVector& rowvec = rowVector( row );

      Real val = 0.0;         

      for( int col = 0; col < rowvec.size(); ++col )
         val += rowvec.value( col ) * solu[rowvec.index( col )];

      Real viol = fabs(val - slacks[row]);

      if (viol > maxviol)
         maxviol = viol;

      sumviol += viol;
   }
}

// This should be computed freshly.
void SPxSolver::qualRdCostViolation(Real& maxviol, Real& sumviol) const
{
   maxviol = 0.0;
   sumviol = 0.0;

   // TODO:   y = c_B * B^-1  => coSolve(y, c_B)
   //         redcost = c_N - yA_N 
   // solve system "x = e_i^T * B^-1" to get i'th row of B^-1
   // DVector y( nRows() );
   // basis().coSolve( x, spx->unitVector( i ) );

   DVector rdcost( nCols() );

   for( int col = 0; col < nCols(); ++col)
   {
      Real viol = 0.0;

      if (spxSense() == SPxLP::MINIMIZE)
      {
         if (rdcost[col] < 0.0)
            viol = -rdcost[col];
      }
      else
      {
         if (rdcost[col] > 0.0)
            viol = rdcost[col];
      }
      if (viol > maxviol)
         maxviol = viol;

      sumviol += viol;
   }
}

} // namespace soplex

//-----------------------------------------------------------------------------
//Emacs Local Variables:
//Emacs mode:c++
//Emacs c-basic-offset:3
//Emacs tab-width:8
//Emacs indent-tabs-mode:nil
//Emacs End:
//-----------------------------------------------------------------------------

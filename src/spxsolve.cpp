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
#pragma ident "@(#) $Id: spxsolve.cpp,v 1.58 2003/01/05 19:03:17 bzfkocht Exp $"

//#define DEBUGGING 1

#include <assert.h>
#include <iostream>

#include "spxdefines.h"
#include "spxsolver.h"
#include "spxpricer.h"
#include "spxratiotester.h"
#include "spxstarter.h"

namespace soplex
{
/**@todo After solve() returned, the algorithm type may have changed.
 *       This may be a problem if solve() is called again.
 * @todo The errors at the beginnin do not set m_status. On the other
 *       hand none of the routines that change for example the pricer
 *       changes the status.
 */
SPxSolver::Status SPxSolver::solve()
{
   METHOD( "SPxSolver::solve()" );

   SPxId enterId;
   int   leaveNum;

   if (dim() <= 0 && coDim() <= 0) // no problem loaded
      return NO_PROBLEM;

   if (slinSolver() == 0) // linear system solver is required.
      return NO_SOLVER;

   if (thepricer == 0) // pricer is required.
      return NO_PRICER;

   if (theratiotester == 0) // ratiotester is required.
      return NO_RATIOTESTER;

   theTime.reset();
   theTime.start();

   m_numCycle = 0;
   iterCount  = 0;

   if (!isInitialized())
   {
      /*
      if(SPxBasis::status() <= NO_PROBLEM)
          SPxBasis::load(this);
       */
      /*@todo != REGULAR is not enough. Also OPTIMAL/DUAL/PRIMAL should
       * be tested and acted accordingly.
       */
      if (thestarter != 0 && status() != REGULAR)  // no basis and no starter.
         thestarter->generate(*this);              // generate start basis.

      init();
   }
   thepricer->setEpsilon(delta());

   //setType(type());

   if (!matrixIsSetup)
   {
      std::cout << "== hallo!!!!!!!!!!!!!!!!!" << std::endl;
      SPxBasis::load(this);
   }
   //std::cout << "== solveX ==" << std::endl;
   //factorized = false;

   assert(thepricer->solver()      == this);
   assert(theratiotester->solver() == this);

   // maybe this should be done in init() ?
   thepricer->setType(type());
   theratiotester->setType(type());

   VERBOSE3({
      std::cout << "starting value = " << value() << std::endl;
      std::cout << "starting shift = " << shift() << std::endl;
   });
   DEBUG( desc().dump(); );

   if (SPxBasis::status() == SPxBasis::OPTIMAL)
      setBasisStatus(SPxBasis::REGULAR);

   m_status   = RUNNING;
   bool stop  = terminate();
   leaveCount = 0;
   enterCount = 0;

   while (!stop)
   {
      if (type() == ENTER)
      {
         do
         {
            VERBOSE3({
               if( iteration() % 100 == 0 )
                  std::cout << "Enter iteration: " << iteration()
                            << ", Value = " << value()
                            << ", Shift = " << shift() << std::endl;
            });

            enterId = thepricer->selectEnter();

            if (!enterId.isValid())
            {
               if (lastUpdate() == 0)
                  break;
              
               // We have a iterationlimit and everything look good? Then stop!
               // 6 is just a number picked.
               if (maxIters > 0 && lastUpdate() < 6
                  && (  SPxBasis::status() == SPxBasis::REGULAR 
                     || SPxBasis::status() == SPxBasis::DUAL 
                     || SPxBasis::status() == SPxBasis::PRIMAL))
                  break;

               std::cout << "== in solve == " << maxIters << " " << int(SPxBasis::status()) << std::endl;
               // We better refactor to make sure the solution is ok.
               factorize();

               enterId = thepricer->selectEnter();

               if (!enterId.isValid())
                  break;
            }
            enter(enterId);
            assert((testBounds(), 1));
            thepricer->entered4(lastEntered(), lastIndex());
            stop = terminate();
            clearUpdateVecs();
            enterCount += (lastIndex() >= 0);
            //@ assert(isConsistent());
         }
         while (!stop);

         VERBOSE3({
            std::cout << "Enter finished. iteration: " << iteration() 
                      << ", value: " << value()
                      << ", shift: " << shift()
                      << ", epsilon: " << epsilon()
                      << ", stop: " << stop
                      << ", basis status: " << int(SPxBasis::status())
                      << ", solver status: " << int(m_status) << std::endl;
         });

         if (!stop)
         {
            if (shift() <= epsilon())
            {
               // factorize();
               unShift();

               VERBOSE3({
                  std::cout << "maxInfeas: " << maxInfeas()
                            << ", shift: " << shift()
                            << ", delta: " << delta() << std::endl;
               });

               if (maxInfeas() + shift() <= delta())
               {
                  setBasisStatus(SPxBasis::OPTIMAL);
                  m_status = OPTIMAL;
                  break;
               }
            }
            setType(LEAVE);
            init();
            thepricer->setType(type());
            theratiotester->setType(type());
         }
      }
      else
      {
         assert(type() == LEAVE);

         do
         {
            VERBOSE3({
               if( iteration() % 100 == 0 )
                  std::cout << "Leave Iteration: " << iteration()
                            << ", Value = " << value()
                            << ", Shift = " << shift() << std::endl;
            });
            
            leaveNum = thepricer->selectLeave();

            if (leaveNum < 0)
            {
               // We have a fresh factorization? Then stop!
               if (lastUpdate() == 0)
                  break;

               // We have a iterationlimit and everything look good? Then stop!
               // 6 is just a number picked.
               if (maxIters > 0 && lastUpdate() < 6
                  && (  SPxBasis::status() == SPxBasis::REGULAR 
                     || SPxBasis::status() == SPxBasis::DUAL 
                     || SPxBasis::status() == SPxBasis::PRIMAL))
                  break;
               
               std::cout << "== in solve == " << maxIters << " " << int(SPxBasis::status()) << std::endl;
               // We better refactor to make sure the solution is ok.
               factorize();

               leaveNum = thepricer->selectLeave();

               if (leaveNum < 0)
                  break;
            }
            leave(leaveNum);
            assert((testBounds(), 1));
            thepricer->left4(lastIndex(), lastLeft());
            stop = terminate();
            clearUpdateVecs();
            leaveCount += lastEntered().isValid();
            //@ assert(isConsistent());
         }
         while (!stop);

         VERBOSE3({
            std::cout << "Leave finished. iteration: " << iteration() 
                      << ", value: " << value()
                      << ", shift: " << shift()
                      << ", epsilon: " << epsilon()
                      << ", stop: " << stop
                      << ", basis status: " << int(SPxBasis::status())
                      << ", solver status: " << int(m_status) << std::endl;
         });

         if (!stop)
         {
            if (shift() <= epsilon())
            {
               // factorize();
               unShift();

               VERBOSE3({
                  std::cout << "maxInfeas: " << maxInfeas()
                            << ", shift: " << shift()
                            << ", delta: " << delta() << std::endl;
               });

               if (maxInfeas() + shift() <= delta())
               {
                  setBasisStatus(SPxBasis::OPTIMAL);
                  m_status = OPTIMAL;
                  break;
               }
            }
            setType(ENTER);
            init();
            thepricer->setType(type());
            theratiotester->setType(type());
         }
      }
   }
   theTime.stop();

   if (m_status == RUNNING)
      m_status = ERROR;

   VERBOSE1({
      std::cout << "Finished solving (status=" << int(status());
      std::cout << ", iters=" << iterCount
                << ", leave=" << leaveCount
                << ", enter=" << enterCount;
      if( status() == OPTIMAL )
         std::cout << ", objValue=" << value();
      std::cout << ")" << std::endl;
   });

#ifndef NDEBUG
   /* check, if solution is really feasible */
   if( status() == OPTIMAL )
   {
      int     c;
      Real    val;
      DVector sol( nCols() );

      getPrimal( sol );

      for(int row = 0; row < nRows(); ++row )
      {
         const SVector& rowvec = rowVector( row );
         val = 0.0;         
         for( c = 0; c < rowvec.size(); ++c )
            val += rowvec.value( c ) * sol[rowvec.index( c )];

         if( LT( val, lhs( row ), delta() ) ||
             GT( val, rhs( row ), delta() ) )
         {
            std::cerr << "Warning! Constraint " << row
                      << " is violated by solution" << std::endl;
            std::cerr << "   lhs:" << lhs( row )
                      << " <= val:" << val
                      << " <= rhs:" << rhs( row ) << std::endl;

            if( type() == LEAVE && isRowBasic( row ) )
            {
               // find basis variable
               for( c = 0; c < nRows(); ++c )
                  if (basis().baseId(c).isSPxRowId()     
                     && (number(basis().baseId(c)) == row))
                     break;

               assert( c < nRows() );

               std::cerr << "   basis idx:" << c
                         << " fVec:" << fVec()[c]
                         << " fRhs:" << fRhs()[c]
                         << " fTest:" << fTest()[c] << std::endl;
            }
         }
      }
      for(int col = 0; col < nCols(); ++col )
      {
         if( LT( sol[col], lower( col ), delta() ) ||
             GT( sol[col], upper( col ), delta() ) )
         {
            std::cerr << "Warning! Bound for column " << col
                      << " is violated by solution" << std::endl;
            std::cerr << "   lower:" << lower( col )
                      << " <= val:" << sol[col]
                      << " <= upper:" << upper( col ) << std::endl;

            if( type() == LEAVE && isColBasic( col ) )
            {
               for( c = 0; c < nRows() ; ++c)
                  if ( basis().baseId( c ).isSPxColId()    
                     && ( number( basis().baseId( c ) ) == col ))
                     break;

               assert( c < nRows() );
               std::cerr << "   basis idx:" << c
                         << " fVec:" << fVec()[c]
                         << " fRhs:" << fRhs()[c]
                         << " fTest:" << fTest()[c] << std::endl;
            }
         }
      }
   }
#endif   
   return status();
}

void SPxSolver::testVecs()
{
   METHOD( "SPxSolver::testVecs()" );
   int i;
   DVector tmp(dim());

   tmp = *theCoPvec;
   multWithBase(tmp);
   tmp -= *theCoPrhs;
   if (tmp.length() > delta())
   {
      VERBOSE3({ std::cout << iteration() << ":\tcoP error = \t"
                           << tmp.length() << std::endl; });
      tmp.clear();
      SPxBasis::coSolve(tmp, *theCoPrhs);
      multWithBase(tmp);
      tmp -= *theCoPrhs;

      VERBOSE3( std::cout << "\t\t\t" << tmp.length() << std::endl; );

      tmp.clear();
      SPxBasis::coSolve(tmp, *theCoPrhs);
      tmp -= *theCoPvec;
      
      VERBOSE3( std::cout << "\t\t\t" << tmp.length() << std::endl; );
   }

   tmp = *theFvec;
   multBaseWith(tmp);
   tmp -= *theFrhs;
   if (tmp.length() > delta())
   {
      VERBOSE3({ std::cout << iteration() << ":\t  F error = \t"
                           << tmp.length() << std::endl; });
      tmp.clear();
      SPxBasis::solve(tmp, *theFrhs);
      tmp -= *theFvec;

      VERBOSE3( std::cout << "\t\t\t" << tmp.length() << std::endl; );
   }

#ifndef NDEBUG
   if (type() == ENTER)
   {
      for (i = 0; i < dim(); ++i)
      {
         if (theCoTest[i] < -delta() && isCoBasic(i))
         {
            std::cerr << "testVecs: theCoTest: this shalt not be!"
                      << std::endl
                      << "  i=" << i 
                      << ", theCoTest[i]=" << theCoTest[i]
                      << ", delta()=" << delta() << std::endl;
         }
      }
      for (i = 0; i < coDim(); ++i)
      {
         if (theTest[i] < -delta() && isBasic(i))
         {
            std::cerr << "testVecs: theTest: this shalt not be!"
                      << std::endl
                      << "  i=" << i 
                      << ", theTest[i]=" << theTest[i]
                      << ", delta()=" << delta() << std::endl;
         }
      }
   }
#endif
}

bool SPxSolver::terminate()
{
   METHOD( "SPxSolver::terminate()" );
#ifndef NDEBUG
   testVecs();
#endif  // NDEBUG

   int redo = dim();

   if (redo < 1000)
      redo = 1000;

   if (iteration() > 10 && iteration() % redo == 0)
   {
#ifndef NDEBUG
      DVector cr(*theCoPrhs);
      DVector fr(*theFrhs);
#endif  // !NDEBUG

      if (type() == ENTER)
         computeEnterCoPrhs();
      else
         computeLeaveCoPrhs();

      computeFrhs();

#ifndef NDEBUG
      cr -= *theCoPrhs;
      fr -= *theFrhs;
      if (cr.length() > delta())
         std::cerr << "unexpected change of coPrhs " 
                   << cr.length() << std::endl;
      if (fr.length() > delta())
         std::cerr << "unexpected change of   Frhs " 
                   << fr.length() << std::endl;
#endif  // !NDEBUG

      if (updateCount > 1)
      {
         std::cout << "== terminate == " << std::endl;
         factorize();
      }
      SPxBasis::coSolve(*theCoPvec, *theCoPrhs);
      SPxBasis::solve (*theFvec, *theFrhs);

      if (pricing() == FULL)
      {
         computePvec();
         if (type() == ENTER)
            computeTest();
      }
      if (shift() > 0)
         unShift();
   }

   if ( maxIters >= 0 && iterations() >= maxIters )
   {
      VERBOSE2({ std::cout << "Maximum number of iterations (" << maxIters
                           << ") reached" << std::endl; });
      m_status = ABORT_ITER;
      return true;
   }
   if ( maxTime >= 0 && maxTime < infinity && time() >= maxTime )
   {
      VERBOSE2({ std::cout << "Timelimit (" << maxTime
                           << ") reached" << std::endl; });
      m_status = ABORT_TIME;
      return true;   
   }
   if (maxValue < infinity)
   {
      /**@todo This code is *NOT* tested. */
         
      if( shift() < epsilon() && maxInfeas() + shift() <= delta() )
      {
         // SPxSense::MINIMIZE == -1, so we have sign = 1 on minimizing
         // rep() * type() > 0 == DUAL, -1 == PRIMAL.
         int sign = -1 * spxSense() * rep() * type();
         
         if( sign * (value() - maxValue) >= 0.0 )
         {
            VERBOSE2({ std::cout << "Objective value limit (" << maxValue
                                 << ") reached" << std::endl; });
            DEBUG({
               std::cout << "Objective value limit reached" << std::endl
                         << " (value: " << value()
                         << ", limit: " << maxValue << ")" << std::endl
                         << " (spxSense: " << int(spxSense())
                         << ", rep: " << int(rep())
                         << ", type: " << int(type()) << std::endl;
            });
            
            m_status = ABORT_VALUE;
            return true;
         }
      }
   }

   if( SPxBasis::status() >= SPxBasis::OPTIMAL  ||
       SPxBasis::status() <= SPxBasis::SINGULAR )
   {
      m_status = UNKNOWN;
      return true;
   }
   return false;
}

SPxSolver::Status SPxSolver::getPrimal (Vector& p_vector) const
{
   METHOD( "SPxSolver::getPrimal()" );

   if (!isInitialized())
      return NOT_INIT;

   if (rep() == ROW)
      p_vector = coPvec();
   else
   {
      const SPxBasis::Desc& ds = desc();

      for (int i = 0; i < nCols(); ++i)
      {
         switch (ds.colStatus(i))
         {
         case SPxBasis::Desc::P_ON_LOWER :
            p_vector[i] = SPxLP::lower(i);
            break;
         case SPxBasis::Desc::P_ON_UPPER :
         case SPxBasis::Desc::P_FIXED :
            p_vector[i] = SPxLP::upper(i);
            break;
         case SPxBasis::Desc::P_FREE :
            p_vector[i] = 0;
            break;
         case SPxBasis::Desc::D_FREE :
         case SPxBasis::Desc::D_ON_UPPER :
         case SPxBasis::Desc::D_ON_LOWER :
         case SPxBasis::Desc::D_ON_BOTH :
         case SPxBasis::Desc::D_UNDEFINED :
            break;
         default:
            abort();
         }
      }
      for (int i = 0; i < dim(); ++i)
      {
         if (baseId(i).isSPxColId())
            p_vector[ number(SPxColId(baseId(i))) ] = fVec()[i];
      }
   }
   return status();
}

SPxSolver::Status SPxSolver::getDual (Vector& p_vector) const
{
   METHOD( "SPxSolver::getDual()" );

   assert(isInitialized());

   if (!isInitialized())
      return NOT_INIT;

   if (rep() == ROW)
   {
      int i;
      p_vector.clear ();
      for (i = nCols() - 1; i >= 0; --i)
      {
         if (baseId(i).isSPxRowId())
            p_vector[ number(SPxRowId(baseId(i))) ] = fVec()[i];
      }
   }
   else
      p_vector = coPvec();

   p_vector *= Real(spxSense());

   return status();
}

SPxSolver::Status SPxSolver::getRdCost (Vector& p_vector) const
{
   METHOD( "SPxSolver::getRdCost()" );

   assert(isInitialized());

   if (!isInitialized())
      return NOT_INIT;

   if (rep() == ROW)
   {
      int i;
      p_vector.clear();
      if (spxSense() == SPxLP::MINIMIZE)
      {
         for (i = dim() - 1; i >= 0; --i)
         {
            if (baseId(i).isSPxColId())
               p_vector[ number(SPxColId(baseId(i))) ] = -fVec()[i];
         }
      }
      else
      {
         for (i = dim() - 1; i >= 0; --i)
         {
            if (baseId(i).isSPxColId())
               p_vector[ number(SPxColId(baseId(i))) ] = fVec()[i];
         }
      }
   }
   else
   {
      p_vector = maxObj();
      p_vector -= pVec();
      if (spxSense() == SPxLP::MINIMIZE)
         p_vector *= -1.0;
   }

   return status();
}

SPxSolver::Status SPxSolver::getSlacks (Vector& p_vector) const
{
   METHOD( "SPxSolver::getSlacks()" );

   assert(isInitialized());

   if (!isInitialized())
      return NOT_INIT;

   if (rep() == COLUMN)
   {
      int i;
      const SPxBasis::Desc& ds = desc();
      for (i = nRows() - 1; i >= 0; --i)
      {
         switch (ds.rowStatus(i))
         {
         case SPxBasis::Desc::P_ON_LOWER :
            p_vector[i] = lhs(i);
            break;
         case SPxBasis::Desc::P_ON_UPPER :
         case SPxBasis::Desc::P_FIXED :
            p_vector[i] = rhs(i);
            break;
         case SPxBasis::Desc::P_FREE :
            p_vector[i] = 0;
            break;
         case SPxBasis::Desc::D_FREE :
         case SPxBasis::Desc::D_ON_UPPER :
         case SPxBasis::Desc::D_ON_LOWER :
         case SPxBasis::Desc::D_ON_BOTH :
         case SPxBasis::Desc::D_UNDEFINED :
            break;
         default:
            abort();
         }
      }
      for (i = dim() - 1; i >= 0; --i)
      {
         if (baseId(i).isSPxRowId())
            p_vector[ number(SPxRowId(baseId(i))) ] = -(*theFvec)[i];
      }
   }
   else
      p_vector = pVec();

   return status();
}

SPxSolver::Status SPxSolver::status() const
{
   METHOD( "SPxSolver::status()" );

   switch( m_status )
   {
   case UNKNOWN :      
      switch (SPxBasis::status())
      {
      case SPxBasis::NO_PROBLEM :
         return NO_PROBLEM;
      case SPxBasis::SINGULAR :
         return SINGULAR;
      case SPxBasis::REGULAR :
      case SPxBasis::DUAL :
      case SPxBasis::PRIMAL :
         return UNKNOWN;
      case SPxBasis::OPTIMAL :
         return OPTIMAL;
      case SPxBasis::UNBOUNDED :
         return UNBOUNDED;
      case SPxBasis::INFEASIBLE :
         return INFEASIBLE;
      default:
         return ERROR;
      }
   case OPTIMAL :
      assert( SPxBasis::status() == SPxBasis::OPTIMAL );
      /*lint -fallthrough*/
   case ABORT_TIME :
   case ABORT_ITER :
   case ABORT_VALUE :
   case RUNNING :
   case REGULAR :
   case NOT_INIT :
   case NO_SOLVER :
   case NO_PRICER :
   case NO_RATIOTESTER :
   case ERROR:
      return m_status;
   default:
      return ERROR;
   }
}

SPxSolver::Status SPxSolver::getResult(
   Real* p_value,
   Vector* p_primal,
   Vector* p_slacks,
   Vector* p_dual,
   Vector* reduCosts) const
{
   METHOD( "SPxSolver::getResult()" );
   if (p_value)
      *p_value = this->value();
   if (p_primal)
      getPrimal(*p_primal);
   if (p_slacks)
      getSlacks(*p_slacks);
   if (p_dual)
      getDual(*p_dual);
   if (reduCosts)
      getRdCost(*reduCosts);
   return status();
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

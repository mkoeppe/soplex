/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*                  This file is part of the class library                   */
/*       SoPlex --- the Sequential object-oriented simPlex.                  */
/*                                                                           */
/*    Copyright (C) 1996-2014 Konrad-Zuse-Zentrum                            */
/*                            fuer Informationstechnik Berlin                */
/*                                                                           */
/*  SoPlex is distributed under the terms of the ZIB Academic Licence.       */
/*                                                                           */
/*  You should have received a copy of the ZIB Academic License              */
/*  along with SoPlex; see the file COPYING. If not email to soplex@zib.de.  */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "spxdefines.h"
#include "spxdevexpr.h"

#define DEVEX_REFINETOL 2.0

namespace soplex
{

void SPxDevexPR::load(SPxSolver* base)
{
   thesolver = base;
   setRep(base->rep());
   assert(isConsistent());
}

bool SPxDevexPR::isConsistent() const
{
#ifdef ENABLE_CONSISTENCY_CHECKS
   if (thesolver != 0)
      if (penalty.dim() != thesolver->coDim()
           || coPenalty.dim() != thesolver->dim())
         return MSGinconsistent("SPxDevexPR");
#endif

   return true;
}

void SPxDevexPR::init(SPxSolver::Type tp)
{
   int i;
   if (tp == SPxSolver::ENTER)
   {
      for (i = penalty.dim(); --i >= 0;)
         penalty[i] = 2;
      for (i = coPenalty.dim(); --i >= 0;)
         coPenalty[i] = 2;
      if( thesolver->hyperPricingEnter )
      {
         if( thesolver->sparsePricingEnter )
         {
            bestPrices.setMax(thesolver->dim());
            prices.reMax(thesolver->dim());
         }
         if( thesolver->sparsePricingEnterCo )
         {
            bestPricesCo.setMax(thesolver->coDim());
            pricesCo.reMax(thesolver->coDim());
         }
      }
   }
   else
   {
      for (i = coPenalty.dim(); --i >= 0;)
         coPenalty[i] = 1;
      if (thesolver->sparsePricingLeave && thesolver->hyperPricingLeave)
      {
         bestPrices.setMax(thesolver->dim());
         prices.reMax(thesolver->dim());
      }
   }
   assert(isConsistent());
}

void SPxDevexPR::setType(SPxSolver::Type tp)
{
   init(tp);
   refined = false;
}

/**@todo suspicious: Shouldn't the relation between dim, coDim, Vecs, 
 *       and CoVecs be influenced by the representation ?
 */
void SPxDevexPR::setRep(SPxSolver::Representation)
{
   if (thesolver != 0)
   {
      addedVecs(thesolver->coDim());
      addedCoVecs(thesolver->dim());
      assert(isConsistent());
   }
}

int SPxDevexPR::buildBestPriceVectorLeave( Real feastol )
{
   int idx;
   int nsorted;
   Real fTesti;
   const Real* fTest = thesolver->fTest().get_const_ptr();
   const Real* cpen = coPenalty.get_const_ptr();
   IdxElement price;
   prices.clear();
   bestPrices.clear();

   // TODO we should check infeasiblities for duplicates or loop over dimension
   //      bestPrices may then also contain duplicates!
   // construct vector of all prices
   for (int i = thesolver->infeasibilities.size() - 1; i >= 0; --i)
   {
      idx = thesolver->infeasibilities.index(i);
      fTesti = fTest[idx];
      if (fTesti < -feastol)
      {
         thesolver->isInfeasible[idx] = VIOLATED;
         price.idx = idx;
         price.val = fTesti * fTesti / cpen[idx];
         prices.append(price);
      }
   }
   // set up structures for the quicksort implementation
   compare.elements = prices.get_const_ptr();
   // do a partial sort to move the best ones to the front
   // TODO this can be done more efficiently, since we only need the indices
   nsorted = SPxQuicksortPart(prices.get_ptr(), compare, 0, prices.size(), thesolver->getMaxUpdates());
   // copy indices of best values to bestPrices
   for( int i = 0; i < nsorted; ++i )
   {
      bestPrices.addIdx(prices[i].idx);
      thesolver->isInfeasible[prices[i].idx] = VIOLATED_AND_CHECKED;
   }

   if( nsorted > 0 )
      return prices[0].idx;
   else
      return -1;
}

int SPxDevexPR::selectLeave()
{
   int retid;

   if (thesolver->hyperPricingLeave && thesolver->sparsePricingLeave)
   {
      if ( bestPrices.size() < 2 || thesolver->basis().lastUpdate() == 0 )
      {
         // call init method to build up price-vector and return index of largest price
         retid = buildBestPriceVectorLeave(theeps);
      }
      else
         retid = selectLeaveHyper(theeps);
   }
   else if (thesolver->sparsePricingLeave)
      retid = selectLeaveSparse(theeps);
   else
      retid = selectLeaveX(theeps);

   if ( retid < 0 && !refined )
   {
      refined = true;
      MSG_INFO3( spxout << "WDEVEX02 trying refinement step..\n"; )
      retid = selectLeaveX(theeps/DEVEX_REFINETOL);
   }

   assert(retid < thesolver->dim());

   return retid;
}

int SPxDevexPR::selectLeaveX(Real feastol, int start, int incr)
{
   Real x;

   const Real* fTest = thesolver->fTest().get_const_ptr();
   const Real* cpen = coPenalty.get_const_ptr();
   Real best = 0;
   int bstI = -1;
   int end = coPenalty.dim();

   for (; start < end; start += incr)
   {
      if (fTest[start] < -feastol)
      {
         x = fTest[start] * fTest[start] / cpen[start];
         if (x > best)
         {
            best = x;
            bstI = start;
            last = cpen[start];
         }
      }
   }
   return bstI;
}

int SPxDevexPR::selectLeaveSparse(Real feastol)
{
   Real x;

   const Real* fTest = thesolver->fTest().get_const_ptr();
   const Real* cpen = coPenalty.get_const_ptr();
   Real best = 0;
   int bstI = -1;
   int idx = -1;
   Real fTesti;
   Real coPeni;

   for (int i = thesolver->infeasibilities.size() - 1; i >= 0; --i)
   {
      idx = thesolver->infeasibilities.index(i);
      fTesti = fTest[idx];
      if (fTesti < -feastol)
      {
         coPeni = cpen[idx];
         x = fTesti * fTesti / coPeni;
         if (x > best)
         {
            best = x;
            bstI = idx;
            last = coPeni;
         }
      }
      else
      {
         thesolver->infeasibilities.remove(i);
         assert(thesolver->isInfeasible[idx] == VIOLATED || thesolver->isInfeasible[idx] == VIOLATED_AND_CHECKED);
         thesolver->isInfeasible[idx] = NOT_VIOLATED;
      }
   }
   return bstI;
}

int SPxDevexPR::selectLeaveHyper(Real feastol)
{
   Real x;

   const Real* fTest = thesolver->fTest().get_const_ptr();
   const Real* cpen = coPenalty.get_const_ptr();
   Real best = 0;
   Real leastBest = infinity;
   int bstI = -1;
   int idx = -1;
   Real fTesti;
   Real coPeni;

   // find the best price from the short candidate list
   for( int i = bestPrices.size() - 1; i >= 0; --i )
   {
      idx = bestPrices.index(i);
      fTesti = fTest[idx];
      if( fTesti < -feastol )
      {
         coPeni = cpen[idx];
         x = fTesti * fTesti / coPeni;
         if( x > best )
         {
            best = x;
            bstI = idx;
            last = coPeni;
         }
         // get the smallest price of candidate list
         if( x < leastBest )
            leastBest = x;
      }
      else
      {
         bestPrices.remove(i);
         thesolver->isInfeasible[idx] = NOT_VIOLATED;
      }
   }

   // make sure we do not skip potential candidates due to a high leastBest value
   if( leastBest == infinity )
   {
      assert(bestPrices.size() == 0);
      leastBest = 0;
   }

   // scan the updated indices for a better price
   for( int i = thesolver->updateViols.size() - 1; i >= 0; --i )
   {
      idx = thesolver->updateViols.index(i);
      // only look at indeces that were not checked already
      if( thesolver->isInfeasible[idx] == VIOLATED )
      {
         fTesti = fTest[idx];
         assert(fTesti < -feastol);
         coPeni = cpen[idx];
         x = fTesti * fTesti / coPeni;
         if( x > leastBest )
         {
            if( x > best )
            {
               best = x;
               bstI = idx;
               last = coPeni;
            }
            // put index into candidate list
            thesolver->isInfeasible[idx] = VIOLATED_AND_CHECKED;
            bestPrices.addIdx(idx);
         }
      }
   }

   return bstI;
}


void SPxDevexPR::left4(int n, SPxId id)
{
   left4X(n, id, 0, 1);
}

void SPxDevexPR::left4X(int n, const SPxId& id, int start, int incr)
{
   if (id.isValid())
   {
      int i, j;
      Real x;
      const Real* rhoVec = thesolver->fVec().delta().values();
      Real rhov_1 = 1 / rhoVec[n];
      Real beta_q = thesolver->coPvec().delta().length2() * rhov_1 * rhov_1;

#ifndef NDEBUG
      if (fabs(rhoVec[n]) < theeps)
      {
         MSG_ERROR( spxout << "WDEVEX01: rhoVec = "
                           << rhoVec[n] << " with smaller absolute value than theeps = " << theeps << std::endl; )
      }
#endif  // NDEBUG

      //  Update #coPenalty# vector
      const IdxSet& rhoIdx = thesolver->fVec().idx();
      int len = thesolver->fVec().idx().size();
      for (i = len - 1 - start; i >= 0; i -= incr)
      {
         j = rhoIdx.index(i);
         x = rhoVec[j] * rhoVec[j] * beta_q;
         // if(x > coPenalty[j])
         coPenalty[j] += x;
      }

      coPenalty[n] = beta_q;
   }
}

SPxId SPxDevexPR::buildBestPriceVectorEnterDim( Real& best, Real feastol )
{
   int idx;
   int nsorted;
   Real x;
   const Real* coTest = thesolver->coTest().get_const_ptr();
   const Real* cpen = coPenalty.get_const_ptr();
   IdxElement price;
   prices.clear();
   bestPrices.clear();

   // construct vector of all prices
   for (int i = thesolver->infeasibilities.size() - 1; i >= 0; --i)
   {
      idx = thesolver->infeasibilities.index(i);
      x = coTest[idx];
      if ( x < -feastol)
      {
         thesolver->isInfeasible[idx] = VIOLATED;
         price.idx = idx;
         price.val = x * x / cpen[idx];
         prices.append(price);
      }
      else
      {
         thesolver->infeasibilities.remove(i);
         thesolver->isInfeasible[idx] = NOT_VIOLATED;
      }
   }
   // set up structures for the quicksort implementation
   compare.elements = prices.get_const_ptr();
   // do a partial sort to move the best ones to the front
   // TODO this can be done more efficiently, since we only need the indices
   nsorted = SPxQuicksortPart(prices.get_ptr(), compare, 0, prices.size(), thesolver->getMaxUpdates());
   // copy indices of best values to bestPrices
   for( int i = 0; i < nsorted; ++i )
   {
      bestPrices.addIdx(prices[i].idx);
      thesolver->isInfeasible[prices[i].idx] = VIOLATED_AND_CHECKED;
   }

   if( nsorted > 0 )
   {
      best = prices[0].val;
      return thesolver->coId(prices[0].idx);
   }
   else
      return SPxId();
}

SPxId SPxDevexPR::buildBestPriceVectorEnterCoDim( Real& best, Real feastol )
{
   int idx;
   int nsorted;
   Real x;
   const Real* test = thesolver->test().get_const_ptr();
   const Real* pen = penalty.get_const_ptr();
   IdxElement price;
   pricesCo.clear();
   bestPricesCo.clear();

   // construct vector of all prices
   for (int i = thesolver->infeasibilitiesCo.size() - 1; i >= 0; --i)
   {
      idx = thesolver->infeasibilitiesCo.index(i);
      x = test[idx];
      if ( x < -feastol)
      {
         thesolver->isInfeasibleCo[idx] = VIOLATED;
         price.idx = idx;
         price.val = x * x / pen[idx];
         pricesCo.append(price);
      }
      else
      {
         thesolver->infeasibilitiesCo.remove(i);
         thesolver->isInfeasibleCo[idx] = NOT_VIOLATED;
      }
   }
   // set up structures for the quicksort implementation
   compare.elements = pricesCo.get_const_ptr();
   // do a partial sort to move the best ones to the front
   // TODO this can be done more efficiently, since we only need the indices
   nsorted = SPxQuicksortPart(pricesCo.get_ptr(), compare, 0, pricesCo.size(), thesolver->getMaxUpdates());
   // copy indices of best values to bestPrices
   for( int i = 0; i < nsorted; ++i )
   {
      bestPricesCo.addIdx(pricesCo[i].idx);
      thesolver->isInfeasibleCo[pricesCo[i].idx] = VIOLATED_AND_CHECKED;
   }

   if( nsorted > 0 )
   {
      best = pricesCo[0].val;
      return thesolver->id(pricesCo[0].idx);
   }
   else
      return SPxId();
}

SPxId SPxDevexPR::selectEnter()
{
   assert(thesolver != 0);

   SPxId enterId;

   enterId = selectEnterX(theeps);

   if( !enterId.isValid() && !refined )
   {
      refined = true;
      MSG_INFO3( spxout << "WDEVEX02 trying refinement step..\n"; )
      enterId = selectEnterX(theeps/DEVEX_REFINETOL);
   }

   return enterId;
}

// choose the best entering index among columns and rows but prefer sparsity
SPxId SPxDevexPR::selectEnterX(Real tol)
{
   SPxId enterId;
   SPxId enterCoId;
   Real best;
   Real bestCo;

   best = 0;
   bestCo = 0;

   if( thesolver->hyperPricingEnter && !refined )
   {
      if( bestPrices.size() < 2 || thesolver->basis().lastUpdate() == 0 )
         enterCoId = (thesolver->sparsePricingEnter) ? buildBestPriceVectorEnterDim(best, tol) : selectEnterDenseDim(best, tol);
      else
         enterCoId = (thesolver->sparsePricingEnter) ? selectEnterHyperDim(best, tol) : selectEnterDenseDim(best, tol);

      if( bestPricesCo.size() < 2 || thesolver->basis().lastUpdate() == 0 )
         enterId = (thesolver->sparsePricingEnterCo) ? buildBestPriceVectorEnterCoDim(bestCo, tol) : selectEnterDenseCoDim(bestCo, tol);
      else
         enterId = (thesolver->sparsePricingEnterCo) ? selectEnterHyperCoDim(bestCo, tol) : selectEnterDenseCoDim(bestCo, tol);
   }
   else
   {
      enterCoId = (thesolver->sparsePricingEnter && !refined) ? selectEnterSparseDim(best, tol) : selectEnterDenseDim(best, tol);
      enterId = (thesolver->sparsePricingEnterCo && !refined) ? selectEnterSparseCoDim(bestCo, tol) : selectEnterDenseCoDim(bestCo, tol);
   }

   // prefer coIds to increase the number of unit vectors in the basis matrix, i.e., rows in colrep and cols in rowrep
   if( enterCoId.isValid() && (best > SPARSITY_TRADEOFF * bestCo || !enterId.isValid()) )
      return enterCoId;
   else
      return enterId;
}

SPxId SPxDevexPR::selectEnterHyperDim(Real& best, Real feastol)
{
   const Real* cTest = thesolver->coTest().get_const_ptr();
   const Real* cpen = coPenalty.get_const_ptr();
   Real leastBest = infinity;
   Real coTesti;
   Real coPeni;
   Real x;
   int enterIdx = -1;
   int idx;

   // find the best price from short candidate list
   for( int i = bestPrices.size() - 1; i >= 0; --i )
   {
      idx = bestPrices.index(i);
      coTesti = cTest[idx];
      if( coTesti < -feastol )
      {
         coPeni = cpen[idx];
         x = coTesti * coTesti / coPeni;
         if( x > best )
         {
            best = x;
            enterIdx = idx;
            last = coPeni;
         }
         if( x < leastBest )
            leastBest = x;
      }
      else
      {
         bestPrices.remove(i);
         thesolver->isInfeasible[idx] = NOT_VIOLATED;
      }
   }

   // make sure we do not skip potential candidates due to a high leastBest value
   if( leastBest == infinity )
   {
      assert(bestPrices.size() == 0);
      leastBest = 0;
   }

   // scan the updated indeces for a better price
   for( int i = thesolver->updateViols.size() -1; i >= 0; --i )
   {
      idx = thesolver->updateViols.index(i);
      // only look at indeces that were not checked already
      if( thesolver->isInfeasible[idx] == VIOLATED )
      {
         if( coTesti < -feastol )
         {
            coTesti = cTest[idx];
            coPeni = cpen[idx];
            x = coTesti * coTesti / coPeni;
            if(x > leastBest)
            {
               if( x > best )
               {
                  best = x;
                  enterIdx = idx;
                  last = cpen[idx];
               }
               // put index into candidate list
               thesolver->isInfeasible[idx] = VIOLATED_AND_CHECKED;
               bestPrices.addIdx(idx);
            }
         }
         else
         {
            thesolver->isInfeasible[idx] = NOT_VIOLATED;
         }
      }
   }

   if (enterIdx >= 0)
      return thesolver->coId(enterIdx);
   else
      return SPxId();
}


SPxId SPxDevexPR::selectEnterHyperCoDim(Real& best, Real feastol)
{
   const Real* test = thesolver->test().get_const_ptr();
   const Real* pen = penalty.get_const_ptr();
   Real leastBest = infinity;
   Real testi;
   Real peni;
   Real x;
   int enterIdx = -1;
   int idx;

   // find the best price from short candidate list
   for( int i = bestPricesCo.size() - 1; i >= 0; --i )
   {
      idx = bestPricesCo.index(i);
      testi = test[idx];
      if( testi < -feastol )
      {
         peni = pen[idx];
         x = testi * testi / peni;
         if( x > best )
         {
            best = x;
            enterIdx = idx;
            last = peni;
         }
         if( x < leastBest )
            leastBest = x;
      }
      else
      {
         bestPricesCo.remove(i);
         thesolver->isInfeasibleCo[idx] = NOT_VIOLATED;
      }
   }
   // make sure we do not skip potential candidates due to a high leastBest value
   if( leastBest == infinity )
   {
      assert(bestPricesCo.size() == 0);
      leastBest = 0;
   }

   //scan the updated indeces for a better price
   for( int i = thesolver->updateViolsCo.size() -1; i >= 0; --i )
   {
      idx = thesolver->updateViolsCo.index(i);
      // only look at indeces that were not checked already
      if( thesolver->isInfeasibleCo[idx] == VIOLATED )
      {
         if( testi < -feastol )
         {
            testi = test[idx];
            peni = pen[idx];
            x = testi * testi / peni;
            if(x > leastBest)
            {
               if( x > best )
               {
                  best = x;
                  enterIdx = idx;
                  last = pen[idx];
               }
               // put index into candidate list
               thesolver->isInfeasibleCo[idx] = VIOLATED_AND_CHECKED;
               bestPricesCo.addIdx(idx);
            }
         }
         else
         {
            thesolver->isInfeasibleCo[idx] = NOT_VIOLATED;
         }
      }
   }

   if (enterIdx >= 0)
      return thesolver->id(enterIdx);
   else
      return SPxId();
}


SPxId SPxDevexPR::selectEnterSparseDim(Real& best, Real feastol)
{
   const Real* cTest = thesolver->coTest().get_const_ptr();
   const Real* cpen = coPenalty.get_const_ptr();
   int enterIdx = -1;
   int idx;
   Real coTesti;
   Real coPeni;
   Real x;

   assert(coPenalty.dim() == thesolver->coTest().dim());
   for(int i = thesolver->infeasibilities.size() -1; i >= 0; --i)
   {
      idx = thesolver->infeasibilities.index(i);
      coTesti = cTest[idx];
      if (coTesti < -feastol)
      {
         coPeni = cpen[idx];
         x = coTesti * coTesti / coPeni;
         if (x > best)
         {
            best = x;
            enterIdx = idx;
            last = cpen[idx];
         }
      }
      else
      {
         thesolver->infeasibilities.remove(i);
         thesolver->isInfeasible[idx] = NOT_VIOLATED;
      }
   }
   if (enterIdx >= 0)
      return thesolver->coId(enterIdx);

   return SPxId();
}


SPxId SPxDevexPR::selectEnterSparseCoDim(Real& best, Real feastol)
{
   const Real* test = thesolver->test().get_const_ptr();
   const Real* pen = penalty.get_const_ptr();
   int enterIdx = -1;
   int idx;
   Real testi;
   Real peni;
   Real x;

   assert(penalty.dim() == thesolver->test().dim());
   for (int i = thesolver->infeasibilitiesCo.size() -1; i >= 0; --i)
   {
      idx = thesolver->infeasibilitiesCo.index(i);
      testi = test[idx];
      if (testi < -feastol)
      {
         peni = pen[idx];
         x = testi * testi / peni;
         if (x > best)
         {
            best = x;
            enterIdx = idx;
            last = pen[idx];
         }
      }
      else
      {
         thesolver->infeasibilitiesCo.remove(i);
         thesolver->isInfeasibleCo[idx] = NOT_VIOLATED;
      }
   }

   if (enterIdx >= 0)
      return thesolver->id(enterIdx);

   return SPxId();
}


SPxId SPxDevexPR::selectEnterDenseDim(Real& best, Real feastol, int start, int incr)
{
   const Real* cTest = thesolver->coTest().get_const_ptr();
   const Real* cpen = coPenalty.get_const_ptr();
   int end = coPenalty.dim();
   int enterIdx = -1;
   Real x;

   assert(end == thesolver->coTest().dim());
   for (; start < end; start += incr)
   {
      if (cTest[start] < -feastol)
      {
         x = cTest[start] * cTest[start] / cpen[start];
         if (x > best)
         {
            best = x;
            enterIdx = start;
            last = cpen[start];
         }
      }
   }

   if (enterIdx >= 0)
      return thesolver->coId(enterIdx);

   return SPxId();
}


SPxId SPxDevexPR::selectEnterDenseCoDim(Real& best, Real feastol, int start, int incr)
{
   const Real* test = thesolver->test().get_const_ptr();
   const Real* pen = penalty.get_const_ptr();
   int end = penalty.dim();
   int enterIdx = -1;
   Real x;

   assert(end == thesolver->test().dim());
   for (; start < end; start += incr)
   {
      if (test[start] < -feastol)
      {
         x = test[start] * test[start] / pen[start];
         if (x > best)
         {
            best = x;
            enterIdx = start;
            last = pen[start];
         }
      }
   }

   if (enterIdx >= 0)
      return thesolver->id(enterIdx);

   return SPxId();
}


void SPxDevexPR::entered4(SPxId id, int n)
{
   entered4X(id, n, 0, 1, 0, 1);
}

/**@todo suspicious: the pricer should be informed, that variable id 
    has entered the basis at position n, but the id is not used here 
    (this is true for all pricers)
*/
void SPxDevexPR::entered4X(SPxId /*id*/, int n,
   int start1, int incr1, int start2, int incr2)
{
   if (n >= 0 && n < thesolver->dim())
   {
      const Real* pVec = thesolver->pVec().delta().values();
      const IdxSet& pIdx = thesolver->pVec().idx();
      const Real* coPvec = thesolver->coPvec().delta().values();
      const IdxSet& coPidx = thesolver->coPvec().idx();
      Real xi_p = 1 / thesolver->fVec().delta()[n];
      int i, j;

      assert(thesolver->fVec().delta()[n] > thesolver->epsilon()
              || thesolver->fVec().delta()[n] < -thesolver->epsilon());

      xi_p = xi_p * xi_p * last;

      for (j = coPidx.size() - 1 - start1; j >= 0; j -= incr1)
      {
         i = coPidx.index(j);
         coPenalty[i] += xi_p * coPvec[i] * coPvec[i];
         if (coPenalty[i] <= 1 || coPenalty[i] > 1e+6)
         {
            init(SPxSolver::ENTER);
            return;
         }
      }

      for (j = pIdx.size() - 1 - start2; j >= 0; j -= incr2)
      {
         i = pIdx.index(j);
         penalty[i] += xi_p * pVec[i] * pVec[i];
         if (penalty[i] <= 1 || penalty[i] > 1e+6)
         {
            init(SPxSolver::ENTER);
            return;
         }
      }
   }
}

void SPxDevexPR::addedVecs (int n)
{
   int initval = (thesolver->type() == SPxSolver::ENTER) ? 2 : 1;
   n = penalty.dim();
   penalty.reDim (thesolver->coDim());
   for (int i = penalty.dim()-1; i >= n; --i )
      penalty[i] = initval;
}

void SPxDevexPR::addedCoVecs(int n)
{
   int initval = (thesolver->type() == SPxSolver::ENTER) ? 2 : 1;
   n = coPenalty.dim();
   coPenalty.reDim(thesolver->dim());
   for (int i = coPenalty.dim()-1; i >= n; --i)
      coPenalty[i] = initval;
}

} // namespace soplex

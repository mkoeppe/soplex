/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*                  This file is part of the class library                   */
/*       SoPlex --- the Sequential object-oriented simPlex.                  */
/*                                                                           */
/*    Copyright (C) 1997-1999 Roland Wunderling                              */
/*                  1997-2001 Konrad-Zuse-Zentrum                            */
/*                            fuer Informationstechnik Berlin                */
/*                                                                           */
/*  SoPlex is distributed under the terms of the ZIB Academic Licence.       */
/*                                                                           */
/*  You should have received a copy of the ZIB Academic License              */
/*  along with SoPlex; see the file COPYING. If not email to soplex@zib.de.  */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#pragma ident "@(#) $Id: spxratiotester.h,v 1.2 2001/11/06 23:31:04 bzfkocht Exp $"

#ifndef _SPXRATIOTESTER_H_
#define _SPXRATIOTESTER_H_


//@ ----------------------------------------------------------------------------
/*      \Section{Imports}
    Import required system include files
 */
#include <assert.h>


/*  and class header files
 */

#include "soplex.h"

namespace soplex
{






//@ ----------------------------------------------------------------------------
/* \Section{Class Declaration}
 */

/** #SoPlex# ratio test base class.
    Class #SPxRatioTester# is the virtual base class for computing the ratio
    test within the Simplex algorithm driven by #SoPlex#. After a #SoPlex#
    solver has been #load()#ed to an #SPxRatioTester#, the solver calls
    #selectLeave()# for computing the ratio test for the entering simplex and
    #selectEnter()# for computing the ratio test in leaving simplex.
 */
class SPxRatioTester
{
public:
   /** Load LP.
       Load the solver and LP for which pricing steps are to be performed.
    */
   virtual void load(SoPlex* lp) = 0;

   ///
   virtual void clear() = 0;

   ///
   virtual SoPlex* solver() const = 0;

   /** Select index to leave the basis.
       Method #selectLeave()# is called by the loaded #SoPlex# solver, when
       computing the entering simplex algorithm. It's task is to select and
       return the index of the basis variable that is to leave the basis.
       When beeing called, #fVec()# fullfills the basic bounds #lbBound()#
       and #ubBound()# within #delta#. #fVec().delta()# is the vector by
       which #fVec()# will be updated in this simplex step. Its nonzero
       indeces are stored in sorted order in #fVec().idx()#.
       
       If #val>0#, #val# is the maximum allowed update value for #fVec()#,
       otherwise the minimum. Method #selectLeave()# must chose #val# of the
       same sign as passed, such that updating #fVec()# by #val# yields a
       new vector that satisfies all basic bounds (within #delta#). The
       returned index, must be the index of an element of #fVec()#, that
       reaches one of its bounds with this update.
    */
   virtual int selectLeave(double& val) = 0;

   /** Select Id to enter the basis.
       Method #selectEnter# is called by the loaded #SoPlex# solver, when
       computing the leaving simplex algorithm. It's task is to select and
       return the #Id# of the basis variable that is to enter the basis.
       When beeing called, #pVec()# fullfills the bounds #lpBound()# and
       #upBound()# and #coPvec()# bounds #lcBound()# and #ucBound()# within
       #delta#, respectively. #pVec().delta()# and #coPvec().delta()# are
       the vectors by which #pVec()# and #coPvec()# will be updated in this
       simplex step. Their nonzero indeces are stored in sorted order in
       #pVec().idx()# and #coPvec().idx()#.
       
       If #val>0#, #val# is the maximum allowed update value for #pVec()#
       and #coPvec()#, otherwise the minimum. Method #selectEnter()# must
       chose #val# of the same sign as passed, such that updating #pVec()#
       and #coPvec()# by #val# yields a new vector that satisfies all basic
       bounds (within #delta#). The returned #Id#, must be the #Id# of an
       element of #pVec()# or #coPvec()#, that reaches one of its bounds
       with this update.
    */
   virtual SoPlex::Id selectEnter(double& val) = 0;

   /** Set Simplex type.
       Inform pricer about (a change of) the loaded #SoPlex#'s #Type#. In
       the sequel, only the corresponding select methods may be called.
    */
   virtual void setType(SoPlex::Type) = 0;

   ///
   virtual ~SPxRatioTester()
   {}

};


} // namespace soplex
#endif // _SPXRATIOTESTER_H_

//-----------------------------------------------------------------------------
//Emacs Local Variables:
//Emacs mode:c++
//Emacs c-basic-offset:3
//Emacs tab-width:8
//Emacs indent-tabs-mode:nil
//Emacs End:
//-----------------------------------------------------------------------------

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
#pragma ident "@(#) $Id: lpsolver.h,v 1.2 2001/11/06 23:31:02 bzfkocht Exp $"


#ifndef _LPSOLVER_H_
#define _LPSOLVER_H_

/*      \Section{Imports}
 */

#include "svector.h"
#include "vector.h"
#include "lpcolset.h"
#include "lprowset.h"

namespace soplex
{


/*      \Section{Declaration of LPSolver}
 */
/** Interface class for LP solver.
Class #LPSolver# provides a generic interface to simplex-type linear program solvers.
After construction, an LP solver is available with an empty problem loaded
to it. The solver can be loaded with a nontrival problem by means of methods
#load()# or #read()#. The latter method reads an LP from a file, while the
former passes an LP as argument.
 
The problem loaded to an #LPSolver# is considered to be of the form:
\[
    \begin{array}{rl}
        \hbox{min/max}  & c^T x         \\
        \hbox{s.t.}     & l \le Ax \le r        \\
                        & w \le x \le u
    \end{array}
\]
Vector $c$ will be referred to as the objective Vector, $l$ and $r$ as the left
and right hand side vectors, respectively, and $w$ and $u$ as lower and upper
bounds, respectively.
 
The problem loaded to an #LPSolver# can be solved with either the primal or dual
simplex algorithm, by calling method #solve()#. A termination criterion for the
solution method can be set with method #setTermination()#.
 
An #LPSolver# has a #Status# associated to it, indicating what the solver knows
about the problem loaded to it. Most non-#const# methods change the status of
the solver.
 
There are various methods available for changing the problem loaded to an
#LPSolver#. They allow to add, change or remove rows or columns of the
constraint matrix or to modify the variable's upper and lower bound, the
objective vector or the left- and right hand sides and senses of the constraint
inequalities.
 
The rows and columns of the LP loaded to an #LPSolver# are numbered implicitly
from 0 to #nofRows()-1# and 0 to #nofCols()-1#, respectively. When rows or columns are
added to the LP, they receive indices #nofRows()#, #nofRows()+1#, ... or #nofCols()#,
#nofCols()+1#, ..., respectively. The indices of the first rows and columns in
the LP remain unchanged. When adding rows or columns, no precautions with
respect to memory management are required, i.e.\ all memory is reset to the
required size automatically.
 
When removing rows or columns from the LP loaded to an #LPSolver# the remaining
rows and columns are renumbered. However, all rows or columns with number lower
than the first one removed keep their number unchanged.
 
Class #LPSolver# provides two means of dealing with the renumbering of the
remaining rows or columns.  First, each removal method provides an optional
parameter, where an array of indices (#int* idx#) can be passed. Upon return of
the remove method, this number array contains the permutations due to the
removal, i.e. #idx[i]# is the new number of the row or column, that had number #i#
{\em before} the last remove method was called. Hence, the number arrays must be
at least of size #nofRows()# or #nofCols()#, respectively.
 
The second concept for dealing with the renumbering of row or column indices in
removal methods is the provision of #RowId#s and #ColId#s. An #LPSolver#
associates a unique #RowId# to each row and a #ColId# to each column of the
loaded LP.  The Id of a row or column remains fixed throughout the time, the row
or column belongs to the LP. In most methods, #RowId#s and #ColId#s may be used
instead of indices.
 */
class LPSolver
{
public:
   /**@name {\em Datatypes} */
   //@{
   /** solver status.
       This enumeration type describes the amount of information, the solver
       knows about its LP.
    */
   enum Status
   {
      /// nothing known on loaded problem.
      UNKNOWN = 0,
      /// loaded problem is unbounded.
      UNBOUNDED,
      /// loaded problem is infeasible.
      INFEASIBLE,
      /// primal (not yet optimal) solution available.
      PRIMAL,
      /// dual (not yet optimal) solution available.
      DUAL,
      /// loaded problem has been solved.
      SOLVED,
      /// an error occurred.
      ERROR
   };


   /// optimization sense.
   enum Sense
   {
      ///
      MAXIMIZE = 1,
      ///
      MINIMIZE = -1
   };


   /// unique id to access columns in an #LPSolver#.
   struct ColId
   {
      int id;
   };

   /// unique id to access rows in an #LPSolver#.
   struct RowId
   {
      int id;
   };

   /** status of variables.
       A basis assigns a #VarStatus# status to each variable of the loaded
       LP.  The names for the variable status come from a standard column
       basis representation.  These are:
       \begin{description}
       \item[#ON_UPPER#]   if the variable is nonbasic and on its upper
                           bound
       \item[#ON_LOWER#]   if the variable is nonbasic and on its upper
                           bound
       \item[#FIXED#]      if the variable is nonbasic and its upper
                           bound is equal to its lower bound
       \item[#FIXED#]      if the variable is nonbasic and unbounded
       \item[#BASIC#]      if the variable is basic
       \end{description}
       For slack variables the interpretation of #ON_UPPER# means, that the
       upper bound of the inequality is tight.  Similarily, if the status
       of a slack variables is #ON_LOWER# the lower bound of the inequality
       is tight.
    */
   enum VarStatus
   {
      /// variable set to its upper bound
      ON_UPPER,
      /// variable set to its lower bound
      ON_LOWER,
      /// variable fixed to identical bounds
      FIXED,
      /// free variable fixed to zero
      ZERO,
      /// variable is basic
      BASIC
   };

   /// value used as $\infty$.
   static const double infinity;
   //@}


   /**@name Solving LPs */
   //@{
   /// solve current LP with the simplex method.
   virtual Status solve() = 0;

   /** Set current basis.
    *  Each variable is set to the status specified in the arrays #rows#
    *  and #cols#, which must be of adequate size.
    *  @see        VarStatus
    */
   virtual void setBasis(const signed char rows[],
                         const signed char cols[]) = 0;

   /// adjust conditions for termination.
   virtual void setTermination(double value = infinity,
                               double time = -1,
                               int iteration = -1) = 0;

   /// get adjusted conditions for termination.
   virtual void getTermination(double* value = 0,
                               double* time = 0,
                               int* iteration = 0) const = 0;
   //@}


   /**@name Accessing Computational Results */
   //@{
   /// return objective value of current solution.
   virtual double objValue() const = 0;

   /// get current solution vector for primal variables.
   virtual Status getPrimal(Vector& vector) const = 0;

   /// return const solution vector for primal variables.
   virtual const Vector& primal() const = 0;

   /// get current solution vector for dual variables.
   virtual Status getDual(Vector& vector) const = 0;

   /// return const solution vector for dual variables.
   virtual const Vector& dual() const = 0;

   /// get current vector of slack variables.
   virtual Status getSlacks(Vector& vector) const = 0;

   /// return const vector of slack variables.
   virtual const Vector& slacks() const = 0;

   /// get vector of reduced costs.
   virtual Status getRdCost(Vector& vector) const = 0;

   /// return const vector of reduced costs.
   virtual const Vector& rdCost() const = 0;

   /// get all results of last solve.
   virtual Status getResult(double* value = 0,
                            Vector* primal = 0,
                            Vector* slacks = 0,
                            Vector* dual = 0,
                            Vector* reduCost = 0) const = 0;

   /** Get current basis.
    *  The status information for each variable and row is copied into the
    *  arrays #rows# and #cols#, which must be of adequate size.
    *  @see    VarStatus
    */
   virtual Status getBasis(signed char rows[],
                           signed char cols[]) const = 0;

   /// return status for primal vars if available.
   virtual const signed char* rowBasis() const = 0;

   /// return status for dual vars if available.
   virtual const signed char* colBasis() const = 0;

   /// get number of iterations of current solution.
   virtual int iterations() const = 0;

   /// get time for computing current solution.
   virtual double time() const = 0;
   //@}


   /**@name Loading LPs */
   //@{
   /// unload current problem and re-initialize lpsolver.
   virtual void clear() = 0;

   /// load LP from #filename# in MPS or LP format.
   virtual void readFile(char* filename) = 0;

   /// dump loaded LP to #filename# in LP format.
   virtual void dumpFile(char* filename) const = 0;
   //@}


   /**@name Adding Rows and Columns
       Each row and column in an #LPSolver# is associated to #RowId# and
       #ColId#, respectively, which remains unchanged, as long as the row or
       column remains in the loaded LP. Theses #RowId#s and #ColId#s are
       assigned by the #LPSolver# when rows and columns are added. Hence, all
       methods for adding rows and columns come with two signatures. One of
       them, provides a first parameter for returning the assigned #RowId#(s)
       or #ColId#(s), respectively.
    */
   //@{
   ///
   virtual void addRow(const LPRow& row) = 0;
   /// add #row# to #LPSolver#s LP.
   virtual void addRow(RowId& id, const LPRow& row) = 0;

   ///
   virtual void addRows(const LPRowSet& set) = 0;
   /// add all #LPRow#s of #set# to #LPSolver#s LP.
   virtual void addRows(RowId id[], const LPRowSet& set) = 0;

   ///
   virtual void addCol(const LPCol& col) = 0;
   /// add #col# to #LPSolver#s LP.
   virtual void addCol(ColId& id, const LPCol& col) = 0;

   ///
   virtual void addCols(const LPColSet& set) = 0;
   /// add all #LPCol#s of #set# to #LPSolver#s LP.
   virtual void addCols(ColId id[], const LPColSet& set) = 0;
   //@}


   /**@name Removing Rows and Columns
       Either single or multiple rows or columns may be removed in one method
       call. In general, both lead to renumbering of the remaining rows or
       columns.

       If only one row or column is removed at a time, the situation is simple.
       Either it is the last row or column, already, or the last one is moved
       to the  position (number) of the removed row or column.

       When multiple rows or columns are removed with one method invocation,
       the renumbering scheme is not specified. Instead, all such removal methods
       provide an additional parameter #perm#. If nonzero, #perm# must point to
       an array of #int#s of (at least) the size of the number of rows or
       columns. After termination, #perm[i]# is the permuted number of the former
       #i#-th row or column, or #<0#, if the #i#-th row or column has been
       removed.
    */
   //@{
   /// remove #i#-th row.
   virtual void removeRow(int i) = 0;
   /// remove row with #RowId id#.
   virtual void removeRow(RowId id) = 0;

   /// remove #i#-th column.
   virtual void removeCol(int i) = 0;
   /// remove column with #ColId id#.
   virtual void removeCol(ColId id) = 0;

   /// remove #n# rows.
   virtual void removeRows(RowId id[], int n, int perm[] = 0) = 0;
   /// remove #n# rows.
   virtual void removeRows(int nums[], int n, int perm[] = 0) = 0;

   /** Remove multiple rows.
       Remove all #LPRow#s with an number #i# such that #perm[i] < 0#. Upon
       completion, #perm[i] >= 0# indicates the new number where the #i#-th
       #LPRow# has been moved to due to this removal. Note, that #perm#
       must point to an array of at least #rowNumber()# #int#s.
    */
   virtual void removeRows(int perm[]) = 0;
   /// remove rows from #start# to #end# (including both).
   virtual void removeRowRange(int start, int end, int perm[] = 0) = 0;

   /// Remove #n# columns.
   virtual void removeCols(ColId id[], int n, int perm[] = 0) = 0;
   /// Remove #n# columns.
   virtual void removeCols(int nums[], int n, int perm[] = 0) = 0;

   /** Remove multiple columns.
       Remove all #LPCol#s with an number #i# such that #perm[i] < 0#. Upon
       completion, #perm[i] >= 0# indicates the new number where the #i#-th
       #LPCol# has been moved to due to this removal. Note, that #perm#
       must point to an array of at least #colNumber()# #int#s.
    */
   virtual void removeCols(int perm[]) = 0;
   /// remove columns from #start# to #end# (including both).
   virtual void removeColRange(int start, int end, int perm[] = 0) = 0;
   //@}

   /**@name Manipulating the LP */
   //@{
   /// change objective vector.
   virtual void changeObj(const Vector& newObj) = 0;

   /// change #i#-th objective value.
   virtual void changeObj(int i, double newVal) = 0;

   /// change #id#-th objective value.
   virtual void changeObj(ColId id, double newVal) = 0;

   /// change vector of lower bounds.
   virtual void changeLower(const Vector& newLower) = 0;

   /// change #i#-th lower bound.
   virtual void changeLower(int i, double newLower) = 0;

   /// change #id#-th lower bound.
   virtual void changeLower(ColId id, double newLower) = 0;

   /// change vector of upper bounds.
   virtual void changeUpper(const Vector& newUpper) = 0;

   /// change #i#-th upper bound.
   virtual void changeUpper(int i, double newUpper) = 0;

   /// change #id#-th upper bound.
   virtual void changeUpper(ColId id, double newUpper) = 0;

   /// change vector of upper bounds.
   virtual void changeBounds(const Vector& newLower, const Vector& newUpper) = 0;

   /// change #i#-th upper bound.
   virtual void changeBounds(int i, double newLower, double newUpper) = 0;

   /// change #id#-th upper bound.
   virtual void changeBounds(ColId id, double newLower, double newUpper) = 0;

   /// change lhs vector for constraints.
   virtual void changeLhs(const Vector& newLhs) = 0;

   /// change #i#-th lhs value.
   virtual void changeLhs(int i, double newLhs) = 0;

   /// change #id#-th lhs value.
   virtual void changeLhs(RowId id, double newLhs) = 0;

   /// change rhs vector for constraints.
   virtual void changeRhs(const Vector& newRhs) = 0;

   /// change #i#-th rhs value.
   virtual void changeRhs(int i, double newRhs) = 0;

   /// change #id#-th rhs value.
   virtual void changeRhs(RowId id, double newRhs) = 0;

   /// change lhs and rhs vectors for constraints.
   virtual void changeRange(const Vector& newLhs, const Vector& newRhs) = 0;

   /// change #i#-th rhs value.
   virtual void changeRange(int i, double newLhs, double newRhs) = 0;

   /// change #id#-th rhs value.
   virtual void changeRange(RowId id, double newLhs, double newRhs) = 0;


   /// change #i#-th row of LP.
   virtual void changeRow(int i, const LPRow& newRow) = 0;

   /// change #id#-th row of LP.
   virtual void changeRow(RowId id, const LPRow& newRow) = 0;

   /// change #i#-th column of LP.
   virtual void changeCol(int i, const LPCol& newCol) = 0;

   /// change #id#-th column of LP.
   virtual void changeCol(ColId id, const LPCol& newCol) = 0;

   /// change LP element (#i#, #j#).
   virtual void changeElement(int i, int j, double val) = 0;

   /// change LP element (#rid#, #cid#).
   virtual void changeElement(RowId rid, ColId cid, double val) = 0;

   /// change optimization sense to #sns#.
   virtual void changeSense(Sense sns) = 0;

   //@}


   /**@name Accessing Loaded LP */
   //@{
   /// get #i#-th row.
   virtual void getRow(int i, LPRow& row) const = 0;

   /// get #id#-th row.
   virtual void getRow(RowId id, LPRow& row) const = 0;

   /// get rows #start# .. #end#.
   virtual void getRows(int start, int end, LPRowSet& set) const = 0;

   /// return const #i#-th row.
   virtual const SVector& rowVector(int i) const = 0;

   /// return const #id#-th row.
   virtual const SVector& rowVector(RowId id) const = 0;

   /// return const lp's rows.
   virtual const LPRowSet& rows() const = 0;

   /// get #i#-th column.
   virtual void getCol(int i, LPCol& column) const = 0;

   /// get #id#-th column.
   virtual void getCol(ColId id, LPCol& column) const = 0;

   /// get columns #start# .. #end#.
   virtual void getCols(int start, int end, LPColSet& set) const = 0;

   /// return const #i#-th column.
   virtual const SVector& colVector(int i) const = 0;

   /// return const #id#-th column.
   virtual const SVector& colVector(ColId id) const = 0;

   /// return const lp's columns.
   virtual const LPColSet& cols() const = 0;


   /// #i#-th lhs value.
   virtual double lhs(int i) const = 0;

   /// #id#-th lhs value.
   virtual double lhs(RowId id) const = 0;

   /// copy lhs value vector to #lhs#.
   virtual void getLhs(Vector& lhs) const = 0;

   /// return const lhs vector.
   virtual const Vector& lhs() const = 0;

   /// #i#-th rhs value.
   virtual double rhs(int i) const = 0;

   /// #id#-th rhs value.
   virtual double rhs(RowId id) const = 0;

   /// copy rhs value vector to #rhs#.
   virtual void getRhs(Vector& rhs) const = 0;

   /// return const rhs vector.
   virtual const Vector& rhs() const = 0;

   /// #i#-th value of objective vector.
   virtual double obj(int i) const = 0;

   /// #id#-th value of objective vector.
   virtual double obj(ColId id) const = 0;

   /// copy objective vector to #obj#.
   virtual void getObj(Vector& obj) const = 0;

   /// return const objective vector.
   virtual const Vector& obj() const = 0;

   /// #i#-th lower bound.
   virtual double lower(int i) const = 0;

   /// #id#-th lower bound.
   virtual double lower(ColId id) const = 0;

   /// copy lower bound vector to #low#.
   virtual void getLower(Vector& low) const = 0;

   /// return const lower bound vector.
   virtual const Vector& lower() const = 0;

   /// #i#-th upper bound.
   virtual double upper(int i) const = 0;

   /// #id#-th upper bound.
   virtual double upper(ColId id) const = 0;

   /// copy upper bound vector to #up#.
   virtual void getUpper(Vector& up) const = 0;

   /// return const upper bound vector.
   virtual const Vector& upper() const = 0;

   /// optimization sense.
   virtual Sense sense() const = 0;
   //@}

   /**@name Inquiry */
   //@{
   /// solvers #Status#.
   virtual Status status() const = 0;

   /// number of columns of loaded LP.
   virtual int nofCols() const = 0;
   /// number of rows of loaded LP.
   virtual int nofRows() const = 0;
   /// number of nonzero coefficients of loaded LP.
   virtual int nofNZEs() const = 0;

   /// number of row #id#.
   virtual int number(RowId id) const = 0;
   /// number of column #id#.
   virtual int number(ColId id) const = 0;

   /// #RowId# of #i#-th inequality.
   virtual RowId rowId(int i) const = 0;
   /// #ColId# of #i#-th column.
   virtual ColId colId(int i) const = 0;

   /// test whether #LPSolver# has row with #id#.
   virtual int has(RowId id) const
   {
      return number(id) >= 0;
   }
   /// test whether #LPSolver# has column with #id#.
   virtual int has(ColId id) const
   {
      return number(id) >= 0;
   }

   /// get row ids.
   virtual void getRowIds(RowId ids[]) const = 0;

   /// get column ids.
   virtual void getColIds(ColId ids[]) const = 0;
   //@}

   /**@name Miscellaneous */
   //@{
   /// destructor.
   virtual ~LPSolver()
   {}
   //@}
}
;

} // namespace soplex
#endif // _LPSOLVER_H_

//-----------------------------------------------------------------------------
//Emacs Local Variables:
//Emacs mode:c++
//Emacs c-basic-offset:3
//Emacs tab-width:8
//Emacs indent-tabs-mode:nil
//Emacs End:
//-----------------------------------------------------------------------------

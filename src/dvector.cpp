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
#pragma ident "@(#) $Id: dvector.cpp,v 1.5 2001/12/10 22:41:57 bzfbleya Exp $"


/*  \Section{Complex Methods}
 */
#include "dvector.h"
#include "spxalloc.h"

namespace soplex
{


/* \SubSection{Maths Operators}
 */
DVector operator+(const Vector& v, const Vector& w)
{
   assert(v.dim() == v.dim());
   DVector res(v.dim());
   for (int i = 0; i < res.dim(); ++i)
      res[i] = v[i] + w[i];
   return res;
}

DVector operator+(const Vector& v, const SVector& w)
{
   DVector res(v);
   res += w;
   return res;
}

DVector operator-(const Vector& vec)
{
   DVector res(vec.dim());
   for (int i = 0; i < res.dim(); ++i)
      res[i] = -vec[i];
   return res;
}

DVector operator-(const Vector& v, const Vector& w)
{
   assert(v.dim() == w.dim());
   DVector res(v.dim());
   for (int i = 0; i < res.dim(); ++i)
      res[i] = v[i] - w[i];
   return res;
}

DVector operator-(const Vector& v, const SVector& w)
{
   DVector res(v);
   res -= w;
   return res;
}

DVector operator-(const SVector& v, const Vector& w)
{
   DVector res(-w);
   res += v;
   return res;
}

DVector operator*(const Vector& v, double x)
{
   DVector res(v.dim());
   for (int i = 0; i < res.dim(); ++i)
      res[i] = x * v[i];
   return res;
}

void DVector::reSize(int newsize)
{
   assert(newsize >= dim());

   spx_realloc(mem, ((newsize > 0) ? newsize : 1));

   val = mem;
   memsize = newsize;
}

void DVector::reSize(int newsize, int newdim)
{
   assert(newsize >= newdim);
   
   spx_realloc(mem, ((newsize > 0) ? newsize : 1));

   val = mem;
   memsize = newsize;
   dimen = newdim;
}

void DVector::reDim(int newdim)
{
   if ( /*newdim > memsize && */ newdim >= dim() )
      reSize(int(newdim + 0.2 * memsize));
   // Seems lint is wrong here. Replace with memset anyway.
   for (int i = dimen; i < newdim; i++)
      mem[i] = 0;
   dimen = newdim;
}

std::istream& operator>>(std::istream& s, DVector& vec)
{
   char c;
   double val;
   int i = 0;

   while (s.get(c).good())
      if (c != ' ' && c != '\t' && c != '\n')
         break;

   if (c != '(')
      s.putback(c);

   else
   {
      do
      {
         s >> val;
         if (i >= vec.dim() - 1)
            vec.reDim(i + 16);
         vec[i++] = val;
         while (s.get(c).good())
            if (c != ' ' && c != '\t' && c != '\n')
               break;
         if (c != ',')
         {
            if (c != ')')
               s.putback(c);
            break;
         }
      }
      while (s.good());
   }

   vec.reDim(i);
   return s;
}

DVector::DVector(const Vector& old)
   : Vector(0, 0)
{
   dimen = old.dim();
   memsize = dimen;
   spx_alloc(mem, memsize);
   val = mem;
   *this = old;
}

DVector::DVector(const DVector& old)
   : Vector(0, 0)
{
   dimen = old.dim();
   memsize = old.memsize;
   spx_alloc(mem, memsize);
   val = mem;
   *this = old;
}

DVector::DVector(int p_dim)
   : Vector(0, 0)
{
   memsize = (p_dim > 0) ? p_dim : 4;
   spx_alloc(mem, memsize);
   val = mem;
   dimen = p_dim;
   /*
       for(int i = 0; i < memsize; ++i)
           mem[i] = 0;
   */
}

DVector::~DVector()
{
   spx_free(mem);
}

int DVector::isConsistent() const
{
   if (val != mem || dimen > memsize || dimen < 0)
   {
      std::cerr << "ERROR: inconsistency detected in class DVector\n";
      return 0;
   }
   return Vector::isConsistent();
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

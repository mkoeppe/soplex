# $Id: Makefile,v 1.3 2001/11/06 23:30:51 bzfkocht Exp $
#* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#*                                                                           *
#*   File....: Makefile                                                      *
#*   Name....: SoPlex Makefile                                               *
#*   Author..: Thorsten Koch                                                 *
#*   Copyright by Author, All rights reserved                                *
#*                                                                           *
#* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

.PHONY:		depend clean lint check quick cover

ARCH            :=      $(shell uname -m | \
                        sed -e s/sun../sparc/ -e s/i.86/x86/)
OSTYPE		:=	$(shell uname -s | tr A-Z a-z)

OPT		=	dbg

COMP		=	gnu
CXX		=	g++
DCXX		=	g++
LINT		=	lintgcc
AR		=	ar
RANLIB		=	ranlib
DOCXX		=	docxx

CPPFLAGS	=	-Isrc
CXXFLAGS	=	-O
ARFLAGS		=	cr
DFLAGS		=	-MM

NAME		=	soplex
FLAGS		=       #
LIBOBJ		= 	cachelpsolver.o changesoplex.o didxset.o \
			docupdate.o dsvector.o dvector.o enter.o factor.o \
			forest.o idxset.o leave.o lpcolset.o lprowset.o \
			lprow.o nameset.o slufactor.o solve.o soplex.o \
			spxaggregatesm.o spxbasis.o spxbounds.o \
			spxchangebasis.o spxdefaultpr.o spxdefaultrt.o \
			spxdesc.o spxdevexpr.o spxfastrt.o spxgeneralsm.o \
			spxharrisrt.o spxhybridpr.o spxio.o spxlp.o spxmps.o \
			spxparmultpr.o spxredundantsm.o spxrem1sm.o \
			spxscale.o spxshift.o spxsolver.o spxsolve.o \
			spxstarter.o spxsteeppr.o spxsumst.o spxvecs.o \
			spxvectorst.o spxweightpr.o spxweightst.o \
			ssvector.o subsvector.o svector.o \
			svset.o timer.o unitvector.o update.o updatevector.o \
			vector.o vector_c.o vsolve.o

OBJECT		=	example.o

#------------------------------------------------------------------------------
#--- NOTHING TO CHANGE FROM HERE ON -------------------------------------------
#------------------------------------------------------------------------------

GCCWARN		=	-Wall -W -Wpointer-arith -Wbad-function-cast \
			-Wcast-align -Wwrite-strings -Wconversion \
			-Wstrict-prototypes -Wmissing-prototypes \
			-Wmissing-declarations -Wno-unknown-pragmas \
			-Wctor-dtor-privacy -Wnon-virtual-dtor -Wreorder \
			-Woverloaded-virtual -Wsign-promo -Wsynth -Wundef \
			-Wcast-qual -Wold-style-cast -Wshadow 
#			-Weffc++ -Wredundant-decls    

#-----------------------------------------------------------------------------
include make/make.$(OSTYPE).$(ARCH).$(COMP).$(OPT)
#-----------------------------------------------------------------------------

TARGET		=	bin/$(NAME).$(OSTYPE).$(ARCH).$(COMP).$(OPT)
LIBRARY		=	lib/lib$(NAME).$(OSTYPE).$(ARCH).$(COMP).$(OPT).a
DEPEND		=	src/depend

OBJDIR		=	O.$(OSTYPE).$(ARCH).$(COMP).$(OPT)
SRCDIR		=	src
BINDIR		=	bin
LIBDIR		=	lib
OBJXXX		=	$(addprefix $(OBJDIR)/,$(OBJECT))
LIBXXX		=	$(addprefix $(OBJDIR)/,$(LIBOBJ))
OBJSRC		=	$(addprefix $(SRCDIR)/,$(OBJECT:.o=.cpp))
LIBSRC		=	$(addprefix $(SRCDIR)/,$(LIBOBJ:.o=.cpp))

$(TARGET):	$(OBJDIR) $(BINDIR) $(OBJXXX) $(LIBRARY) 
		$(CXX) $(CXXFLAGS) $(OBJXXX) $(LIBRARY) $(LDFLAGS) -o $@

$(LIBRARY):	$(LIBDIR) $(LIBXXX) 
		-rm $(LIBRARY)
		$(AR) $(ARFLAGS) $@ $(LIBXXX) 
		$(RANLIB) $@

lint:		$(OBJSRC) $(LIBSRC)
		$(LINT) src/project.lnt -os\(src/lint.out\) \
		$(CPPFLAGS) -UNDEBUG $^

#doc:		$(DOCDIR)/soplex.dvi
#
#html:		$(DOCSRC)
#		-@mkdir $(HTMLDIR)
#		$(DOCXX) -d $(HTMLDIR) $(DOCSRC)
check:		
		cd check; ./check.sh normal.test $(TARGET)

quick:		
		cd check; ./check.sh quick.test $(TARGET)

cover:
		cd check; ./cover.sh cover.test $(TARGET)

clean:
		-rm -rf $(OBJDIR)/* $(LIBRARY) $(TARGET)

$(OBJDIR):	
		-mkdir $(OBJDIR)

$(LIBDIR):
		-mkdir $(LIBDIR)

$(BINDIR):
		-mkdir $(BINDIR)

depend:
		$(SHELL) -ec '$(DCXX) $(DFLAGS) $(CPPFLAGS) \
		$(OBJSRC:.o=.cpp) $(LIBSRC:.o=.cpp) \
		| sed '\''s/^\([0-9A-z]\{1,\}\)\.o/$(OBJDIR:/=\/)\/\1.o/g'\'' \
		>$(DEPEND)'

include		$(DEPEND)

$(OBJDIR)/%.o:	$(SRCDIR)/%.cpp
		$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@








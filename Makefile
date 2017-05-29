
MAKEFLAGS+=-r --no-builtin-rules -R --no-builtin-variables

ifndef DEBUG
HIDE=@
endif

C=gcc-7
CXX=gcc-7

CFLAGS=\
  -pg \
  -g \
  -O3 \
  -march=native \
  -Wall \
  -Werror \
  -Wfatal-errors \
  -Wpedantic \
  -Wextra \
  -Wshadow \
  -fno-strict-aliasing \
  -pedantic \
  -pedantic-errors
CXXFLAGS=$(CFLAGS) \
  -std=c++14
LDFLAGS=-pg
LDXXFLAGS=$(LDFLAGS) -lstdc++

SRCDIR=src
BUILDDIR=build
BINDIR=$(BUILDDIR)/bin
OBJDIR=$(BUILDDIR)/obj

CSRC=$(wildcard $(SRCDIR)/*.c)
COBJS=$(addprefix $(OBJDIR)/,$(notdir $(CSRC:.c=.obj)))
CPPSRC=$(wildcard $(SRCDIR)/*.cpp)
CPPOBJS=$(addprefix $(OBJDIR)/,$(notdir $(CPPSRC:.cpp=.o)))

HEADERS=$(wildcard $(SRCDIR)/*.h)
APP=$(BINDIR)/simulate
APP_C=$(BINDIR)/simulate_c
APPOPTS=assets/octave/octave.png

LIBS=
INCLUDES=-Istb
FRAMEWORKS=
TIDY_APP=clang-format
GIT=git

.PHONY: run
run: all
	@echo [run] $(APP)
	$(HIDE)$(APP) $(APPOPTS)

.PHONY: all
all: status $(APP) $(APP_C) ;
Makefile: ;

.PHONY: status
status:
	@echo [git-status]
	$(HIDE)$(GIT) status -s

.PHONY: clean
clean:
	-rm $(COBJS)
	-rm $(CPPOBJS)
	-rmdir $(OBJDIR)
	-rm $(APP)
	-rm $(APP_C)
	-rmdir $(BINDIR)
	-rmdir -p $(BUILDDIR)

$(APP_C): $(COBJS) Makefile | $(BINDIR)
	@echo [link] $@
	$(HIDE)$(C) $(LDFLAGS) $(COBJS) -o $@ $(LIBS) $(FRAMEWORKS)

$(APP): $(CPPOBJS) Makefile | $(BINDIR)
	@echo [link] $@
	$(HIDE)$(CXX) $(LDXXFLAGS) $(CPPOBJS) -o $@ $(LIBS) $(FRAMEWORKS)

.SUFFIXES:.c .obg .c.tidy
$(COBJS) : $(OBJDIR)/%.obj : $(SRCDIR)/%.c | $(OBJDIR)
	@echo [tidy] $<
	$(HIDE)$(TIDY_APP) -i $<
	@echo [c] $<
	$(HIDE)time $(C) $(CFLAGS) $(INCLUDES) -c $< -o $@

.SUFFIXES:.cpp .h .o .cpp.tidy .h.tidy
$(CPPOBJS) : $(OBJDIR)/%.o : $(SRCDIR)/%.cpp | $(OBJDIR)
	@echo [tidy] $<
	$(HIDE)$(TIDY_APP) -i $<
	@echo [c++] $<
	$(HIDE)time $(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BINDIR) $(OBJDIR):
	@echo [mkdir] $(@)
	$(HIDE)mkdir -p $(@)

.PHONY: tidy
tidy: $(addsuffix .tidy,$(SRC) $(HEADERS)) status ;

%.c.tidy: %.c
	@echo [tidy] $<
	$(HIDE)$(TIDY_APP) -i $<

%.cpp.tidy: %.cpp
	@echo [tidy] $<
	$(HIDE)$(TIDY_APP) -i $<

%.h.tidy: %.h
	@echo [tidy] $<
	$(HIDE)$(TIDY_APP) -i $<

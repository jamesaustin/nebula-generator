
MAKEFLAGS+=-r --no-builtin-rules -R --no-builtin-variables

ifndef DEBUG
HIDE=@
endif

CXX=gcc-7
CXXFLAGS=\
  -g \
  -O3 \
  -std=c++14 \
  -march=native \
  -Wall \
  -Werror \
  -Wfatal-errors \
  -Wpedantic \
  -Wextra \
  -Wshadow \
  -fno-strict-aliasing
LDFLAGS=-lstdc++
TIDY_APP=clang-format
GIT=git

SRCDIR=src
BUILDDIR=build
BINDIR=$(BUILDDIR)/bin
OBJDIR=$(BUILDDIR)/obj

SRC=$(wildcard $(SRCDIR)/*.cpp)
HEADERS=$(wildcard $(SRCDIR)/*.h)
OBJS=$(addprefix $(OBJDIR)/,$(notdir $(SRC:.cpp=.o)))
APP=$(BINDIR)/simulate
APPOPTS=assets/octave/octave.png

LIBS=
INCLUDES=-Istb
FRAMEWORKS=

.PHONY: run
run: all
	@echo [run] $(APP)
	$(HIDE)$(APP) $(APPOPTS)

.PHONY: all
all: status $(APP) ;
Makefile: ;

.PHONY: status
status:
	@echo [git-status]
	$(HIDE)$(GIT) status -s

.PHONY: clean
clean:
	-rm $(OBJS)
	-rmdir $(OBJDIR)
	-rm $(APP)
	-rmdir $(BINDIR)
	-rmdir -p $(BUILDDIR)

$(APP): $(OBJS) Makefile | $(BINDIR)
	@echo [link] $@
	$(HIDE)$(CXX) $(LDFLAGS) $(OBJS) -o $@ $(LIBS) $(FRAMEWORKS)

.SUFFIXES:.cpp .h .o .cpp.tidy .h.tidy
$(OBJS) : $(OBJDIR)/%.o : $(SRCDIR)/%.cpp | $(OBJDIR)
	@echo [tidy] $<
	$(HIDE)$(TIDY_APP) -i $<
	@echo [c++] $<
	$(HIDE)$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BINDIR) $(OBJDIR):
	@echo [mkdir] $(@)
	$(HIDE)mkdir -p $(@)

.PHONY: tidy
tidy: $(addsuffix .tidy,$(SRC) $(HEADERS)) status ;

%.cpp.tidy: %.cpp
	@echo [tidy] $<
	$(HIDE)$(TIDY_APP) -i $<

%.h.tidy: %.h
	@echo [tidy] $<
	$(HIDE)$(TIDY_APP) -i $<

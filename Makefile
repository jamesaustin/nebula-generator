
MAKEFLAGS+=-r --no-builtin-rules -R --no-builtin-variables

ifndef DEBUG
HIDE=@
endif

CXX=gcc-7
CXXFLAGS=\
  -g \
  -O3 \
  -std=c++14 \
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


APP=simulate
APPOPTS=assets/octave/octave.png
SOURCES=$(wildcard *.cpp)
HEADERS=$(wildcard *.h)
OBJS=$(SOURCES:.cpp=.o)
LIBS=
INCLUDES=
FRAMEWORKS=

.PHONY: run
run: all
	@echo [run] $(APP)
	$(HIDE)./$(APP) $(APPOPTS)

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
	-rm $(APP)

$(APP): $(OBJS) Makefile
	@echo [app] $@
	$(HIDE)$(CXX) $(LDFLAGS) $(OBJS) -o $@ $(LIBS) $(FRAMEWORKS)

.SUFFIXES:.cpp .h .o .cpp.tidy .h.tidy
$(SOURCES:.cpp=.o) : %.o: %.cpp
	@echo [tidy] $<
	$(HIDE)$(TIDY_APP) -i $<
	@echo [c++] $<
	$(HIDE)$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: tidy
tidy: $(addsuffix .tidy,$(SOURCES) $(HEADERS)) status ;

%.cpp.tidy: %.cpp
	@echo [tidy] $<
	$(HIDE)$(TIDY_APP) -i $<

%.h.tidy: %.h
	@echo [tidy] $<
	$(HIDE)$(TIDY_APP) -i $<

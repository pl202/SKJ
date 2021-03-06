all: ioskj.exe

.PHONY: docs tests requires

clean:
	rm -f *.debug *.exe *.o

# Define operating system
UNAME := $(shell uname -o)
ifeq ($(UNAME), GNU/Linux)
	OS := linux
endif
ifeq ($(UNAME), Msys)
	OS := win
endif

# Define compile options and required libraries
CXX_FLAGS := -std=c++11 -Wall -Wno-unused-function -Wno-unused-local-typedefs
ifeq ($(OS),win)
	# Static library linking on Windows
	CXX_FLAGS += -static
	# Increase stack size to 4Mb on Windows
	# We are allocating quite a lot of memory in the top level "task"
	# functions (in particular Data) - could be refectored but this is
	# easier (and safer) for now
	CXX_FLAGS += -Wl,--stack,4194304
endif
INC_DIRS := -I. -Irequires/stencila/cpp -Irequires/boost-$(OS)
LIB_DIRS := -Lrequires/boost-$(OS)/lib
LIBS := -lboost_system -lboost_filesystem -lboost_regex

# Find all .hpp and .cpp files (to save time don't recurse into subdirectories)
HPPS := $(shell find . -maxdepth 1 -name "*.hpp")
CPPS := $(shell find . -maxdepth 1 -name "*.cpp")

#############################################################
# Executables

# Executable for normal use
ioskj.exe: $(HPPS) $(CPPS)
	$(CXX) $(CXX_FLAGS) -O3 $(INC_DIRS) -o$@ ioskj.cpp $(LIB_DIRS) $(LIBS)

# Executable for debugging
ioskj.debug: $(HPPS) $(CPPS)
	$(CXX) $(CXX_FLAGS) -g -O0 $(INC_DIRS) -o$@ ioskj.cpp $(LIB_DIRS) $(LIBS)

# Executable for profiling
ioskj.prof: $(HPPS) $(CPPS)
	$(CXX) $(CXX_FLAGS) -pg -O3 $(INC_DIRS) -o$@ ioskj.cpp $(LIB_DIRS) $(LIBS)

# Executable for tests
tests.exe: tests.cpp
	$(CXX) $(CXX_FLAGS) -O3 $(INC_DIRS) -otests.exe tests.cpp $(LIB_DIRS) $(LIBS) -lboost_unit_test_framework

#############################################################
# Tasks

# Compile main executable
compile: ioskj.exe

# Run main executable
run: ioskj.exe
	./ioskj.exe

# Run profiling executable
profile: ioskj.prof
	./ioskj.prof
	gprof ioskj.prof gmon.out > profiling.txt

# Run the tests
test: tests.exe
	(./tests.exe) || (exit 1)

#############################################################
# R driver

drive:
	Rscript driver.r

#############################################################
# Package

package:
	rm -rf ioskj
	mkdir -p ioskj ioskj/parameters ioskj/data ioskj/procedures ioskj/feasible/output
	cp ioskj.exe ioskj
	cp ioskj.r ioskj
	cp ioskj-drive.r ioskj
	cp -r parameters/input ioskj/parameters/input
	cp -r data/input ioskj/data/input
	cp -r procedures/input ioskj/procedures/input
	cp feasible/output/accepted.tsv ioskj/feasible/output/accepted.tsv
ifeq ($(UNAME), GNU/Linux)
	tar zcf ioskj-linux.tar.gz ioskj
else
	zip -r ioskj-$(OS).zip ioskj
endif

#############################################################
# Documentation

# Generate Doxygen documentation
doxygen-html:
	doxygen doxygen/Doxyfile

# Stencil list for rendering
STENCILS := \
	model/description parameters/description procedures/description \
	model/display yield/display \
	feasible/display ss3/display evaluate/display
# Stencil output list
STENCILS_HTML := $(patsubst %,%/index.html,$(STENCILS))
# Stencil `.pages` folders list
STENCILS_PAGES := $(patsubst %,.pages/%,$(STENCILS))

# Render a stencil: Cila -> HTML
%/index.html: %/stencil.cila
	stencila-r $(dir $<) render export:$(dir $<)index.html
# Render a stencil: Cila -> Word
%/index.docx: %/stencil.cila
	cd $(dir $<) && Rscript -e "require(stencila); Stencil('.')$$ render()$$ export('index.docx')"
# Copy a stencil to `.pages`
.pages/%: %/index.html
	mkdir -p $@  ; cp -fr $(dir $<). $@

# Render all stencils
stencils-html: $(STENCILS_HTML)

# Copy all stencils to `.pages` folder in preparation for publishing
stencils-pages: $(STENCILS_PAGES)

# Generate all documentation
docs: doxygen-html stencils-html

# Publish documentation
# Aggregates alternative documentation into a single place for publishing using Github pages 
# Requires the a branch called "gh-pages":
#	git branch gh-pages
# and the "ghp-import" script
# 	sudo pip install ghp-import
publish:
	mkdir -p .pages && cp reports/final-report-draft.pdf .pages/final-report-draft.pdf
	mkdir -p .pages/doxygen  ; cp -fr doxygen/html/. .pages/doxygen/
	ghp-import -m "Updated pages" -p .pages

#############################################################
# Dynamic presentations in `presents` folder
# 
# Use by appending `-deploy` etc after directory name e.g.
#    make presents/2016-02-03-maldives/mp-brule-deploy

%-serve: %/.
	stencila-r $(dir $<) render serve ...

%-view: %/.
	stencila-r $(dir $<) render view ...

# Create a symlink to the local executable and directories
%-localize: %/.
	rm -rf $(dir $<)ioskj*
	ln -sfT  $(abspath $(shell pwd)) $(dir $<)ioskj

# Put the latest version of the package, commit and push
%-deploy: %/. package
	cp ioskj-linux.tar.gz $(dir $<)
	cd $(dir $<) && git add "ioskj-linux.tar.gz"
	cd $(dir $<) && git add "stencil.cila"
	-cd $(dir $<) && git commit -m "Update ioskj package"
	cd $(dir $<) && git push

#############################################################
# Requirements

BOOST_VERSION := 1_60_0

requires/boost_$(BOOST_VERSION).tar.bz2:
	@mkdir -p requires
	wget --no-check-certificate -O $@ http://prdownloads.sourceforge.net/boost/boost_$(BOOST_VERSION).tar.bz2

requires/boost-$(OS): requires/boost_$(BOOST_VERSION).tar.bz2
	tar --bzip2 -xf $< -C requires
	mv requires/boost_$(BOOST_VERSION) requires/boost-$(OS)
	touch $@


BOOST_BOOTSTRAP_FLAGS := --with-libraries=filesystem,regex,test
BOOST_B2_FLAGS := -d0 --prefix=. link=static install
ifeq ($(OS), linux)
	BOOST_B2_FLAGS += cxxflags=-fPIC
endif
ifeq ($(OS), win)
	BOOST_BOOTSTRAP_FLAGS += --with-toolset=mingw
	BOOST_B2_FLAGS += --layout=system release toolset=gcc
endif

requires/boost-$(OS).flag: requires/boost-$(OS)
	cd $< ; ./bootstrap.sh $(BOOST_BOOTSTRAP_FLAGS)
	sed -i "s/mingw/gcc/g" $</project-config.jam
	cd $< ; ./b2 $(BOOST_B2_FLAGS)
	touch $@

requires-boost: requires/boost-$(OS).flag

STENCILA_VERSION := 0.13

requires/stencila-$(STENCILA_VERSION).zip:
	@mkdir -p requires
	wget --no-check-certificate -O $@ https://github.com/stencila/stencila/archive/$(STENCILA_VERSION).zip

requires/stencila: requires/stencila-$(STENCILA_VERSION).zip
	rm -rf requires/stencila
	unzip $< -d requires
	mv requires/stencila-$(STENCILA_VERSION) requires/stencila
	touch $@

requires-stencila: requires/stencila

requires: requires-boost requires-stencila


#############################################################
# Running on AWS EC2
#
# 1.Launch an instance
#
# 2.Package up the binary and copy it to the instance
# 	make package
# 	scp -i <path/to/aws/key> ioskj-linux.zip ubuntu@<ip.address.of.instance>:~
#
# 3.Shell into the instance and unzip the package
# 	ssh -i <path/to/aws/key> ubuntu@<ip.address.of.instance>
#	unzip ioskj-linux.zip
# 
# 4.Copy package to a serparate directory and run a long MCMC chain
# there
#	cp -r ioskj ioskj1
#	cd ioskj1 && ./ioskj.exe condition_demc 10000000 &
#	..repeat..
#
# 5. To stop all chains and zip up the results
# 	killall -15 ioskj.exe
# 	tar -zcvf ioskj1.tar.gz ioskj1
# 	..repeat..
# 	
# 6. On localhost download those files
# 	scp -i <path/to/aws/key> ubuntu@<ip.address.of.instance>:~/ioskj1.tar.gz demc
# 	..repeat..
#
#Sorry, this is just a quick hack makefile.
#It assumes the adapter compiled under ../build

ADAPTER_INSTALL_DIR=../../build

CXX_FLAGS += -I../../include
CXX_FLAGS += -g -Wall
LD_FLAGS += -L${ADAPTER_INSTALL_DIR} -Wl,-rpath=${ADAPTER_INSTALL_DIR},--enable-new-dtags
LD_FLAGS += -lControlSystemAdapter -lboost_thread -lboost_system

all: testIndependentControlCore

testIndependentControlCore: testIndependentControlCore.cpp IndependentControlCore.h
	g++ $< ${CXX_FLAGS} ${LD_FLAGS} -o $@

clean:
	rm -rf testIndependentControlCore

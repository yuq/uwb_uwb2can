#!makefile

CXX?=g++

all: uwb2can

test: algorithm.cpp test_algorithm.cpp
	$(CXX) $^ -o $@

uwb2can: can.cpp main.cpp
	$(CXX) $^ -o $@




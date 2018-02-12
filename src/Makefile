CXX = g++ --std=c++14 
CPPFLAGS = -g -Wall -O0 -I../include -L../src
COMP = $(CXX) $(CPPFLAGS) $^ -o $@

fagin_test : fagin_test.cc
	$(COMP)

unihash_test : unihash_test.o
	$(COMP)

larson_kalja_test : larson_kalja_test.o
	$(COMP)

test_unit : test_unit.o
	$(COMP)

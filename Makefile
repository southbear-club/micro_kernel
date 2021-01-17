all:
	g++ demo.cpp -o test_micro -std=c++14 -lpthread -lrt -ldl
clean:
	rm -rf test_micro
main:
	g++ `llvm-config --cxxflags --ldflags --libs --system-libs` uwulang.cpp -o uwulang

clean:
	rm output.o
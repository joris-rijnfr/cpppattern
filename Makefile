.PHONY: build clean

build:
	cmake -B build
	cmake --build build
	cp build/cpppattern_test .


clean:
	rm -rf build cpppattern_test

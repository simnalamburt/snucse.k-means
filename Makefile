CXX=g++
CXXFLAGS=-Wall -Wextra -O3 -std=c++0x

LIBS = -lrt
LDFLAGS = ${LIBS}

all: seq pthread

seq: kmeans_seq

kmeans_seq: kmeans_seq.o kmeans_main.o
	${CXX} $^ -o $@ ${LDFLAGS}

pthread: kmeans_pthread

kmeans_pthread: kmeans_pthread.o kmeans_main.o
	${CXX} $^ -o $@ ${LDFLAGS} -lpthread

run_all:
	./gen_data.py centroid 64 centroid.point
	./gen_data.py data 65536 data.point 64
	thorq --add kmeans_seq centroid.point data.point result_seq.class final_centroid_seq.point 1024
	thorq --add kmeans_pthread centroid.point data.point result_pthread.class final_centroid_par.point 1024
run_seq:
	./gen_data.py centroid 64 centroid.point
	./gen_data.py data 65536 data.point 64
	thorq --add kmeans_seq centroid.point data.point result_seq.class final_centroid_seq.point 1024
run:
	./gen_data.py centroid 64 centroid.point
	./gen_data.py data 65536 data.point 64
	thorq --add kmeans_pthread centroid.point data.point result_pthread.class final_centroid_par.point 1024

plot:
	./plot_data.py result centroid.point data.point result_seq.class result_seq.png
	./plot_data.py result centroid.point data.point result_pthread.class result_pthread.png

clean:
	rm -f kmeans_seq kmeans_pthread kmeans_main.o kmeans_seq.o kmeans_pthread.o centroid.point data.point final_centroid.point result.class task* result_seq.png result_pthread.png

.PHONY: all seq pthread clean test

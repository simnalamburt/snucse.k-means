TARGET=kmeans_seq kmeans_pthread

CPP=g++
CPPFLAGS=-Wall -Wextra -O3 -std=c++0x -mmmx -msse -msse2 -msse3 -msse4.1 -msse4.2
LDFLAGS = -lrt

all: $(TARGET)

kmeans_seq: kmeans_seq.o kmeans_main.o
	$(CPP) $^ -o $@ $(LDFLAGS)

kmeans_pthread: LDFLAGS += -lpthread

kmeans_pthread: kmeans_pthread.o kmeans_main.o
	$(CPP) $^ -o $@ $(LDFLAGS)

run_all:
	./gen_data.py centroid 64 centroid.point
	./gen_data.py data 65536 data.point 64
	thorq --add kmeans_seq centroid.point data.point result_seq.class final_centroid_seq.point 1024
	thorq --add kmeans_pthread centroid.point data.point result_pthread.class final_centroid_par.point 1024
run:
	./gen_data.py centroid 64 centroid.point
	./gen_data.py data 65536 data.point 64
	thorq --add kmeans_pthread centroid.point data.point result_pthread.class final_centroid_par.point 1024

test:
	@cmp --silent result_seq.class result_pthread.class || echo "Test Failed"
	./plot_data.py result centroid.point data.point result_seq.class result_seq.png
	./plot_data.py result centroid.point data.point result_pthread.class result_pthread.png

clean:
	rm -f $(TARGET) *.o

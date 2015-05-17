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
	bin/gen centroid 64 tmp/centroid.point
	bin/gen data 65536 tmp/data.point 64
	thorq --add kmeans_seq     tmp/centroid.point tmp/data.point tmp/result_seq.class     tmp/final_centroid_seq.point 1024
	thorq --add kmeans_pthread tmp/centroid.point tmp/data.point tmp/result_pthread.class tmp/final_centroid_par.point 1024
run:
	bin/gen centroid 64 tmp/centroid.point
	bin/gen data 65536 tmp/data.point 64
	thorq --add kmeans_pthread tmp/centroid.point tmp/data.point tmp/result_pthread.class tmp/final_centroid_par.point 1024

test:
	@cmp --silent tmp/result_seq.class tmp/result_pthread.class || echo "Test Failed"
	bin/plot result tmp/centroid.point tmp/data.point tmp/result_seq.class     tmp/result_seq.png
	bin/plot result tmp/centroid.point tmp/data.point tmp/result_pthread.class tmp/result_pthread.png

clean:
	rm -f $(TARGET) *.o

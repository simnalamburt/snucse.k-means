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

run_all: $(TARGET)
	bin/gen centroid 64 tmp/cen
	bin/gen data 65536 tmp/data 64
	thorq --add kmeans_seq     tmp/cen tmp/data tmp/result.ref tmp/final.ref 1024
	thorq --add kmeans_pthread tmp/cen tmp/data tmp/result     tmp/final     1024
run: kmeans_pthread
	bin/gen centroid 64 tmp/cen
	bin/gen data 65536 tmp/data 64
	thorq --add kmeans_pthread tmp/cen tmp/data tmp/result     tmp/final     1024

test:
	@cmp --silent tmp/result.ref tmp/result || echo "Test Failed"
	bin/plot result tmp/cen tmp/data tmp/result.ref tmp/result.ref.png
	bin/plot result tmp/cen tmp/data tmp/result     tmp/result.png

clean:
	rm -f $(TARGET) *.o

CPPFLAGS=-O3 -Wall -Wextra -std=c++0x
LDFLAGS=-lrt

all: main.cc
	g++ $(CPPFLAGS) -fopenmp    $^ $(LDFLAGS) -o tmp/bin
	g++ $(CPPFLAGS) -DReference $^ $(LDFLAGS) -o tmp/bin.ref

run_all:
	bin/gen centroid 64 tmp/cen
	bin/gen data 65536 tmp/data 64
	thorq --add --device gpu tmp/bin.ref tmp/cen tmp/data tmp/result.ref
	thorq --add --device gpu tmp/bin     tmp/cen tmp/data tmp/result
run:
	bin/gen centroid 64 tmp/cen
	bin/gen data 65536 tmp/data 64
	thorq --add --device gpu tmp/bin     tmp/cen tmp/data tmp/result

test:
	@cmp --silent tmp/result.ref tmp/result || echo "Test Failed"
	bin/plot result tmp/cen tmp/data tmp/result.ref tmp/result.ref.png
	bin/plot result tmp/cen tmp/data tmp/result     tmp/result.png

CPPFLAGS=-O3 -Wall -Wextra -std=c++0x
LDFLAGS=-lrt

all: bin
bin: main.cc
	g++ $(CPPFLAGS) $^ $(LDFLAGS) -o tmp/bin
run: bin
	bin/gen centroid 64 tmp/cen
	bin/gen data 262144 tmp/data 64
	thorq --add --device gpu tmp/bin tmp/cen tmp/data tmp/result

ref: ref.cc
	g++ $(CPPFLAGS) $^ $(LDFLAGS) -o tmp/ref
run_all: bin ref
	bin/gen centroid 16 tmp/cen
	bin/gen data 65536 tmp/data 16
	thorq --add --device gpu tmp/ref tmp/cen tmp/data tmp/result.ref
	thorq --add --device gpu tmp/bin tmp/cen tmp/data tmp/result
test:
	@cmp --silent tmp/result.ref tmp/result || echo "Test Failed"
	bin/plot result tmp/cen tmp/data tmp/result.ref tmp/result.ref.png
	bin/plot result tmp/cen tmp/data tmp/result     tmp/result.png

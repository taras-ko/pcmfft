CXXFLAGS = \
	-g \
	-mtune=native \
	-ffast-math \
	-fomit-frame-pointer \
	-dA -fverbose-asm \
	-DFIXED_POINT=16 -lm \
	-std=c++0x \
	-Isha1

SOURCES = \
	dejavu.c \
	getopt.c
#	kiss_fft.c \
#	database.c \
#	qsort.c \
#	sha1/sha1.cpp \
#	sha1/sha1_api.cpp

default:
	$(CXX) $(CXXFLAGS) -o dejavu $(SOURCES)

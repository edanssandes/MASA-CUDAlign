AUTOMAKE_OPTIONS = subdir-objects
SUBDIRS = $(LIBMASA_PATH)
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

bin_PROGRAMS = cudalign
cudalign_CXXFLAGS = $(CUDA_CFLAGS) $(COMMONFLAGS) -malign-double -fno-strict-aliasing  
cudalign_LDADD = $(CUDA_LIBS) $(COMMONFLAGS) ./src/CUDAligner.cu_o $(LIBMASA_PATH)/libmasa.a -lcuda -lcudart -lpthread

cudalign_SOURCES = \
./src/main.cpp \
./src/cuda_util.cpp \
./src/CUDAligner.cpp \
./src/CUDAligner.cu \
./src/CUDAlignerParameters.cpp 

noinst_HEADERS = \
./src/cuda_util.h \
./src/CUDAligner.hpp \
./src/CUDAlignerParameters.hpp 


NVCCFLAGS += -ftz=true -prec-sqrt=false -prec-div=false
NVCCFLAGS += --compiler-options -fno-strict-aliasing
#NVCCFLAGS += -Xptxas -dlcm=cg #disable L1 cache
NVCCFLAGS += $(COMMONFLAGS)


clean-local:
	rm -f ./src/*.cu_o

.cu.cu_o:
	$(NVCC) -o $@ -c $< $(NVCCFLAGS)

./src/CUDAligner.cu_o: ./src/CUDAligner.hpp
#./src/cuda_util.cu_o: ./src/cuda_util.h

dist-clone-link:
	@if test -L $(LIBMASA_PATH); then \
		mv $(LIBMASA_PATH) $(LIBMASA_PATH).link && \
		cp -r -L $(LIBMASA_PATH).link $(LIBMASA_PATH); \
	fi

dist-unclone-link:
	@if test -L $(LIBMASA_PATH).link; then \
		rm -rf $(LIBMASA_PATH) && \
		mv $(LIBMASA_PATH).link $(LIBMASA_PATH); \
	fi

dist-link: dist-clone-link dist dist-unclone-link

sw-regs:
	$(NVCC) --disable-warnings --ptxas-options=-v -o /tmp/gpu_sw.o -c $(srcdir)/src/CUDAligner.cu $(NVCCFLAGS)

sw-ptx:
	$(NVCC) --disable-warnings --ptx $(srcdir)/src/CUDAligner.cu $(NVCCFLAGS)

.PHONY: doxygen

doxygen:
	pushd . && cd doxygen && doxygen *.doxyfile && popd
	pushd . && cd $(LIBMASA_PATH)/doxygen && doxygen *.doxyfile && popd










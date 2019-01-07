################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/capture/MultiChVideo.cpp \
../src/capture/thread.cpp \
../src/capture/v4l2camera.cpp 

OBJS += \
./src/capture/MultiChVideo.o \
./src/capture/thread.o \
./src/capture/v4l2camera.o 

CPP_DEPS += \
./src/capture/MultiChVideo.d \
./src/capture/thread.d \
./src/capture/v4l2camera.d 


# Each subdirectory must supply rules for building sources it contributes
src/capture/%.o: ../src/capture/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/encTrans -I../src -I../src/core -I../src/nvMedia -I../include/osa -I../include/EGL -O3 -Xcompiler -fopenmp -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_50,code=sm_50 -m64 -odir "src/capture" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/encTrans -I../src -I../src/core -I../src/nvMedia -I../include/osa -I../include/EGL -O3 -Xcompiler -fopenmp --compile -m64 -ccbin aarch64-linux-gnu-g++  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



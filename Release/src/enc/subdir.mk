################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/enc/NvBuffer.cpp \
../src/enc/NvElement.cpp \
../src/enc/NvElementProfiler.cpp \
../src/enc/NvLogging.cpp \
../src/enc/NvV4l2Element.cpp \
../src/enc/NvV4l2ElementPlane.cpp \
../src/enc/NvVideoEncoder.cpp 

OBJS += \
./src/enc/NvBuffer.o \
./src/enc/NvElement.o \
./src/enc/NvElementProfiler.o \
./src/enc/NvLogging.o \
./src/enc/NvV4l2Element.o \
./src/enc/NvV4l2ElementPlane.o \
./src/enc/NvVideoEncoder.o 

CPP_DEPS += \
./src/enc/NvBuffer.d \
./src/enc/NvElement.d \
./src/enc/NvElementProfiler.d \
./src/enc/NvLogging.d \
./src/enc/NvV4l2Element.d \
./src/enc/NvV4l2ElementPlane.d \
./src/enc/NvVideoEncoder.d 


# Each subdirectory must supply rules for building sources it contributes
src/enc/%.o: ../src/enc/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/encTrans -I../src -I../src/core -I../src/cr_osa/inc -I../src/enc -O3 -Xcompiler -fopenmp -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_50,code=sm_50 -m64 -odir "src/enc" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/encTrans -I../src -I../src/core -I../src/cr_osa/inc -I../src/enc -O3 -Xcompiler -fopenmp --compile -m64 -ccbin aarch64-linux-gnu-g++  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



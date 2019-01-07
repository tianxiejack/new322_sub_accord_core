################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/gluWindow.cpp \
../src/main.cpp \
../src/main_cap.cpp \
../src/main_core.cpp \
../src/main_core_file.cpp \
../src/main_glu.cpp 

OBJS += \
./src/gluWindow.o \
./src/main.o \
./src/main_cap.o \
./src/main_core.o \
./src/main_core_file.o \
./src/main_glu.o 

CPP_DEPS += \
./src/gluWindow.d \
./src/main.d \
./src/main_cap.d \
./src/main_core.d \
./src/main_core_file.d \
./src/main_glu.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/encTrans -I../src -I../src/core -I../src/nvMedia -I../include/osa -I../include/EGL -O3 -Xcompiler -fopenmp -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_50,code=sm_50 -m64 -odir "src" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/encTrans -I../src -I../src/core -I../src/nvMedia -I../include/osa -I../include/EGL -O3 -Xcompiler -fopenmp --compile -m64 -ccbin aarch64-linux-gnu-g++  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



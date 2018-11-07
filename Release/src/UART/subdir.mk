################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/UART/uartApi.cpp 

OBJS += \
./src/UART/uartApi.o 

CPP_DEPS += \
./src/UART/uartApi.d 


# Each subdirectory must supply rules for building sources it contributes
src/UART/%.o: ../src/UART/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/encTrans -I../src -I../src/core -I../src/cr_osa/inc -O3 -Xcompiler -fopenmp -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_50,code=sm_50 -m64 -odir "src/UART" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/encTrans -I../src -I../src/core -I../src/cr_osa/inc -O3 -Xcompiler -fopenmp --compile -m64 -ccbin aarch64-linux-gnu-g++  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



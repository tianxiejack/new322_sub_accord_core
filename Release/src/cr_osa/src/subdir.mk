################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/cr_osa/src/osa.cpp \
../src/cr_osa/src/osa_buf.cpp \
../src/cr_osa/src/osa_eth_client.cpp \
../src/cr_osa/src/osa_eth_server.cpp \
../src/cr_osa/src/osa_event.cpp \
../src/cr_osa/src/osa_file.cpp \
../src/cr_osa/src/osa_i2c.cpp \
../src/cr_osa/src/osa_image_queue.cpp \
../src/cr_osa/src/osa_mbx.cpp \
../src/cr_osa/src/osa_msgq.cpp \
../src/cr_osa/src/osa_mutex.cpp \
../src/cr_osa/src/osa_pipe.cpp \
../src/cr_osa/src/osa_prf.cpp \
../src/cr_osa/src/osa_que.cpp \
../src/cr_osa/src/osa_rng.cpp \
../src/cr_osa/src/osa_sem.cpp \
../src/cr_osa/src/osa_thr.cpp \
../src/cr_osa/src/osa_tsk.cpp 

OBJS += \
./src/cr_osa/src/osa.o \
./src/cr_osa/src/osa_buf.o \
./src/cr_osa/src/osa_eth_client.o \
./src/cr_osa/src/osa_eth_server.o \
./src/cr_osa/src/osa_event.o \
./src/cr_osa/src/osa_file.o \
./src/cr_osa/src/osa_i2c.o \
./src/cr_osa/src/osa_image_queue.o \
./src/cr_osa/src/osa_mbx.o \
./src/cr_osa/src/osa_msgq.o \
./src/cr_osa/src/osa_mutex.o \
./src/cr_osa/src/osa_pipe.o \
./src/cr_osa/src/osa_prf.o \
./src/cr_osa/src/osa_que.o \
./src/cr_osa/src/osa_rng.o \
./src/cr_osa/src/osa_sem.o \
./src/cr_osa/src/osa_thr.o \
./src/cr_osa/src/osa_tsk.o 

CPP_DEPS += \
./src/cr_osa/src/osa.d \
./src/cr_osa/src/osa_buf.d \
./src/cr_osa/src/osa_eth_client.d \
./src/cr_osa/src/osa_eth_server.d \
./src/cr_osa/src/osa_event.d \
./src/cr_osa/src/osa_file.d \
./src/cr_osa/src/osa_i2c.d \
./src/cr_osa/src/osa_image_queue.d \
./src/cr_osa/src/osa_mbx.d \
./src/cr_osa/src/osa_msgq.d \
./src/cr_osa/src/osa_mutex.d \
./src/cr_osa/src/osa_pipe.d \
./src/cr_osa/src/osa_prf.d \
./src/cr_osa/src/osa_que.d \
./src/cr_osa/src/osa_rng.d \
./src/cr_osa/src/osa_sem.d \
./src/cr_osa/src/osa_thr.d \
./src/cr_osa/src/osa_tsk.d 


# Each subdirectory must supply rules for building sources it contributes
src/cr_osa/src/%.o: ../src/cr_osa/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/encTrans -I../src -I../src/core -I../src/cr_osa/inc -I../src/nvMedia -O3 -Xcompiler -fopenmp -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_50,code=sm_50 -m64 -odir "src/cr_osa/src" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/encTrans -I../src -I../src/core -I../src/cr_osa/inc -I../src/nvMedia -O3 -Xcompiler -fopenmp --compile -m64 -ccbin aarch64-linux-gnu-g++  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



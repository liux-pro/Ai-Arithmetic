/* Copyright 2022 Sipeed Technology Co., Ltd. All Rights Reserved.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef __TM_PORT_H
#define __TM_PORT_H

#define TM_ARCH_CPU         (0) //default, pure cpu compute
#define TM_ARCH_ARM_SIMD    (1) //ARM Cortex M4/M7, etc.
#define TM_ARCH_ARM_NEON    (2) //ARM Cortex A7, etc.
#define TM_ARCH_ARM_MVEI    (3) //ARMv8.1: M55, etc.
#define TM_ARCH_RV32P       (4) //T-head E907, etc.
#define TM_ARCH_RV64V       (5) //T-head C906,C910, etc.

#define TM_OPT0             (0) //default, least code and buf
#define TM_OPT1             (1) //opt for speed, need more code and buf
#define TM_OPT2             (2) //TODO

/******************************* PORT CONFIG  ************************************/
#define TM_ARCH         TM_ARCH_CPU
#define TM_OPT_LEVEL    TM_OPT0
#define TM_MDL_TYPE     TM_MDL_INT8
#define TM_FASTSCALE    (0)         //enable if your chip don't have FPU, may speed up 1/3, but decrease accuracy
#define TM_LOCAL_MATH   (0)         //use local math func (like exp()) to avoid libm
#define TM_ENABLE_STAT  (0)         //enable mdl stat functions
#define TM_MAX_CSIZE    (36)      //max channel num //used if INT8 mdl  //cost TM_MAX_CSIZE*4 Byte
#define TM_MAX_KSIZE    (5*5)       //max kernel_size   //cost TM_MAX_KSIZE*4 Byte
#define TM_MAX_KCSIZE   (5*5*36)   //max kernel_size*channels //cost TM_MAX_KSIZE*sizeof(mtype_t) Byte

#define TM_INLINE       
#define TM_WEAK         

#define tm_malloc(x)    malloc(x)
#define tm_free(x)      free(x)

#define TM_PRINTF(...) printf(__VA_ARGS__)
#define TM_DBG(...)    //TM_PRINTF("###L%d: ",__LINE__);TM_PRINTF(__VA_ARGS__);
#define TM_DBGL()      TM_PRINTF("###L%d\r\n",__LINE__);

/******************************* DBG TIME CONFIG  ************************************/
extern uint32_t systick;
#define  TM_GET_MS()       (systick)  //((uint32_t)(clock()*1000000/CLOCKS_PER_SEC))

#define TM_DBGT_INIT()     uint32_t _start,_finish;uint32_t _time;_start=TM_GET_MS();
#define TM_DBGT_START()    _start=TM_GET_MS();
#define TM_DBGT(x)         {_finish=TM_GET_MS();\
                            _time = (_finish-_start);\
                            TM_PRINTF("===%s use %d ms\r\n", (x), _time);\
                            _start=TM_GET_MS();}

/******************************* DBG PERFORMANCE CONFIG  ************************************/
#define  TM_GET_TICK(x)
#define  TM_TICK_PERUS
#define  TM_PERF_REG(x)
#define  TM_PERF_EXTREG(x)
#define  TM_PERF_INIT(x)
#define  TM_PERF_START(x)
#define  TM_PERF_ADD(x)
#define  TM_PERF_PRINT(x)
/******************************* OPS CONFIG  ************************************/




#endif
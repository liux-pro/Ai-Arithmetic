#ifndef MY_STDINT_H
#define MY_STDINT_H

// 检查是否已经定义了这些类型
#ifndef UINT8_T_DEFINED
typedef unsigned char uint8_t;
#define UINT8_T_DEFINED
#endif

#ifndef UINT16_T_DEFINED
typedef unsigned int uint16_t;
#define UINT16_T_DEFINED
#endif

#ifndef UINT32_T_DEFINED
typedef unsigned long uint32_t;
#define UINT32_T_DEFINED
#endif

#ifndef INT8_T_DEFINED
typedef char int8_t;
#define INT8_T_DEFINED
#endif

#ifndef INT16_T_DEFINED
typedef int int16_t;
#define INT16_T_DEFINED
#endif

#ifndef INT32_T_DEFINED
typedef long int32_t;
#define INT32_T_DEFINED
#endif

#endif // MY_STDINT_H

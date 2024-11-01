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

//sum = SUM(Ai*Bi)
TM_INLINE void tm_dot_prod(mtype_t* sptr, mtype_t* kptr,uint32_t siz, sumtype_t* result)
{
    sumtype_t sum=0;
    uint32_t i = 0;
    uint32_t cnt = (siz>>3)<<3;  //8
    for(; i+8-1 <cnt; ){
        sum += sptr[i]*kptr[i];i++;
        sum += sptr[i]*kptr[i];i++;
        sum += sptr[i]*kptr[i];i++;
        sum += sptr[i]*kptr[i];i++;
        sum += sptr[i]*kptr[i];i++;
        sum += sptr[i]*kptr[i];i++;
        sum += sptr[i]*kptr[i];i++;
        sum += sptr[i]*kptr[i];i++;
    }
    for(; i <siz; i++){
        sum += sptr[i]*kptr[i]; 
    }
    *result = sum;
    return;
}

TM_INLINE  void tm_dot_prod_pack2(mtype_t* sptr, mtype_t* kptr, uint32_t siz, sumtype_t* result)
{ 
    sumtype_t sum0 = 0;
    sumtype_t sum1 = 0;
    mtype_t* kptr0 = kptr;
    mtype_t* kptr1 = kptr+siz;

    uint32_t i = 0;
    uint32_t cnt = (siz>>3)<<3;  //8
    for(; i+8-1 <cnt; ){
        sum0 += sptr[i]*kptr0[i]; sum1 += sptr[i]*kptr1[i]; i++;
        sum0 += sptr[i]*kptr0[i]; sum1 += sptr[i]*kptr1[i]; i++;
        sum0 += sptr[i]*kptr0[i]; sum1 += sptr[i]*kptr1[i]; i++;
        sum0 += sptr[i]*kptr0[i]; sum1 += sptr[i]*kptr1[i]; i++;
        sum0 += sptr[i]*kptr0[i]; sum1 += sptr[i]*kptr1[i]; i++;
        sum0 += sptr[i]*kptr0[i]; sum1 += sptr[i]*kptr1[i]; i++;
        sum0 += sptr[i]*kptr0[i]; sum1 += sptr[i]*kptr1[i]; i++;
        sum0 += sptr[i]*kptr0[i]; sum1 += sptr[i]*kptr1[i]; i++;
    }
    for(; i <siz; i++){
        sum0 += sptr[i]*kptr0[i]; 
        sum1 += sptr[i]*kptr1[i]; 
    }

    result[0] = sum0;
    result[1] = sum1;
    return;
}

TM_INLINE void tm_dot_prod_gap_3x3x1(mtype_t* sptr, mtype_t* kptr, uint32_t* k_oft, sumtype_t* result)
{
    *result = sptr[k_oft[0]]*kptr[0] + sptr[k_oft[1]]*kptr[1] + sptr[k_oft[2]]*kptr[2] + \
        sptr[k_oft[3]]*kptr[3] + sptr[k_oft[4]]*kptr[4] + sptr[k_oft[5]]*kptr[5] + \
        sptr[k_oft[6]]*kptr[6] + sptr[k_oft[7]]*kptr[7] + sptr[k_oft[8]]*kptr[8] ;
    return;                  
}

TM_INLINE void tm_dot_prod_3x3x1(mtype_t* sptr, mtype_t* kptr, sumtype_t* result)
{
    *result = sptr[0]*kptr[0] + sptr[1]*kptr[1] + sptr[2]*kptr[2] + \
        sptr[3]*kptr[3] + sptr[4]*kptr[4] + sptr[5]*kptr[5] + \
        sptr[6]*kptr[6] + sptr[7]*kptr[7] + sptr[8]*kptr[8] ;
    return;
}





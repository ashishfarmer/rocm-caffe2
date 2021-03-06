/**
 * Copyright (c) 2016-present, Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "caffe2/core/context_hip.h"
#include "caffe2/operators/accuracy_op.h"
#include "caffe2/utils/math.h"
#include "hip/hip_runtime.h"
#include <hipcub/hipcub.hpp>

namespace caffe2 {

namespace {
__global__ void AccuracyKernel(const int N,
                               const int D,
                               const int top_k,
                               const float* Xdata,
                               const int* labelData,
                               float* accuracy)
{
    using BlockReduce = hipcub::BlockReduce<int, CAFFE_HIP_NUM_THREADS>;
    __shared__ typename BlockReduce::TempStorage temp_storage;
    int correct = 0;
    for(int row = blockIdx.x; row < N; row += gridDim.x)
    {
        const int label        = labelData[row];
        const float label_pred = Xdata[row * D + label];
        int ngt                = 0;
        for(int col = threadIdx.x; col < D; col += blockDim.x)
        {
            const float pred = Xdata[row * D + col];
            if(pred > label_pred || (pred == label_pred && col <= label))
            {
                ++ngt;
            }
        }
        ngt = BlockReduce(temp_storage).Sum(ngt);
        if(ngt <= top_k)
        {
            ++correct;
        }
        __syncthreads();
    }
    if(threadIdx.x == 0)
    {
        atomicAdd(accuracy, static_cast<float>(correct));
    }
}

__global__ void AccuracyDivideKernel(const int N, float* accuracy) { *accuracy /= N; }
} // namespace

template <>
bool AccuracyOp<float, HIPContext>::RunOnDevice()
{
    auto& X     = Input(PREDICTION);
    auto& label = Input(LABEL);
    auto* Y     = Output(0);
    CAFFE_ENFORCE_EQ(X.ndim(), 2);
    int N = X.dim32(0);
    int D = X.dim32(1);
    CAFFE_ENFORCE_EQ(label.ndim(), 1);
    CAFFE_ENFORCE_EQ(label.dim32(0), N);
    Y->Resize(vector<TIndex>());
    float* Ydata = Y->mutable_data<float>();
    math::Set<float, HIPContext>(1, 0, Ydata, &context_);
    hipLaunchKernelGGL((AccuracyKernel),
                       dim3(std::min(CAFFE_MAXIMUM_NUM_BLOCKS, N)),
                       dim3(CAFFE_HIP_NUM_THREADS),
                       0,
                       context_.hip_stream(),
                       N,
                       D,
                       top_k_,
                       X.data<float>(),
                       label.data<int>(),
                       Ydata);
    // This is going to be executed only in one single kernel. Not very beautiful,
    // but probably we have to do this?
    hipLaunchKernelGGL(
        (AccuracyDivideKernel), dim3(1), dim3(1), 0, context_.hip_stream(), N, Ydata);
    return true;
}

REGISTER_HIP_OPERATOR(Accuracy, AccuracyOp<float, HIPContext>);
} // namespace caffe2

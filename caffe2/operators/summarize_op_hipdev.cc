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

#include <thrust/device_vector.h>
#include <thrust/transform_reduce.h>
#include <thrust/system/cuda/execution_policy.h>

#include "caffe2/operators/summarize_op.h"
#include "caffe2/core/context_hip.h"

namespace caffe2 {

namespace {

// structure used to accumulate the moments and other statistical properties
// encountered so far.
template <typename T>
struct SummaryStatsData
{
    T n;
    T min;
    T max;
    T mean;
    T M2;

    // initialize to the identity element
    void initialize()
    {
        n = mean = M2 = 0;
        min           = std::numeric_limits<T>::max();
        max           = std::numeric_limits<T>::min();
    }

    T variance() { return (n == 1 ? 0 : M2 / (n - 1)); }
};

// stats_unary_op is a functor that takes in a value x and
// returns a variace_data whose mean value is initialized to x.
template <typename T>
struct summary_stats_unary_op
{
    __host__ __device__ SummaryStatsData<T> operator()(const T& x) const
    {
        SummaryStatsData<T> result;
        result.n    = 1;
        result.min  = x;
        result.max  = x;
        result.mean = x;
        result.M2   = 0;
        return result;
    }
};

// summary_stats_binary_op is a functor that accepts two SummaryStatsData
// structs and returns a new SummaryStatsData which are an
// approximation to the summary_stats for
// all values that have been agregated so far
template <typename T>
struct summary_stats_binary_op : public thrust::binary_function<const SummaryStatsData<T>&,
                                                                const SummaryStatsData<T>&,
                                                                SummaryStatsData<T>>
{
    __host__ __device__ SummaryStatsData<T> operator()(const SummaryStatsData<T>& x,
                                                       const SummaryStatsData<T>& y) const
    {
        SummaryStatsData<T> result;
        T n         = x.n + y.n;
        T delta     = y.mean - x.mean;
        T delta2    = delta * delta;
        result.n    = n;
        result.min  = thrust::min(x.min, y.min);
        result.max  = thrust::max(x.max, y.max);
        result.mean = x.mean + delta * y.n / n;
        result.M2   = x.M2 + y.M2;
        result.M2 += delta2 * x.n * y.n / n;
        return result;
    }
};

} // namespace

template <>
bool SummarizeOp<float, HIPContext>::RunOnDevice()
{
    auto& X     = Input(0);
    const int N = X.size();
    DCHECK_GT(N, 0);

    // TODO(Yangqing): Any better way to avoid having to const cast?
    thrust::device_ptr<float> Xdata(const_cast<float*>(X.data<float>()));
    summary_stats_unary_op<float> unary_op;
    summary_stats_binary_op<float> binary_op;
    SummaryStatsData<float> init;
    init.initialize();
    // compute summary statistics
    SummaryStatsData<float> result = thrust::transform_reduce(
#if THRUST_VERSION >= 100800
        thrust::cuda::par.on(context_.hip_stream()),
#endif // THRUST_VERSION >= 100800
        Xdata,
        Xdata + N,
        unary_op,
        init,
        binary_op);
    float standard_deviation = std::sqrt(result.variance());
    if(to_file_)
    {
        (*log_file_) << result.min << " " << result.max << " " << result.mean << " "
                     << standard_deviation << std::endl;
    }
    if(OutputSize())
    {
        auto* Y = OperatorBase::Output<TensorHIP>(0);
        Y->Resize(4);
        float output_buffer[NUM_STATS] = {result.min, result.max, result.mean, standard_deviation};
        context_.Copy<float, CPUContext, HIPContext>(
            NUM_STATS, output_buffer, Y->mutable_data<float>());
    }
    return true;
}

REGISTER_HIP_OPERATOR(Summarize, SummarizeOp<float, HIPContext>);
} // namespace caffe2

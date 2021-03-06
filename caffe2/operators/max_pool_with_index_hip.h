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

#ifndef CAFFE2_OPERATORS_MAX_POOL_WITH_INDEX_HIP_H_
#define CAFFE2_OPERATORS_MAX_POOL_WITH_INDEX_HIP_H_

#include <cfloat>
#include "caffe2/core/context.h"
#include "caffe2/core/context_hip.h"
#include "caffe2/core/logging.h"
#include "caffe2/core/operator.h"
#include "caffe2/operators/conv_pool_op_base.h"
#include "caffe2/operators/pool_op.h"
#include "caffe2/utils/math.h"

namespace caffe2 {

class MaxPoolWithIndexOp final : public ConvPoolOpBase<HIPContext>
{
    public:
    USE_CONV_POOL_BASE_FUNCTIONS(HIPContext);
    MaxPoolWithIndexOp(const OperatorDef& operator_def, Workspace* ws)
        : ConvPoolOpBase<HIPContext>(operator_def, ws)
    {
    }
    ~MaxPoolWithIndexOp() {}

    template <typename T>
    bool DoRunWithType();

    bool RunOnDevice() override;

    // Input: X
    // Output: Y, mask
};

class MaxPoolWithIndexGradientOp final : public ConvPoolOpBase<HIPContext>
{
    public:
    USE_CONV_POOL_BASE_FUNCTIONS(HIPContext);
    MaxPoolWithIndexGradientOp(const OperatorDef& operator_def, Workspace* ws)
        : ConvPoolOpBase<HIPContext>(operator_def, ws)
    {
    }
    ~MaxPoolWithIndexGradientOp() {}

    template <typename T>
    bool DoRunWithType();

    bool RunOnDevice() override;

    // Input: X, dY, mask
    // Output: dX
};

}; // namespace caffe2

#endif // CAFFE2_OPERATORS_MAX_POOL_WITH_INDEX_HIP_H_

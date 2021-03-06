/* Copyright (c) 2016 PaddlePaddle Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#pragma once
#include "paddle/fluid/framework/eigen.h"
#include "paddle/fluid/framework/op_registry.h"
#include "paddle/fluid/operators/gather.h"
#include "paddle/fluid/operators/scatter.h"

namespace paddle {
namespace operators {

using Tensor = framework::Tensor;

template <typename T>
class ScatterOpKernel : public framework::OpKernel<T> {
 public:
  void Compute(const framework::ExecutionContext &ctx) const override {
    PADDLE_ENFORCE(platform::is_cpu_place(ctx.GetPlace()),
                   "This kernel only runs on CPU.");
    auto *X = ctx.Input<Tensor>("X");
    auto *Ids = ctx.Input<Tensor>("Ids");
    auto *Updates = ctx.Input<Tensor>("Updates");
    auto *Out = ctx.Output<Tensor>("Out");

    // In place output: Out = X, Out[Ids] += Updates
    framework::TensorCopySync(*X, ctx.GetPlace(), Out);
    // Apply ScatterUpdate: Out[index] += Updates[:]
    ScatterAssign<T>(ctx.device_context(), *Updates, *Ids, Out);
  }
};

template <typename T>
class ScatterGradientOpKernel : public framework::OpKernel<T> {
 public:
  void Compute(const framework::ExecutionContext &ctx) const override {
    PADDLE_ENFORCE(platform::is_cpu_place(ctx.GetPlace()),
                   "This kernel only runs on CPU.");
    auto *dX = ctx.Output<Tensor>(framework::GradVarName("X"));
    auto *dUpdates = ctx.Output<Tensor>(framework::GradVarName("Updates"));
    auto *Ids = ctx.Input<Tensor>("Ids");
    auto *dOut = ctx.Input<Tensor>(framework::GradVarName("Out"));

    // In place gradient: dX = dO
    framework::TensorCopySync(*dOut, ctx.GetPlace(), dX);
    dUpdates->mutable_data<T>(ctx.GetPlace());
    // Gradient by Gather: dUpdates += dO[Ids]
    CPUGather<T>(ctx.device_context(), *dOut, *Ids, dUpdates);
  }
};

}  // namespace operators
}  // namespace paddle

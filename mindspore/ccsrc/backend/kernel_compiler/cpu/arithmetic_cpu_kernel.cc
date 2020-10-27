/**
 * Copyright 2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "backend/kernel_compiler/cpu/arithmetic_cpu_kernel.h"
#include <thread>
#include <string>
#include "runtime/device/cpu/cpu_device_address.h"

namespace mindspore {
namespace kernel {
template <typename T>
void ArithmeticCPUKernel::AssignAdd(T *input1, const T *input2, T *out, size_t start, size_t end) {
  for (size_t i = start; i < end; i++) {
    out[i] = input1[i] + input2[i];
    input1[i] = out[i];
  }
}

template <typename T>
void ArithmeticCPUKernel::Add(const T *input1, const T *input2, T *out, size_t start, size_t end) {
  for (size_t i = start; i < end; i++) {
    out[i] = input1[i] + input2[i];
  }
}

template <typename T>
void ArithmeticCPUKernel::Sub(const T *input1, const T *input2, T *out, size_t start, size_t end) {
  for (size_t i = start; i < end; i++) {
    std::vector<size_t> idx;
    GenIndex(i, &idx);
    out[i] = input1[idx[0]] - input2[idx[1]];
  }
}

template <typename T>
void ArithmeticCPUKernel::Mul(const T *input1, const T *input2, T *out, size_t start, size_t end) {
  for (size_t i = start; i < end; i++) {
    out[i] = input1[i] * input2[i];
  }
}

template <typename T>
void ArithmeticCPUKernel::Div(const T *input1, const T *input2, T *out, size_t start, size_t end) {
  for (size_t i = start; i < end; i++) {
    auto div_number = input2[i];
    if (div_number == 0) {
      MS_LOG(EXCEPTION) << "Cannot divided by 0!";
    }
    out[i] = input1[i] / div_number;
  }
}

void ArithmeticCPUKernel::InitKernel(const CNodePtr &kernel_node) {
  MS_EXCEPTION_IF_NULL(kernel_node);
  std::string kernel_name = AnfAlgo::GetCNodeName(kernel_node);
  if (kernel_name == prim::kPrimTensorAdd->name()) {
    operate_type_ = ADD;
  } else if (kernel_name == prim::kPrimSub->name()) {
    operate_type_ = SUB;
  } else if (kernel_name == prim::kPrimMul->name()) {
    operate_type_ = MUL;
  } else if (kernel_name == "Div") {
    operate_type_ = DIV;
  } else if (kernel_name == prim::kPrimAssignAdd->name()) {
    operate_type_ = ASSIGNADD;
  }

  input_shape0_ = AnfAlgo::GetPrevNodeOutputInferShape(kernel_node, 0);
  input_shape1_ = AnfAlgo::GetPrevNodeOutputInferShape(kernel_node, 1);
  output_shape_ = AnfAlgo::GetOutputInferShape(kernel_node, 0);
  if (output_shape_.size() == 0) {
    output_shape_.insert(output_shape_.begin(), 1);
  }
  size_t l = input_shape0_.size();
  for (size_t i = 0; i < output_shape_.size() - l; ++i) {
    input_shape0_.insert(input_shape0_.begin(), 1);
  }
  l = input_shape1_.size();
  for (size_t i = 0; i < output_shape_.size() - l; ++i) {
    input_shape1_.insert(input_shape1_.begin(), 1);
  }
  CPUKernelUtils::GetElementNumEveryDim(input_shape0_, &input_element_num0_);
  CPUKernelUtils::GetElementNumEveryDim(input_shape1_, &input_element_num1_);
  CPUKernelUtils::GetElementNumEveryDim(output_shape_, &output_element_num_);
  dtype_ = AnfAlgo::GetPrevNodeOutputInferDataType(kernel_node, 0);
  if (dtype_ != AnfAlgo::GetPrevNodeOutputInferDataType(kernel_node, 1)) {
    MS_LOG(EXCEPTION) << "Input0 and input1 must has the same data type";
  }
}

bool ArithmeticCPUKernel::Launch(const std::vector<kernel::AddressPtr> &inputs,
                                 const std::vector<kernel::AddressPtr> & /*workspace*/,
                                 const std::vector<kernel::AddressPtr> &outputs) {
  if (dtype_ == kNumberTypeInt32) {
    LaunchKernel<int>(inputs, outputs);
  } else if (dtype_ == kNumberTypeFloat32) {
    LaunchKernel<float>(inputs, outputs);
  } else if (dtype_ == kNumberTypeInt64) {
    LaunchKernel<int64_t>(inputs, outputs);
  } else {
    MS_LOG(EXCEPTION) << "Only support int32, float32, but actual data type is " << TypeIdLabel(dtype_);
  }
  return true;
}

void ArithmeticCPUKernel::GenIndex(size_t num, std::vector<size_t> *idx) {
  std::vector<size_t> tmp;
  for (size_t i = 0; i < output_shape_.size() - 1; ++i) {
    if (output_element_num_[i] > num) {
      tmp.push_back(0);
    } else {
      tmp.push_back(num / output_element_num_[i]);
      num %= output_element_num_[i];
    }
  }
  tmp.push_back(num);
  size_t idx0 = 0;
  size_t idx1 = 0;
  for (size_t k = 0; k < tmp.size() - 1; ++k) {
    if (input_shape0_[k] > 1) {
      idx0 += tmp[k] * input_element_num0_[k];
    }
    if (input_shape1_[k] > 1) {
      idx1 += tmp[k] * input_element_num1_[k];
    }
  }
  if (input_shape0_[tmp.size() - 1] > 1) {
    idx0 += tmp[tmp.size() - 1];
  }
  if (input_shape1_[tmp.size() - 1] > 1) {
    idx1 += tmp[tmp.size() - 1];
  }
  idx->push_back(idx0);
  idx->push_back(idx1);
}
template <typename T>
void ArithmeticCPUKernel::LaunchKernel(const std::vector<AddressPtr> &inputs, const std::vector<AddressPtr> &outputs) {
  T *input1 = reinterpret_cast<T *>(inputs[0]->addr);
  T *input2 = reinterpret_cast<T *>(inputs[1]->addr);
  T *output = reinterpret_cast<T *>(outputs[0]->addr);
  auto lens = outputs[0]->size / sizeof(T);
  size_t thread_num = lens < 128 * 24 ? std::ceil(lens / 128.0) : 24;
  MS_LOG(INFO) << "lens=" << lens << "; use thread_num=" << thread_num;
  std::vector<std::thread> threads;
  threads.reserve(thread_num);
  size_t start = 0;
  size_t once_compute_size = (lens + thread_num - 1) / thread_num;
  while (start < lens) {
    size_t end = (start + once_compute_size) > lens ? lens : (start + once_compute_size);
    if (operate_type_ == ADD) {
      threads.emplace_back(std::thread(&ArithmeticCPUKernel::Add<T>, this, input1, input2, output, start, end));
    } else if (operate_type_ == SUB) {
      threads.emplace_back(std::thread(&ArithmeticCPUKernel::Sub<T>, this, input1, input2, output, start, end));
    } else if (operate_type_ == MUL) {
      threads.emplace_back(std::thread(&ArithmeticCPUKernel::Mul<T>, this, input1, input2, output, start, end));
    } else if (operate_type_ == DIV) {
      threads.emplace_back(std::thread(&ArithmeticCPUKernel::Div<T>, this, input1, input2, output, start, end));
    } else if (operate_type_ == ASSIGNADD) {
      threads.emplace_back(std::thread(&ArithmeticCPUKernel::AssignAdd<T>, this, input1, input2, output, start, end));
    }
    start += once_compute_size;
  }
  for (size_t i = 0; i < threads.size(); ++i) {
    threads[i].join();
  }
}
}  // namespace kernel
}  // namespace mindspore

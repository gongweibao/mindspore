/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#include "src/ops/bias_grad.h"

#ifndef PRIMITIVE_WRITEABLE
#include "src/ops/ops_register.h"
#endif

namespace mindspore {
namespace lite {
#ifdef PRIMITIVE_WRITEABLE
int BiasGrad::UnPackAttr(const Primitive &prim, const std::vector<AnfNodePtr> &inputs) {
  if (this->primitive_ == nullptr) {
    this->primitive_ = new (std::nothrow) schema::PrimitiveT;
    if (this->primitive_ == nullptr) {
      MS_LOG(ERROR) << "new primitiveT failed";
      return RET_ERROR;
    }
    this->primitive_->value.type = schema::PrimitiveType_BiasGrad;
  }
  if (this->primitive_->value.type != schema::PrimitiveType_BiasGrad) {
    MS_LOG(ERROR) << "Primitive type is error :" << this->primitive_->value.type;
    return RET_ERROR;
  }
  if (this->primitive_->value.value == nullptr) {
    auto attr = new (std::nothrow) schema::BiasGradT();
    if (attr == nullptr) {
      MS_LOG(ERROR) << "new primitiveT value failed";
      return RET_ERROR;
    }

    this->primitive_->value.value = attr;
    if (this->primitive_->value.value == nullptr) {
      MS_LOG(ERROR) << "primitive value is nullptr";
      return RET_ERROR;
    }
    this->primitive_->value.value = attr;
  }
  return RET_OK;
}
#else
int BiasGrad::UnPackToFlatBuilder(const schema::Primitive *primitive, flatbuffers::FlatBufferBuilder *fbb) {
  MS_ASSERT(nullptr != primitive);
  MS_ASSERT(nullptr != fbb);
  auto attr = primitive->value_as_BiasGrad();
  if (attr == nullptr) {
    MS_LOG(ERROR) << "value_as_BiasGrad return nullptr";
    return RET_ERROR;
  }

  auto val_offset = schema::CreateBiasGrad(*fbb);
  auto prim_offset = schema::CreatePrimitive(*fbb, schema::PrimitiveType_BiasGrad, val_offset.o);
  fbb->Finish(prim_offset);
  return RET_OK;
}

PrimitiveC *BiasGradCreator(const schema::Primitive *primitive) {
  return PrimitiveC::NewPrimitiveC<BiasGrad>(primitive);
}
Registry BiasGradRegistry(schema::PrimitiveType_BiasGrad, BiasGradCreator);
#endif

int BiasGrad::InferShape(std::vector<Tensor *> inputs, std::vector<Tensor *> outputs) {
  if (inputs.size() != 1) {
    MS_LOG(ERROR) << "BiasGrad should have one input";
    return RET_ERROR;
  }
  if (outputs.size() != 1) {
    MS_LOG(ERROR) << "BiasGrad should have one output";
    return RET_ERROR;
  }
  auto *in0 = inputs.front();
  auto *out = outputs.front();
  MS_ASSERT(in0 != nullptr);
  MS_ASSERT(out != nullptr);

  auto inshape = in0->shape();
  int ndim = inshape.size();
  for (int i = 0; i < ndim - 1; i++) {
    inshape[i] = 1;
  }
  out->set_shape(inshape);
  out->set_data_type(in0->data_type());
  out->set_format(in0->format());

  return RET_OK;
}
}  // namespace lite
}  // namespace mindspore

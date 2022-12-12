// Copyright (c) 2022 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "fastdeploy/vision/detection/contrib/rknpu2/preprocessor.h"
#include "fastdeploy/function/concat.h"

namespace fastdeploy {
namespace vision {
namespace detection {

RKYOLOPreprocessor::RKYOLOPreprocessor() {
  size_ = {640, 640};
  padding_value_ = {114.0, 114.0, 114.0};
  is_mini_pad_ = false;
  is_no_pad_ = false;
  is_scale_up_ = true;
  stride_ = 32;
  max_wh_ = 7680.0;
}

void RKYOLOPreprocessor::LetterBox(FDMat* mat) {
  std::cout << "mat->Height() = " << mat->Height() << std::endl;
  std::cout << "mat->Width() = " << mat->Width() << std::endl;

  float scale =
      std::min(size_[1] * 1.0 / mat->Height(), size_[0] * 1.0 / mat->Width());
  std::cout << "RKYOLOPreprocessor scale_ = " << scale << std::endl;
  if (!is_scale_up_) {
    scale = std::min(scale, 1.0f);
  }
  std::cout << "RKYOLOPreprocessor scale_ = " << scale << std::endl;
  scale_.push_back(scale);

  int resize_h = int(round(mat->Height() * scale));
  int resize_w = int(round(mat->Width() * scale));

  int pad_w = size_[0] - resize_w;
  int pad_h = size_[1] - resize_h;
  if (is_mini_pad_) {
    pad_h = pad_h % stride_;
    pad_w = pad_w % stride_;
  } else if (is_no_pad_) {
    pad_h = 0;
    pad_w = 0;
    resize_h = size_[1];
    resize_w = size_[0];
  }

  pad_hw_values_.push_back({pad_h, pad_w});

  if (std::fabs(scale - 1.0f) > 1e-06) {
    Resize::Run(mat, resize_w, resize_h);
  }
  if (pad_h > 0 || pad_w > 0) {
    float half_h = pad_h * 1.0 / 2;
    int top = int(round(half_h - 0.1));
    int bottom = int(round(half_h + 0.1));
    float half_w = pad_w * 1.0 / 2;
    int left = int(round(half_w - 0.1));
    int right = int(round(half_w + 0.1));
    Pad::Run(mat, top, bottom, left, right, padding_value_);
  }
}

bool RKYOLOPreprocessor::Preprocess(FDMat* mat, FDTensor* output) {
  // process after image load
  //  float ratio = std::min(size_[1] * 1.0f / static_cast<float>(mat->Height()),
  //                         size_[0] * 1.0f / static_cast<float>(mat->Width()));
  //  if (std::fabs(ratio - 1.0f) > 1e-06) {
  //    int interp = cv::INTER_AREA;
  //    if (ratio > 1.0) {
  //      interp = cv::INTER_LINEAR;
  //    }
  //    int resize_h = int(mat->Height() * ratio);
  //    int resize_w = int(mat->Width() * ratio);
  //    Resize::Run(mat, resize_w, resize_h, -1, -1, interp);
  //  }

  // RKYOLO's preprocess steps
  // 1. letterbox
  // 2. convert_and_permute(swap_rb=true)
  LetterBox(mat);
  BGR2RGB::Run(mat);
  mat->ShareWithTensor(output);
  output->ExpandDim(0);  // reshape to n, h, w, c
  return true;
}

bool RKYOLOPreprocessor::Run(std::vector<FDMat>* images,
                             std::vector<FDTensor>* outputs) {
  if (images->size() == 0) {
    FDERROR << "The size of input images should be greater than 0."
            << std::endl;
    return false;
  }
  outputs->resize(1);
  // Concat all the preprocessed data to a batch tensor
  std::vector<FDTensor> tensors(images->size());
  for (size_t i = 0; i < images->size(); ++i) {
    if (!Preprocess(&(*images)[i], &tensors[i])) {
      FDERROR << "Failed to preprocess input image." << std::endl;
      return false;
    }
  }

  if (tensors.size() == 1) {
    (*outputs)[0] = std::move(tensors[0]);
  } else {
    function::Concat(tensors, &((*outputs)[0]), 0);
  }
  return true;
}

}  // namespace detection
}  // namespace vision
}  // namespace fastdeploy

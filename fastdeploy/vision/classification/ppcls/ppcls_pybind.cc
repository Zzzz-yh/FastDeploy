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
#include "fastdeploy/pybind/main.h"

namespace fastdeploy {
void BindPaddleClas(pybind11::module& m) {
  pybind11::class_<vision::classification::PaddleClasPreprocessor>(
      m, "PaddleClasPreprocessor")
      .def(pybind11::init<std::string>())
      .def("run", [](vision::classification::PaddleClasPreprocessor& self, std::vector<pybind11::array>& im_list) {
        std::vector<vision::FDMat> images;
        for (size_t i = 0; i < im_list.size(); ++i) {
          images.push_back(vision::WrapMat(PyArrayToCvMat(im_list[i])));
        }
        std::vector<FDTensor> outputs;
        if (!self.Run(&images, &outputs)) {
          throw std::runtime_error("Failed to preprocess the input data in PaddleClasPreprocessor.");
        }
        if (!self.WithGpu()) {
          for (size_t i = 0; i < outputs.size(); ++i) {
            outputs[i].StopSharing();
          }
        }
        return outputs;
      })
      .def("use_gpu", [](vision::classification::PaddleClasPreprocessor& self, int gpu_id = -1) {
        self.UseGpu(gpu_id);
      });

  pybind11::class_<vision::classification::PaddleClasPostprocessor>(
      m, "PaddleClasPostprocessor")
      .def(pybind11::init<int>())
      .def("run", [](vision::classification::PaddleClasPostprocessor& self, std::vector<FDTensor>& inputs) {
        std::vector<vision::ClassifyResult> results;
        if (!self.Run(inputs, &results)) {
          throw std::runtime_error("Failed to postprocess the runtime result in PaddleClasPostprocessor.");
        }
        return results;
      })
      .def("run", [](vision::classification::PaddleClasPostprocessor& self, std::vector<pybind11::array>& input_array) {
        std::vector<vision::ClassifyResult> results;
        std::vector<FDTensor> inputs;
        PyArrayToTensorList(input_array, &inputs, /*share_buffer=*/true);
        if (!self.Run(inputs, &results)) {
          throw std::runtime_error("Failed to postprocess the runtime result in PaddleClasPostprocessor.");
        }
        return results;
      })
      .def_property("topk", &vision::classification::PaddleClasPostprocessor::GetTopk, &vision::classification::PaddleClasPostprocessor::SetTopk);

  pybind11::class_<vision::classification::PaddleClasModel, FastDeployModel>(
      m, "PaddleClasModel")
      .def(pybind11::init<std::string, std::string, std::string, RuntimeOption,
                          ModelFormat>())
      .def("predict", [](vision::classification::PaddleClasModel& self, pybind11::array& data) {
        cv::Mat im = PyArrayToCvMat(data);
        vision::ClassifyResult result;
        self.Predict(im, &result);
        return result;
      })
      .def("batch_predict", [](vision::classification::PaddleClasModel& self, std::vector<pybind11::array>& data) {
        std::vector<cv::Mat> images;
        for (size_t i = 0; i < data.size(); ++i) {
          images.push_back(PyArrayToCvMat(data[i]));
        }
        std::vector<vision::ClassifyResult> results;
        self.BatchPredict(images, &results);
        return results;
      })
      .def_property_readonly("preprocessor", &vision::classification::PaddleClasModel::GetPreprocessor)
      .def_property_readonly("postprocessor", &vision::classification::PaddleClasModel::GetPostprocessor);
}
}  // namespace fastdeploy

#include <iostream>
#include <string>
#include "fastdeploy/vision.h"

void ONNXInfer(const std::string& model_dir, const std::string& image_file) {
  std::string model_file = model_dir + "/scrfd_500m_bnkps_shape640x640.onnx";
  std::string params_file;
  auto option = fastdeploy::RuntimeOption();
  option.UseCpu();
  auto format = fastdeploy::ModelFormat::ONNX;

  auto model = fastdeploy::vision::facedet::SCRFD(
      model_file, params_file, option, format);

  if (!model.Initialized()) {
    std::cerr << "Failed to initialize." << std::endl;
    return;
  }

  fastdeploy::TimeCounter tc;
  tc.Start();
  auto im = cv::imread(image_file);
  fastdeploy::vision::FaceDetectionResult res;
  if (!model.Predict(&im, &res)) {
    std::cerr << "Failed to predict." << std::endl;
    return;
  }
  auto vis_im = fastdeploy::vision::Visualize::VisFaceDetection(im, res);
  tc.End();
  tc.PrintInfo("SCRFD in ONNX");

  cv::imwrite("infer_onnx.jpg", vis_im);
  std::cout
      << "Visualized result saved in ./infer_onnx.jpg"
      << std::endl;
}

void RKNPU2Infer(const std::string& model_dir, const std::string& image_file) {
  std::string model_file = model_dir + "/scrfd_500m_bnkps_shape640x640_rk3588.rknn";
  std::string params_file;
  auto option = fastdeploy::RuntimeOption();
  option.UseRKNPU2();
  auto format = fastdeploy::ModelFormat::RKNN;

  auto model = fastdeploy::vision::facedet::SCRFD(model_file, params_file, option, format);

  if (!model.Initialized()) {
    std::cerr << "Failed to initialize." << std::endl;
    return;
  }
  model.DisableNormalizeAndPermute();

  fastdeploy::TimeCounter tc;
  tc.Start();
  auto im = cv::imread(image_file);
  fastdeploy::vision::FaceDetectionResult res;
  if (!model.Predict(&im, &res)) {
    std::cerr << "Failed to predict." << std::endl;
    return;
  }
  auto vis_im = fastdeploy::vision::Visualize::VisFaceDetection(im, res);
  tc.End();
  tc.PrintInfo("SCRFD in RKNN");

  cv::imwrite("infer_rknn.jpg", vis_im);
  std::cout
      << "Visualized result saved in ./infer_rknn.jpg"
      << std::endl;
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::cout
        << "Usage: infer_demo path/to/model_dir path/to/image run_option, "
           "e.g ./infer_model ./picodet_model_dir ./test.jpeg"
        << std::endl;
    return -1;
  }

  RKNPU2Infer(argv[1], argv[2]);
  ONNXInfer(argv[1], argv[2]);
  return 0;
}
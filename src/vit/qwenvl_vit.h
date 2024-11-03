// QwenVL VIT(Vision transformer).

#pragma once

#ifdef PILLOW_RESIZE_ENABLE
#include <PillowResize/PillowResize.hpp>
#endif
#include <opencv2/opencv.hpp>

#include "src/vit/vit.h"

namespace netease::grps {

class QwenvlVIT : public VIT {
public:
  QwenvlVIT() : VIT("qwenvl") {}
  ~QwenvlVIT() override = default;

  /**
   * @brief Preprocess images, and will be used as the input to the ViT model.
   * @param images: images bytes will be preprocessed.
   * @param prompt: prompt may be changed when vit encoding.
   * @return processed image data with trt tensor format, will be used as the input to the ViT model.
   */
  VitModelInputType Preprocess(const std::vector<std::vector<char>>& images_bytes, std::string& prompt) override;

  /**
   * @brief Postprocess output of vit trt model, and will be used as trtllm ptuning embedding table.
   * @param model_out: output of vit trt model will be postprocessed.
   * @param prompt: prompt may be changed when vit encoding.
   * @return vit embeddings will be used as trtllm ptuning embedding table.
   */
  PtuningEmbeddingTableType Postprocess(VitModelOutputType& model_out, std::string& prompt) override;

private:
  void Normalize(cv::Mat& img);
  void LoadImage(const std::vector<std::vector<char>>& images_bytes,
                 std::vector<std::vector<cv::Mat>>& out,
                 size_t idx,
                 int input_size = 448);

  // IMAGENET mean and std for normalization
  cv::Scalar imagenet_mean_ = {0.48145466, 0.4578275, 0.40821073};
  cv::Scalar imagenet_std_ = {0.26862954, 0.26130258, 0.27577711};
};

} // namespace netease::grps
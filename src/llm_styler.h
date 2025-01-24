// Prompt builder and llm generated txt parser for different style of llm model family.

#pragma once

#include <rapidjson/document.h>

#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace netease::grps {

class LLMStyler {
public:
  /**
   * @brief Construct a new LLMStyler object
   * @param style_name: Style name.
   * @param system_prompt: System role prompt.
   * @param roles: Roles name. [`system_name`, `user_name`, `assistant_name`]
   * @param support_func_call: If support function call.
   * @param func_call_observation_words: Function call observation words, used to early stop.
   * @param add_generation_prompt: If true, will add generation prompt in the end of prompt.
   */
  LLMStyler(std::string style_name,
            std::string system_prompt,
            const std::vector<std::string>& roles,
            bool support_func_call,
            std::string func_call_observation_words,
            bool add_generation_prompt = false)
      : style_name_(std::move(style_name))
      , system_prompt_(std::move(system_prompt))
      , roles_(roles)
      , support_func_call_(support_func_call)
      , func_call_observation_words_(std::move(func_call_observation_words))
      , add_generation_prompt_(add_generation_prompt) {}
  virtual ~LLMStyler() = default;

  /**
   * @brief Build prompt for model input from OpenAI interface json body request.
   * @param json_body: Json body from client.
   * @return <if_function_call, prompt, img_urls>: if_function_call is true if the prompt contains function call.
   */
  virtual std::tuple<bool, std::string, std::vector<std::string>> BuildPrompt(const rapidjson::Document& json_body);

  /**
   * @brief Parse function call response from generated text and build content and tool_calls array of message
   * member of OpenAI interface response.
   * @param gen_txt: Generated text.
   * @param req_id: Request id.
   * @param message: Message member of OpenAI interface response format.
   * @param allocator: Json allocator.
   * @return stop reason.
   */
  virtual std::string ParseFunctionCall(const std::string& gen_txt,
                                        int64_t req_id,
                                        rapidjson::GenericValue<rapidjson::UTF8<>>& message,
                                        rapidjson::MemoryPoolAllocator<>& allocator);

  const std::string& GetRole(const std::string& role_name) {
    if (role_name == "system") {
      return roles_[0];
    } else if (role_name == "user") {
      return roles_[1];
    } else if (role_name == "assistant") {
      return roles_[2];
    } else {
      return role_name;
    }
  }

  [[nodiscard]] const std::string& style_name() const { return style_name_; }
  [[nodiscard]] const std::string& system_prompt() const { return system_prompt_; }
  [[nodiscard]] const std::vector<std::string>& roles() const { return roles_; }
  [[nodiscard]] bool support_func_call() const { return support_func_call_; }
  [[nodiscard]] const std::string& func_call_observation_words() const { return func_call_observation_words_; }
  [[nodiscard]] bool add_generation_prompt() const { return add_generation_prompt_; }

private:
  std::string style_name_;                  // Style name.
  std::string system_prompt_;               // System role prompt.
  std::vector<std::string> roles_;          // Roles name. [`system_name`, `user_name`, `assistant_name`]
  bool support_func_call_ = false;          // If support function call.
  std::string func_call_observation_words_; // Function call observation words. Used to early stop.
  bool add_generation_prompt_ = false;      // If true, will add generation prompt in the end of prompt.
};

class QwenStyler : public LLMStyler {
public:
  QwenStyler()
      : LLMStyler("qwen", "You are a helpful assistant.", {"system", "user", "assistant"}, true, "Observation:", true) {
  }
  ~QwenStyler() override = default;

  /**
   * @brief Build prompt for model input from OpenAI interface json body request.
   * @param json_body: Json body from client.
   * @return <if_function_call, prompt, img_urls>: if_function_call is true if the prompt contains function call.
   */
  std::tuple<bool, std::string, std::vector<std::string>> BuildPrompt(const rapidjson::Document& json_body) override;

  /**
   * @brief Parse function call response from generated text and build content and tool_calls array of message
   * member of OpenAI interface response.
   * @param gen_txt: Generated text.
   * @param req_id: Request id.
   * @param message: Message member of OpenAI interface response format.
   * @param allocator: Json allocator.
   * @return stop reason.
   */
  std::string ParseFunctionCall(const std::string& gen_txt,
                                int64_t req_id,
                                rapidjson::GenericValue<rapidjson::UTF8<>>& message,
                                rapidjson::MemoryPoolAllocator<>& allocator) override;
};

class Qwen25Styler : public LLMStyler {
public:
  Qwen25Styler()
      : LLMStyler("qwen2.5",
                  "You are Qwen, created by Alibaba Cloud. You are a helpful assistant.",
                  {"system", "user", "assistant"},
                  true,
                  "",
                  true) {}
  ~Qwen25Styler() override = default;

  /**
   * @brief Build prompt for model input from OpenAI interface json body request.
   * @param json_body: Json body from client.
   * @return <if_function_call, prompt, img_urls>: if_function_call is true if the prompt contains function call.
   */
  std::tuple<bool, std::string, std::vector<std::string>> BuildPrompt(const rapidjson::Document& json_body) override;

  /**
   * @brief Parse function call response from generated text and build content and tool_calls array of message
   * member of OpenAI interface response.
   * @param gen_txt: Generated text.
   * @param req_id: Request id.
   * @param message: Message member of OpenAI interface response format.
   * @param allocator: Json allocator.
   * @return stop reason.
   */
  std::string ParseFunctionCall(const std::string& gen_txt,
                                int64_t req_id,
                                rapidjson::GenericValue<rapidjson::UTF8<>>& message,
                                rapidjson::MemoryPoolAllocator<>& allocator) override;
};

class ChatGlm3Styler : public LLMStyler {
public:
  ChatGlm3Styler() : LLMStyler("chatglm3", "", {"<|system|>", "<|user|>", "<|assistant|>"}, true, "", true) {}
  ~ChatGlm3Styler() override = default;

  /**
   * @brief Build prompt for model input from OpenAI interface json body request.
   * @param json_body: Json body from client.
   * @return <if_function_call, prompt, img_urls>: if_function_call is true if the prompt contains function call.
   */
  std::tuple<bool, std::string, std::vector<std::string>> BuildPrompt(const rapidjson::Document& json_body) override;

  /**
   * @brief Parse function call response from generated text and build content and tool_calls array of message
   * member of OpenAI interface response.
   * @param gen_txt: Generated text.
   * @param req_id: Request id.
   * @param message: Message member of OpenAI interface response format.
   * @param allocator: Json allocator.
   * @return stop reason.
   */
  std::string ParseFunctionCall(const std::string& gen_txt,
                                int64_t req_id,
                                rapidjson::GenericValue<rapidjson::UTF8<>>& message,
                                rapidjson::MemoryPoolAllocator<>& allocator) override;
};

class Glm4Styler : public LLMStyler {
public:
  Glm4Styler()
      : LLMStyler("glm4",
                  "你是一个名为 ChatGLM 的人工智能助手。你是基于智谱AI训练的语言模型 GLM-4 "
                  "模型开发的，你的任务是针对用户的问题和要求提供适当的答复和支持。",
                  {"<|system|>", "<|user|>", "<|assistant|>"},
                  true,
                  "<|observation|>",
                  true) {}
  ~Glm4Styler() override = default;

  /**
   * @brief Build prompt for model input from OpenAI interface json body request.
   * @param json_body: Json body from client.
   * @return <if_function_call, prompt, img_urls>: if_function_call is true if the prompt contains function call.
   */
  std::tuple<bool, std::string, std::vector<std::string>> BuildPrompt(const rapidjson::Document& json_body) override;

  /**
   * @brief Parse function call response from generated text and build content and tool_calls array of message
   * member of OpenAI interface response.
   * @param gen_txt: Generated text.
   * @param req_id: Request id.
   * @param message: Message member of OpenAI interface response format.
   * @param allocator: Json allocator.
   * @return stop reason.
   */
  std::string ParseFunctionCall(const std::string& gen_txt,
                                int64_t req_id,
                                rapidjson::GenericValue<rapidjson::UTF8<>>& message,
                                rapidjson::MemoryPoolAllocator<>& allocator) override;
};

class Llama3Styler : public LLMStyler {
public:
  Llama3Styler() : LLMStyler("llama3", "", {"system", "user", "assistant"}, false, "", true) {}
  ~Llama3Styler() override = default;

  /**
   * @brief Build prompt for model input from OpenAI interface json body request.
   * @param json_body: Json body from client.
   * @return <if_function_call, prompt, img_urls>: if_function_call is true if the prompt contains function call.
   */
  std::tuple<bool, std::string, std::vector<std::string>> BuildPrompt(const rapidjson::Document& json_body) override;

  /**
   * @brief Parse function call response from generated text and build content and tool_calls array of message
   * member of OpenAI interface response.
   * @param gen_txt: Generated text.
   * @param req_id: Request id.
   * @param message: Message member of OpenAI interface response format.
   * @param allocator: Json allocator.
   * @return stop reason.
   */
  std::string ParseFunctionCall(const std::string& gen_txt,
                                int64_t req_id,
                                rapidjson::GenericValue<rapidjson::UTF8<>>& message,
                                rapidjson::MemoryPoolAllocator<>& allocator) override;
};

class Internlm2Styler : public LLMStyler {
public:
  Internlm2Styler()
      : LLMStyler("internlm2",
                  "You are an AI assistant whose name is InternLM (书生·浦语).\n"
                  "- InternLM (书生·浦语) is a conversational language model that is developed by Shanghai AI "
                  "Laboratory (上海人工智能实验室). It is designed to be helpful, honest, and harmless.\n"
                  "- InternLM (书生·浦语) can understand and communicate fluently in the language chosen by the user "
                  "such as English and 中文.",
                  {"system", "user", "assistant"},
                  true,
                  "Observation:",
                  true) {}
  ~Internlm2Styler() override = default;

  /**
   * @brief Build prompt for model input from OpenAI interface json body request.
   * @param json_body: Json body from client.
   * @return <if_function_call, prompt, img_urls>: if_function_call is true if the prompt contains function call.
   */
  std::tuple<bool, std::string, std::vector<std::string>> BuildPrompt(const rapidjson::Document& json_body) override;

  /**
   * @brief Parse function call response from generated text and build content and tool_calls array of message
   * member of OpenAI interface response.
   * @param gen_txt: Generated text.
   * @param req_id: Request id.
   * @param message: Message member of OpenAI interface response format.
   * @param allocator: Json allocator.
   * @return stop reason.
   */
  std::string ParseFunctionCall(const std::string& gen_txt,
                                int64_t req_id,
                                rapidjson::GenericValue<rapidjson::UTF8<>>& message,
                                rapidjson::MemoryPoolAllocator<>& allocator) override;
};

class Internvl2Internlm2Styler : public LLMStyler {
public:
  Internvl2Internlm2Styler()
      : LLMStyler("internvl2-internlm2",
                  "你是由上海人工智能实验室联合商汤科技开发的书生多模态大模型，英文名叫InternVL, "
                  "是一个有用无害的人工智能助手。",
                  {"system", "user", "assistant"},
                  false,
                  "",
                  true) {}
  ~Internvl2Internlm2Styler() override = default;

  /**
   * @brief Build prompt for model input from OpenAI interface json body request.
   * @param json_body: Json body from client.
   * @return <if_function_call, prompt, img_urls>: if_function_call is true if the prompt contains function call.
   */
  std::tuple<bool, std::string, std::vector<std::string>> BuildPrompt(const rapidjson::Document& json_body) override;

  /**
   * @brief Parse function call response from generated text and build content and tool_calls array of message
   * member of OpenAI interface response.
   * @param gen_txt: Generated text.
   * @param req_id: Request id.
   * @param message: Message member of OpenAI interface response format.
   * @param allocator: Json allocator.
   * @return stop reason.
   */
  std::string ParseFunctionCall(const std::string& gen_txt,
                                int64_t req_id,
                                rapidjson::GenericValue<rapidjson::UTF8<>>& message,
                                rapidjson::MemoryPoolAllocator<>& allocator) override;
};

class Internvl2Phi3Styler : public LLMStyler {
public:
  Internvl2Phi3Styler()
      : LLMStyler("internvl2-phi3",
                  "你是由上海人工智能实验室联合商汤科技开发的书生多模态大模型，英文名叫InternVL, "
                  "是一个有用无害的人工智能助手。",
                  {"<|system|>\n", "<|user|>\n", "<|assistant|>\n"},
                  false,
                  "",
                  true) {}
  ~Internvl2Phi3Styler() override = default;

  /**
   * @brief Build prompt for model input from OpenAI interface json body request.
   * @param json_body: Json body from client.
   * @return <if_function_call, prompt, img_urls>: if_function_call is true if the prompt contains function call.
   */
  std::tuple<bool, std::string, std::vector<std::string>> BuildPrompt(const rapidjson::Document& json_body) override;

  /**
   * @brief Parse function call response from generated text and build content and tool_calls array of message
   * member of OpenAI interface response.
   * @param gen_txt: Generated text.
   * @param req_id: Request id.
   * @param message: Message member of OpenAI interface response format.
   * @param allocator: Json allocator.
   * @return stop reason.
   */
  std::string ParseFunctionCall(const std::string& gen_txt,
                                int64_t req_id,
                                rapidjson::GenericValue<rapidjson::UTF8<>>& message,
                                rapidjson::MemoryPoolAllocator<>& allocator) override;
};

class Internvl2Qwen2Styler : public Internvl2Internlm2Styler {
public:
  Internvl2Qwen2Styler() = default;
  ~Internvl2Qwen2Styler() override = default;
};

class Internvl25Styler : public LLMStyler {
public:
  Internvl25Styler()
      : LLMStyler(
          "internvl2.5",
          "你是书生·万象，英文名是InternVL，是由上海人工智能实验室、清华大学及多家合作单位联合开发的多模态大语言模型。",
          {"system", "user", "assistant"},
          false,
          "",
          true) {}
  ~Internvl25Styler() override = default;

  /**
   * @brief Build prompt for model input from OpenAI interface json body request.
   * @param json_body: Json body from client.
   * @return <if_function_call, prompt, img_urls>: if_function_call is true if the prompt contains function call.
   */
  std::tuple<bool, std::string, std::vector<std::string>> BuildPrompt(const rapidjson::Document& json_body) override;

  /**
   * @brief Parse function call response from generated text and build content and tool_calls array of message
   * member of OpenAI interface response.
   * @param gen_txt: Generated text.
   * @param req_id: Request id.
   * @param message: Message member of OpenAI interface response format.
   * @param allocator: Json allocator.
   * @return stop reason.
   */
  std::string ParseFunctionCall(const std::string& gen_txt,
                                int64_t req_id,
                                rapidjson::GenericValue<rapidjson::UTF8<>>& message,
                                rapidjson::MemoryPoolAllocator<>& allocator) override;
};

class QwenvlStyler : public LLMStyler {
public:
  QwenvlStyler()
      : LLMStyler("qwenvl", "You are a helpful assistant.", {"system", "user", "assistant"}, false, "", true) {
    img_ctx_replace_str_.append("<img>");
    for (size_t k = 0; k < 256; k++) { // 256: token count for every image patch.
      img_ctx_replace_str_.append("<imgpad>");
    }
    img_ctx_replace_str_.append("</img>");
  }
  ~QwenvlStyler() override = default;

  /**
   * @brief Build prompt for model input from OpenAI interface json body request.
   * @param json_body: Json body from client.
   * @return <if_function_call, prompt, img_urls>: if_function_call is true if the prompt contains function call.
   */
  std::tuple<bool, std::string, std::vector<std::string>> BuildPrompt(const rapidjson::Document& json_body) override;

  /**
   * @brief Parse function call response from generated text and build content and tool_calls array of message
   * member of OpenAI interface response.
   * @param gen_txt: Generated text.
   * @param req_id: Request id.
   * @param message: Message member of OpenAI interface response format.
   * @param allocator: Json allocator.
   * @return stop reason.
   */
  std::string ParseFunctionCall(const std::string& gen_txt,
                                int64_t req_id,
                                rapidjson::GenericValue<rapidjson::UTF8<>>& message,
                                rapidjson::MemoryPoolAllocator<>& allocator) override;

private:
  std::string img_ctx_replace_str_{};
};

class Qwen2vlStyler : public LLMStyler {
public:
  Qwen2vlStyler()
      : LLMStyler("qwen2vl", "You are a helpful assistant.", {"system", "user", "assistant"}, false, "", true) {}
  ~Qwen2vlStyler() override = default;

  /**
   * @brief Build prompt for model input from OpenAI interface json body request.
   * @param json_body: Json body from client.
   * @return <if_function_call, prompt, img_urls>: if_function_call is true if the prompt contains function call.
   */
  std::tuple<bool, std::string, std::vector<std::string>> BuildPrompt(const rapidjson::Document& json_body) override;

  /**
   * @brief Parse function call response from generated text and build content and tool_calls array of message
   * member of OpenAI interface response.
   * @param gen_txt: Generated text.
   * @param req_id: Request id.
   * @param message: Message member of OpenAI interface response format.
   * @param allocator: Json allocator.
   * @return stop reason.
   */
  std::string ParseFunctionCall(const std::string& gen_txt,
                                int64_t req_id,
                                rapidjson::GenericValue<rapidjson::UTF8<>>& message,
                                rapidjson::MemoryPoolAllocator<>& allocator) override;
};

class Phi3Styler : public LLMStyler {
public:
  Phi3Styler()
      : LLMStyler("phi3",
                  "You are a helpful assistant.",
                  {"<|system|>\n", "<|user|>\n", "<|assistant|>\n"},
                  false,
                  "",
                  true) {}
  ~Phi3Styler() override = default;

  /**
   * @brief Build prompt for model input from OpenAI interface json body request.
   * @param json_body: Json body from client.
   * @return <if_function_call, prompt, img_urls>: if_function_call is true if the prompt contains function call.
   */
  std::tuple<bool, std::string, std::vector<std::string>> BuildPrompt(const rapidjson::Document& json_body) override;

  /**
   * @brief Parse function call response from generated text and build content and tool_calls array of message
   * member of OpenAI interface response.
   * @param gen_txt: Generated text.
   * @param req_id: Request id.
   * @param message: Message member of OpenAI interface response format.
   * @param allocator: Json allocator.
   * @return stop reason.
   */
  std::string ParseFunctionCall(const std::string& gen_txt,
                                int64_t req_id,
                                rapidjson::GenericValue<rapidjson::UTF8<>>& message,
                                rapidjson::MemoryPoolAllocator<>& allocator) override;
};

class LLMStylerFactory {
public:
  LLMStylerFactory() = default;
  virtual ~LLMStylerFactory() = default;
  static std::unique_ptr<LLMStyler> CreateLLMStyler(const std::string& llm_style);
};

} // namespace netease::grps

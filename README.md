# grps-trtllm

## 目录

* [1. 说明](#1-说明)
* [2. 工程结构](#2-工程结构)
* [3. 本地开发与调试](#3-本地开发与调试)
    * [3.1 拉取代码](#31-拉取代码)
    * [3.2 创建容器](#32-创建容器)
    * [3.3 构建trtllm引擎](#33-构建trtllm引擎)
    * [3.3 修改inference.yml配置](#33-修改inferenceyml配置)
    * [3.5 构建与部署](#35-构建与部署)
    * [3.6 模拟请求](#36-模拟请求)
    * [3.7 指标观测](#37-指标观测)
    * [3.8 关闭服务](#38-关闭服务)
* [4. docker部署](#4-docker部署)
* [5. 与xinference-vllm性能比较](#5-与xinference-vllm性能比较)

## 1. 说明

[grps](https://github.com/NetEase-Media/grps)接入[trtllm](https://github.com/NVIDIA/TensorRT-LLM)
实现```LLM```服务，相比较[triton-trtllm](https://github.com/triton-inference-server/tensorrtllm_backend)
实现服务。有如下优势：

* 通过纯```C++```实现完整```LLM```服务，包含```tokenizer```部分。
* 不存在```triton_server <--> tokenizer_backend <--> trtllm_backend```之间的进程间通信。
* 通过```grps```的自定义```http```功能实现```OpenAI```接口协议，支持```chat```和```function call```模式。
* 支持扩展不同```LLM```的```prompt```构建风格以及生成结果的解析风格，以实现不同```LLM```的```chat```
  和```function call```模式。
* 通过测试，```grps-trtllm```相比较```triton-trtllm```性能有稳定的提升。

当前问题：

* 由于不同家族系的```LLM```的```chat```和```function call```
  的```prompt```构建以及结果解析风格不同，所以需要实现不同```LLM```家族的```styler```，见```src/llm_styler.cc/.h```
  。目前仅实现了```qwen```
  ，后续可以实现其他家族的```styler```，用户可以自行扩展。拓展后需要修改```conf/inference.yml```的```llm_style```为对应的家族名。
  不同家族的```styler```持续开发中...。
* 当前基于```tensorrt-llm v0.10.0```进行的实现，新版本持续支持中...。
* ```grps```刚支持```trtllm```没多久，欢迎提交```pr```贡献支持更多的```LLM```家族的```styler```以及修复bug。

## 2. 工程结构

```text
|-- client                              # 客户端样例
|   |--openai_benchmark.py              # 通过OpenAI客户端进行benchmark
|   |--openai_cli.py                    # 通过OpenAI客户端进行chat
|   |--openai_func_call.py              # 通过OpenAI客户端进行function call
|   |--triton_benchmark.py              # Triton trtllm server benchmark脚本
|-- conf                                # 配置文件
|   |-- inference.yml                   # 推理配置
|   |-- server.yml                      # 服务配置
|-- data                                # 数据文件
|-- docker                              # docker镜像构建
|-- second_party                        # 第二方依赖
|   |-- grps-server-framework           # grps框架依赖
|-- src                                 # 自定义源码
|   |-- constants.cc/.h                 # 常量定义
|   |-- customized_inferer.cc/.h        # 自定义推理器
|   |-- grps_server_customized.cc/.h    # 自定义库初始化
|   |-- llm_styler.cc/.h                # LLM风格定义，prompt构建，结果解析
|   |-- tokenizer.cc/.h                 # Tokenizer实现
|   |-- trtllm_model_instance.cc/.h     # TensorRT-LLM模型实例
|   |-- trtllm_model_state.cc/.h        # TensorRT-LLM模型状态
|   |-- utils.cc/.h                     # 工具
|   |-- main.cc                         # 本地单元测试
|-- third_party                         # 第三方依赖
|-- build.sh                            # 构建脚本
|-- CMakelists.txt                      # 工程构建文件
|-- .clang-format                       # 代码格式化配置文件
|-- .config                             # 工程配置文件，包含一些工程配置开关
```

## 3. 本地开发与调试

以qwen2-instruct为例。

### 3.1 拉取代码

```bash
git clone https://github.com/NetEase-Media/grps_trtllm.git
cd grps_trtllm
git submodule update --init --recursive
```

### 3.2 创建容器

使用```registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda12.4_cudnn8.9_trtllm0.10.0_py3.10```镜像。
这里挂载了当前目录用于构建工程并保留构建产物，挂载/tmp目录用于保存构建的trtllm引擎文件。参考```triton-trtllm```
设置共享内存大小，解除物理内存锁定限制，设置栈大小，配置参数```--shm-size=2g --ulimit memlock=-1 --ulimit stack=67108864```。

```bash
# 创建容器
docker run -itd --name grps_trtllm_dev --runtime=nvidia --network host --shm-size=2g --ulimit memlock=-1 --ulimit stack=67108864 \
-v $(pwd):/grps_dev -v /tmp:/tmp -w /grps_dev \
registry.cn-hangzhou.aliyuncs.com/opengrps/grps_gpu:grps1.1.0_cuda12.4_cudnn8.9_trtllm0.10.0_py3.10 bash
# 进入开发容器
docker exec -it grps_trtllm_dev bash
```

### 3.3 构建trtllm引擎

```bash
# 下载Qwen2-7B-Instruct模型
apt update && apt install git-lfs
git lfs install
git clone https://huggingface.co/Qwen/Qwen2-7B-Instruct /tmp/Qwen2-7B-Instruct

# 进入TensorRT-LLM/examples/qwen目录，参考README进行构建trtllm引擎。
cd third_party/TensorRT-LLM/examples/qwen
# 这里以tp4为例进行构建，即使用4张卡进行tensor并行推理
# 转换ckpt
python3 convert_checkpoint.py --model_dir /tmp/Qwen2-7B-Instruct \
--output_dir /tmp/Qwen2-7B-Instruct/tllm_checkpoint_4gpu_tp4/ --dtype float16 --tp_size 4
# 构建引擎，当出现OOM，可以适当缩小max_batch_size，max_input_len，max_output_len等参数
trtllm-build --checkpoint_dir /tmp/Qwen2-7B-Instruct/tllm_checkpoint_4gpu_tp4/ \
--output_dir /tmp/Qwen2-7B-Instruct/trt_engines/fp16_4gpu/ \
--gemm_plugin float16 --max_batch_size 16 --paged_kv_cache enable --max_input_len 2048 --max_output_len 512
# 回到工程根目录
cd ../../../../
```

### 3.3 修改inference.yml配置

修改[conf/inference.yml](conf/inference.yml)中```inferer_args```相关参数。注意修改```tokenizer_path```
和```gpt_model_path```为新路径，更多核心参数见如下：

```yaml
models:
  - name: trtllm_model
    ...
    inferer_args:
      # llm style used to build prompt(chat or function call) and parse generated response for openai interface.
      # Current support {`qwen`}.
      llm_style: qwen

      # tokenizer config.
      # path of tokenizer. Must be set. Could be tokenizer.json(hf tokenizer), tokenizer.model(sentencepiece
      # tokenizer) or tokenizer_model(RWKV world tokenizer).
      tokenizer_path: /tmp/Qwen2-7B-Instruct/tokenizer.json
      tokenizer_parallelism: 16 # tokenizers count for parallel tokenization. Will be set to 1 if not set.
      end_token_id: 151643 # end token id of tokenizer. Null if not set.
      pad_token_id: 151643 # pad token id of tokenizer. Null if not set.
      stop_words: # additional stop words of tokenizer. Empty if not set.
        - "<|im_start|>"
        - "<|im_end|>"
        - "<|endoftext|>"
      bad_words: # additional bad words of tokenizer. Empty if not set.
      special_tokens_id: # special tokens of tokenizer. Empty if not set.
        - 151643 # "<|endoftext|>"
        - 151644 # "<|im_start|>"
        - 151645 # "<|im_end|>"
      skip_special_tokens: true # skip special tokens when decoding. Will be set to true if not set.

      # trtllm config.
      gpt_model_type: inflight_fused_batching # must be `V1`(==`v1`) or `inflight_batching`(==`inflight_fused_batching`).
      gpt_model_path: /tmp/Qwen2-7B-Instruct/trt_engines/fp16_4gpu/ # path of model. Must be set.
      batch_scheduler_policy: guaranteed_no_evict # must be `max_utilization` or `guaranteed_no_evict`.
      kv_cache_free_gpu_mem_fraction: 0.6 # will be set to 0.9 or `max_tokens_in_paged_kv_cache` if not set.
      exclude_input_in_output: true # will be set to false if not set.
```

### 3.5 构建与部署

```bash
# 构建
grpst archive .

# 部署，注意使用mpi方式启动，参数为并行推理的GPU数量
# 首次构建完后，修改配置后可以直接启动服务无需重新构建，通过--inference_conf以及--server_conf参数指定.
# grpst start --inference_conf=conf/inference.yml --server_conf=conf/server.yml --mpi_np 4
grpst start ./server.mar --mpi_np 4

# 查看服务状态
grpst ps
# 如下输出
PORT(HTTP,RPC)      NAME                PID                 DEPLOY_PATH         
9997                my_grps             65322               /home/appops/.grps/my_grps
```

### 3.6 模拟请求

```bash
# curl命令非stream请求
curl --no-buffer http://127.0.0.1:9997//v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "qwen2-instruct",
    "messages": [
      {
        "role": "system",
        "content": "You are a helpful assistant."
      },
      {
        "role": "user",
        "content": "你好，你是谁？"
      }
    ]
  }'
# 返回如下：
: '
{
 "id": "chatcmpl-737",
 "object": "chat.completion",
 "created": 1724291091,
 "model": "qwen2-instruct",
 "system_fingerprint": "grps-trtllm-server",
 "choices": [
  {
   "index": 0,
   "message": {
    "role": "assistant",
    "content": "你好！我是阿里云开发的一款超大规模语言模型，我叫通义千问。作为一个AI助手，我的目标是帮助用户获得准确、有用的信息，解决他们的问题和困惑。我被设计成能够进行多轮对话、保持逻辑一致，并能够覆盖各种主题，包括但不限于科技、文化、生活常识等。无论是你需要学习知识、完成任务，还是只是想聊天解闷，我都在这里为你服务。请随时告诉我你需要什么帮助，我会尽力提供支持。"
   },
   "logprobs": null,
   "finish_reason": "stop"
  }
 ],
 "usage": {
  "prompt_tokens": 24,
  "completion_tokens": 24,
  "total_tokens": 48
 }
}
'

# curl命令stream请求
curl --no-buffer http://127.0.0.1:9997//v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "qwen2-instruct",
    "messages": [
      {
        "role": "system",
        "content": "You are a helpful assistant."
      },
      {
        "role": "user",
        "content": "你好，你是谁？"
      }
    ],
    "stream": true
  }'
# 返回如下：
: '
data: {"id":"chatcmpl-4","object":"chat.completion.chunk","created":1724295387,"model":"qwen2-instruct","system_fingerprint":"grps-trtllm-server","choices":[{"index":0,"delta":{"role":"assistant","content":"你好"},"logprobs":null,"finish_reason":null}]}
data: {"id":"chatcmpl-4","object":"chat.completion.chunk","created":1724295387,"model":"qwen2-instruct","system_fingerprint":"grps-trtllm-server","choices":[{"index":0,"delta":{"content":"！"},"logprobs":null,"finish_reason":null}]}
data: {"id":"chatcmpl-4","object":"chat.completion.chunk","created":1724295387,"model":"qwen2-instruct","system_fingerprint":"grps-trtllm-server","choices":[{"index":0,"delta":{"content":"我是"},"logprobs":null,"finish_reason":null}]}
'

# openai_cli.py 非stream请求
python3 client/openai_cli.py 127.0.0.1:9997 "你好，你是谁？" false
# 返回如下：
: '
ChatCompletion(id='chatcmpl-5', choices=[Choice(finish_reason='stop', index=0, logprobs=None, message=ChatCompletionMessage(content='你好！我是阿里云开发的一款超大规模语言模型，我叫通义千问。作为一个AI助手，我的目标是帮助用户获得准确、有用的信息，解决他们的问题和困惑。我被设计成能够进行多轮对话、保持逻辑一致，并能够覆盖各种主题，包括但不限于科技、文化、生活常识等。无论是你需要学习知识、完成任务，还是只是想聊天解闷，我都在这里为你服务。请随时告诉我你需要什么帮助，我会尽力提供支持。', refusal=None, role='assistant', function_call=None, tool_calls=None))], created=1724295422, model='qwen2', object='chat.completion', service_tier=None, system_fingerprint='grps-trtllm-server', usage=CompletionUsage(completion_tokens=102, prompt_tokens=24, total_tokens=126))
'

# openai_cli.py stream请求
python3 client/openai_cli.py 127.0.0.1:9997 "你好，你是谁？" true
# 返回如下：
: '
ChatCompletionChunk(id='chatcmpl-6', choices=[Choice(delta=ChoiceDelta(content='你好', function_call=None, refusal=None, role='assistant', tool_calls=None), finish_reason=None, index=0, logprobs=None)], created=1724295460, model='qwen2', object='chat.completion.chunk', service_tier=None, system_fingerprint='grps-trtllm-server', usage=None)
ChatCompletionChunk(id='chatcmpl-6', choices=[Choice(delta=ChoiceDelta(content='！', function_call=None, refusal=None, role=None, tool_calls=None), finish_reason=None, index=0, logprobs=None)], created=1724295460, model='qwen2', object='chat.completion.chunk', service_tier=None, system_fingerprint='grps-trtllm-server', usage=None)
ChatCompletionChunk(id='chatcmpl-6', choices=[Choice(delta=ChoiceDelta(content='我是', function_call=None, refusal=None, role=None, tool_calls=None), finish_reason=None, index=0, logprobs=None)], created=1724295460, model='qwen2', object='chat.completion.chunk', service_tier=None, system_fingerprint='grps-trtllm-server', usage=None)
'

# openai_func_call.py进行function call模拟
python3 client/openai_func_call.py 127.0.0.1:9997
# 返回如下：
: '
Query server with question: What's the weather like in Boston today? ...
Server response: thought:  I need to use the get_current_weather API to find out the current weather in Boston., call local function(get_current_weather) with arguments: location=Boston,MA, unit=fahrenheit
Send the result back to the server with function result(59.0) ...
Final server response: The current weather in Boston is 59 degrees Fahrenheit.
'
```

### 3.7 指标观测

通过访问```http://ip:9997/``` 可以查看服务的指标信息。如下指标：
![metrics_0.png](data/metrics_0.png)<br>
![metrics_1.png](data/metrics_1.png)

### 3.8 关闭服务

```bash
# 关闭服务
grpst stop my_grps
```

## 4. docker部署

```bash
# 构建自定义工程docker镜像
docker build -t grps_trtllm_server:1.0.0 -f docker/Dockerfile .

# 使用上面构建好的镜像启动docker容器
# 注意挂载/tmp目录，因为构建的trtllm引擎文件在/tmp目录下
# 映射服务端口9997
docker run -itd --runtime=nvidia --name="grps_trtllm_server" --shm-size=2g --ulimit memlock=-1 \
--ulimit stack=67108864 -v /tmp:/tmp -p 9997:9997 \
grps_trtllm_server:1.0.0 grpst start server.mar --mpi_np 4

# 使用docker logs可以跟踪服务日志
docker logs -f grps_trtllm_server

# 模拟请求见3.6章节所述

# 关闭容器
docker rm -f grps_trtllm_server
```

### 5. 与xinference-vllm性能比较

这里不再比较与```triton-trtllm```性能，因为它不是```OpenAI```协议。比较与```xinference-vllm```服务的性能差异。

```
GPU: RTX 2080Ti * 4
CUDA: cuda_12.4
Trtllm: 0.10.0
xinference: 0.14.1
vLLM: 0.5.4
CPU: Intel(R) Xeon(R) Gold 6242R CPU @ 3.10GHz
Mem：128G
LLM: Qwen2-7B
```

短输入输出：
固定输入（华盛顿是谁？），输入输出总长度140 tokens左右。

| 服务 \ 吞吐(tokens/s) \ 并发 | 1       | 2       | 4       | 6       | 8       | 10      | 16      |
|------------------------|---------|---------|---------|---------|---------|---------|---------|
| xinference-vllm        | 98.79   | 181.76  | 343.55  | 436.62  | 580.80  | 660.71  | 968.86  |
| grps-trtllm            | 128.57  | 231.68  | 429.19  | 561.54  | 714.15  | 836.60  | 1226.88 |
| 同比                     | +30.14% | +27.46% | +24.93% | +28.61% | +22.96% | +26.62% | +26.63% |

长输入输出：
固定输入为1.2k左右tokens数量的文章，输出为150左右token数量的总结。

| 服务 \ 吞吐(tokens/s) \ 并发 | 1       | 2       | 4       | 6       | 8       | 10      | 16      |
|------------------------|---------|---------|---------|---------|---------|---------|---------|
| xinference-vllm        | 681.38  | 1112.14 | 1797.84 | 2135.98 | 2507.70 | 2669.51 | 3511.76 |
| grps-trtllm            | 797.51  | 1300.54 | 2042.17 | 2400.99 | 2763.28 | 2947.73 | 3637.28 |
| 同比                     | +17.04% | +16.94% | +13.59% | +12.41% | +10.19% | +10.42% | +3.57%  |

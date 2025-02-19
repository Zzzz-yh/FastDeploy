# 客户端访问说明
本文以访问使用fastdeployserver部署的yolov5模型为例，讲述客户端如何请求服务端进行推理服务。关于如何使用fastdeployserver部署yolov5模型，可以参考文档[yolov5服务化部署](../../../examples/vision/detection/yolov5/serving)

## 基本原理介绍
fastdeployserver实现了由[kserve](https://github.com/kserve/kserve)提出的为机器学习模型推理服务而设计的[Predict Protocol协议](https://github.com/kserve/kserve/blob/master/docs/predict-api/v2/required_api.md) API，该API既简单易用同时又支持高性能部署的使用场景，目前提供基于HTTP和GRPC两种网络协议的访问方式。


当fastdeployserver启动后，默认情况下，8000端口用于响应HTTP请求，8001端口用于响应GRPC请求。用户需要请求的资源通常有两种：

### **模型的元信息（metadata)**

**HTTP**

访问方式： GET `v2/models/${MODEL_NAME}[/versions/${MODEL_VERSION}]`

使用GET请求该url路径可以获取参与服务的模型的元信息，其中`${MODEL_NAME}`表示模型的名字，${MODEL_VERSION}表示模型的版本。服务器会把模型的元信息以json格式返回，返回的格式为一个字典，以$metadata_model_response表示返回的对象，各字段和内容形式表示如下：

```json
$metadata_model_response =
    {
      "name" : $string,
      "versions" : [ $string, ... ] #optional,
      "platform" : $string,
      "inputs" : [ $metadata_tensor, ... ],
      "outputs" : [ $metadata_tensor, ... ]
    }

$metadata_tensor =
    {
      "name" : $string,
      "datatype" : $string,
      "shape" : [ $number, ... ]
    }
```

**GRPC**

模型服务的GRPC定义为

```text
service GRPCInferenceService
{
  // Check liveness of the inference server.
  rpc ServerLive(ServerLiveRequest) returns (ServerLiveResponse) {}

  // Check readiness of the inference server.
  rpc ServerReady(ServerReadyRequest) returns (ServerReadyResponse) {}

  // Check readiness of a model in the inference server.
  rpc ModelReady(ModelReadyRequest) returns (ModelReadyResponse) {}

  // Get server metadata.
  rpc ServerMetadata(ServerMetadataRequest) returns (ServerMetadataResponse) {}

  // Get model metadata.
  rpc ModelMetadata(ModelMetadataRequest) returns (ModelMetadataResponse) {}

  // Perform inference using a specific model.
  rpc ModelInfer(ModelInferRequest) returns (ModelInferResponse) {}
}
```

访问方式：使用GRPC客户端调用模型服务GRPC接口中定义的ModelMetadata方法。

接口中请求的ModelMetadataRequest message和返回的ServerMetadataResponse message结构如下，可以看到和上面的HTTP里使用的json结构基本相同。

```text
message ModelMetadataRequest
{
  // The name of the model.
  string name = 1;

  // The version of the model to check for readiness. If not given the
  // server will choose a version based on the model and internal policy.
  string version = 2;
}

message ModelMetadataResponse
{
  // Metadata for a tensor.
  message TensorMetadata
  {
    // The tensor name.
    string name = 1;

    // The tensor data type.
    string datatype = 2;

    // The tensor shape. A variable-size dimension is represented
    // by a -1 value.
    repeated int64 shape = 3;
  }

  // The model name.
  string name = 1;

  // The versions of the model available on the server.
  repeated string versions = 2;

  // The model's platform. See Platforms.
  string platform = 3;

  // The model's inputs.
  repeated TensorMetadata inputs = 4;

  // The model's outputs.
  repeated TensorMetadata outputs = 5;
}
```


### **推理服务**

**HTTP**

访问方式：POST `v2/models/${MODEL_NAME}[/versions/${MODEL_VERSION}]/infer`

使用POST请求该url路径可以请求模型的推理服务，获取推理结果。POST请求中的数据同样以json格式上传，以$inference_request表示上传的对象，各字段和内容形式表示如下：
```json
 $inference_request =
    {
      "id" : $string #optional,
      "parameters" : $parameters #optional,
      "inputs" : [ $request_input, ... ],
      "outputs" : [ $request_output, ... ] #optional
    }

$request_input =
    {
      "name" : $string,
      "shape" : [ $number, ... ],
      "datatype"  : $string,
      "parameters" : $parameters #optional,
      "data" : $tensor_data
    }

$request_output =
    {
      "name" : $string,
      "parameters" : $parameters #optional,
    }

$parameters =
{
  $parameter, ...
}

$parameter = $string : $string | $number | $boolean
```
其中$tensor_data表示一维或多维数组，如果是一维数据，必须按照行主序的方式进行排列tensor中的数据。
服务器推理完成后，返回结果数据，以$inference_response表示返回的对象，各字段和内容形式表示如下：

```json
$inference_response =
    {
      "model_name" : $string,
      "model_version" : $string #optional,
      "id" : $string,
      "parameters" : $parameters #optional,
      "outputs" : [ $response_output, ... ]
    }

$response_output =
    {
      "name" : $string,
      "shape" : [ $number, ... ],
      "datatype"  : $string,
      "parameters" : $parameters #optional,
      "data" : $tensor_data
    }
```

**GRPC**

访问方式：使用GRPC客户端调用模型服务GRPC接口中定义的ModelInfer方法。

接口中请求的ModelInferRequest message和返回的ModelInferResponse message结构如下，更完整的结构定义可以参考kserve Predict Protocol [GRPC部分](https://github.com/kserve/kserve/blob/master/docs/predict-api/v2/required_api.md#grpc)

```text
message ModelInferRequest
{
  // An input tensor for an inference request.
  message InferInputTensor
  {
    // The tensor name.
    string name = 1;

    // The tensor data type.
    string datatype = 2;

    // The tensor shape.
    repeated int64 shape = 3;

    // Optional inference input tensor parameters.
    map<string, InferParameter> parameters = 4;

    // The tensor contents using a data-type format. This field must
    // not be specified if "raw" tensor contents are being used for
    // the inference request.
    InferTensorContents contents = 5;
  }

  // An output tensor requested for an inference request.
  message InferRequestedOutputTensor
  {
    // The tensor name.
    string name = 1;

    // Optional requested output tensor parameters.
    map<string, InferParameter> parameters = 2;
  }

  // The name of the model to use for inferencing.
  string model_name = 1;

  // The version of the model to use for inference. If not given the
  // server will choose a version based on the model and internal policy.
  string model_version = 2;

  // Optional identifier for the request. If specified will be
  // returned in the response.
  string id = 3;

  // Optional inference parameters.
  map<string, InferParameter> parameters = 4;

  // The input tensors for the inference.
  repeated InferInputTensor inputs = 5;

  // The requested output tensors for the inference. Optional, if not
  // specified all outputs produced by the model will be returned.
  repeated InferRequestedOutputTensor outputs = 6;

  // The data contained in an input tensor can be represented in "raw"
  // bytes form or in the repeated type that matches the tensor's data
  // type. To use the raw representation 'raw_input_contents' must be
  // initialized with data for each tensor in the same order as
  // 'inputs'. For each tensor, the size of this content must match
  // what is expected by the tensor's shape and data type. The raw
  // data must be the flattened, one-dimensional, row-major order of
  // the tensor elements without any stride or padding between the
  // elements. Note that the FP16 and BF16 data types must be represented as
  // raw content as there is no specific data type for a 16-bit float type.
  //
  // If this field is specified then InferInputTensor::contents must
  // not be specified for any input tensor.
  repeated bytes raw_input_contents = 7;
}

message ModelInferResponse
{
  // An output tensor returned for an inference request.
  message InferOutputTensor
  {
    // The tensor name.
    string name = 1;

    // The tensor data type.
    string datatype = 2;

    // The tensor shape.
    repeated int64 shape = 3;

    // Optional output tensor parameters.
    map<string, InferParameter> parameters = 4;

    // The tensor contents using a data-type format. This field must
    // not be specified if "raw" tensor contents are being used for
    // the inference response.
    InferTensorContents contents = 5;
  }

  // The name of the model used for inference.
  string model_name = 1;

  // The version of the model used for inference.
  string model_version = 2;

  // The id of the inference request if one was specified.
  string id = 3;

  // Optional inference response parameters.
  map<string, InferParameter> parameters = 4;

  // The output tensors holding inference results.
  repeated InferOutputTensor outputs = 5;

  // The data contained in an output tensor can be represented in
  // "raw" bytes form or in the repeated type that matches the
  // tensor's data type. To use the raw representation 'raw_output_contents'
  // must be initialized with data for each tensor in the same order as
  // 'outputs'. For each tensor, the size of this content must match
  // what is expected by the tensor's shape and data type. The raw
  // data must be the flattened, one-dimensional, row-major order of
  // the tensor elements without any stride or padding between the
  // elements. Note that the FP16 and BF16 data types must be represented as
  // raw content as there is no specific data type for a 16-bit float type.
  //
  // If this field is specified then InferOutputTensor::contents must
  // not be specified for any output tensor.
  repeated bytes raw_output_contents = 6;
}
```


## 客户端工具

了解了fastdeployserver服务提供的接口之后，用户可以HTTP客户端工具来请求HTTP服务器，或者是使用GRPC客户端工具请求GRPC服务器。默认情况下，fastdeployserver启动后，8000端口用于响应HTTP请求，8001端口用于响应GRPC请求。

### 使用HTTP客户端

这里分别介绍如何使用tritonclient和requests库来访问fastdeployserver的HTTP服务，第一种工具是专门为模型服务做的客户端，对请求和响应进行了封装，方便用户使用。而第二种工具通用的http客户端工具，使用该工具进行访问可以帮助用户更好地理解上述原理描述中的数据结构。

一. 使用tritonclient访问服务

安装tritonclient\[http\]

```bash
pip install tritonclient[http]
```

1.获取yolov5的模型元数据
```python
import tritonclient.http as httpclient  # 导入httpclient
server_addr = 'localhost:8000'  # 这里写fastdeployserver服务器的实际地址
client = httpclient.InferenceServerClient(server_addr)  # 创建client
model_metadata = client.get_model_metadata(
      model_name='yolov5', model_version='1') # 请求yolov5模型的元数据
```
可以打印看一下模型的输入和输出有哪些
```python
print(model_metadata.inputs)
```

```text
[{'name': 'INPUT', 'datatype': 'UINT8', 'shape': [-1, -1, -1, 3]}]
```

```python
print(model_metadata.outputs)
```

```text
[{'name': 'detction_result', 'datatype': 'BYTES', 'shape': [-1, -1]}]
```

2.请求推理服务

根据模型的inputs和outputs构造数据，然后请求推理

```python
# 假设图像数据的文件名为000000014439.jpg
import cv2
image = cv2.imread('000000014439.jpg')
image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)[None]

inputs = []
infer_input = httpclient.InferInput('INPUT', image.shape, 'UINT8') # 构造输入
infer_input.set_data_from_numpy(image)  # 载入输入数据
inputs.append(infer_input)
outputs = []
infer_output = httpclient.InferRequestedOutput('detction_result') # 构造输出
outputs.append(infer_output)
response = client.infer(
            'yolov5', inputs, model_version='1', outputs=outputs)  # 请求推理
response_outputs = response.as_numpy('detction_result') # 根据输出变量名获取结果
```

二. 使用requests访问服务

安装requests
```bash
pip install requests
```
1.获取yolov5的模型元数据

```python
import requests
url = 'http://localhost:8000/v2/models/yolov5/versions/1' # 根据上述章节中"模型的元信息"的获取接口构造url
response = requests.get(url)
response = response.json() # 返回数据为json，以json格式解析
```
打印一下返回的模型元数据
```python
print(response)
```
```text
{'name': 'yolov5', 'versions': ['1'], 'platform': 'ensemble', 'inputs': [{'name': 'INPUT', 'datatype': 'UINT8', 'shape': [-1, -1, -1, 3]}], 'outputs': [{'name': 'detction_result', 'datatype': 'BYTES', 'shape': [-1, -1]}]}
```
2.请求推理服务

根据模型的inputs和outputs构造数据，然后请求推理。
```python
url = 'http://localhost:8000/v2/models/yolov5/versions/1/infer' # 根据上述章节中"推理服务"的接口构造url
# 假设图像数据的文件名为000000014439.jpg
import cv2
image = cv2.imread('000000014439.jpg')
image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)[None]

payload = {
  "inputs" : [
    {
      "name" : "INPUT",
      "shape" : image.shape,
      "datatype" : "UINT8",
      "data" : image.tolist()
    }
  ],
  "outputs" : [
    {
      "name" : "detction_result"
    }
  ]
}
response = requests.post(url, data=json.dumps(payload))
response = response.json()  # 返回数据为json，以json格式解析后即为推理后返回的结果
```

### 使用GRPC客户端

安装tritonclient\[grpc\]
```bash
pip install tritonclient[grpc]
```
tritonclient\[grpc\]提供了使用GRPC的客户端，并且对GRPC的交互进行了封装，使得用户不用手动和服务端建立连接，也不用去直接使用grpc的stub去调用服务器的接口，而是封装后给用户提供了和tritonclient HTTP客户端一样的接口进行使用。

1.获取yolov5的模型元数据
```python
import tritonclient.grpc as grpcclient # 导入grpc客户端
server_addr = 'localhost:8001'  # 这里写fastdeployserver grpc服务器的实际地址
client = grpcclient.InferenceServerClient(server_addr)  # 创建client
model_metadata = client.get_model_metadata(
      model_name='yolov5', model_version='1') # 请求yolov5模型的元数据
```
2.请求推理服务
根据返回的model_metadata来构造请求数据。首先看一下模型的输入和输出有哪些
```python
print(model_metadata.inputs)
```
```text
[name: "INPUT"
datatype: "UINT8"
shape: -1
shape: -1
shape: -1
shape: 3
]
```

```python
print(model_metadata.outputs)
```

```text
[name: "detction_result"
datatype: "BYTES"
shape: -1
shape: -1
]
```

根据模型的inputs和outputs构造数据，然后请求推理
```python
# 假设图像数据的文件名为000000014439.jpg
import cv2
image = cv2.imread('000000014439.jpg')
image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)[None]

inputs = []
infer_input = grpcclient.InferInput('INPUT', image.shape, 'UINT8') # 构造输入
infer_input.set_data_from_numpy(image)  # 载入输入数据
inputs.append(infer_input)
outputs = []
infer_output = grpcclient.InferRequestedOutput('detction_result') # 构造输出
outputs.append(infer_output)
response = client.infer(
            'yolov5', inputs, model_version='1', outputs=outputs)  # 请求推理
response_outputs = response.as_numpy('detction_result') # 根据输出变量名获取结果
```

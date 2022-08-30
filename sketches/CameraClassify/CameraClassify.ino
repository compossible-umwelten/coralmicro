/*
 * Copyright 2022 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// [start-snippet:ardu-classification]
#include <coralmicro_SD.h>
#include <coralmicro_camera.h>

#include <cstdint>
#include <memory>

#include "Arduino.h"
#include "coral_micro.h"
#include "libs/tensorflow/classification.h"
#include "libs/rpc/rpc_http_server.h"

// This is the equivalent arduino sketch for examples/classify_camera. Upload
// this sketch and then trigger an inference by clicking the user button.
// On a Linux computer, you can also trigger the classification and get the
// result back in json format over USB:
//    python3 -m pip install -r examples/classify_camera/requirements.txt
//    python3 examples/classify_camera/classify_camera_client.py

namespace {
using namespace coralmicro;
using namespace coralmicro::arduino;

bool setup_success{false};
int buttonPin = PIN_BTN;

tflite::MicroMutableOpResolver<3> resolver;
const tflite::Model* model = nullptr;
std::vector<uint8_t> model_data;
std::shared_ptr<coralmicro::EdgeTpuContext> context = nullptr;
std::unique_ptr<tflite::MicroInterpreter> interpreter = nullptr;
TfLiteTensor* input_tensor = nullptr;
int model_height;
int model_width;
bool bayered;

const std::string kModelPath = "/models/mnv2_324_quant_bayered_edgetpu.tflite";
std::vector<tensorflow::Class> results;

constexpr int kTensorArenaSize = 8 * 1024 * 1024;
STATIC_TENSOR_ARENA_IN_SDRAM(tensor_arena, kTensorArenaSize);

FrameBuffer frame_buffer;

bool ClassifyFromCamera() {
  if (Camera.grab(frame_buffer) != CameraStatus::SUCCESS) {
    return false;
  }
  std::memcpy(tflite::GetTensorData<uint8_t>(input_tensor),
              frame_buffer.getBuffer(), frame_buffer.getBufferSize());
  if (interpreter->Invoke() != kTfLiteOk) {
    return false;
  }
  results = tensorflow::GetClassificationResults(interpreter.get(), 0.0f, 3);
  return true;
}

void ClassifyRpc(struct jsonrpc_request* r) {
  if (!setup_success) {
    jsonrpc_return_error(
        r, -1, "Inference failed because setup was not successful", nullptr);
    return;
  }
  if (!ClassifyFromCamera()) {
    jsonrpc_return_error(r, -1, "Failed to run classification from camera.",
                         nullptr);
    return;
  }
  if (!results.empty()) {
    const auto& result = results[0];
    jsonrpc_return_success(
        r, "{%Q: %d, %Q: %d, %Q: %V, %Q: %d, %Q: %d, %Q: %g}", "width",
        model_width, "height", model_height, "base64_data",
        frame_buffer.getBufferSize(), frame_buffer.getBuffer(), "bayered",
        bayered, "id", result.id, "score", result.score);
    return;
  }
  jsonrpc_return_success(r, "{%Q: %d, %Q: %d, %Q: %V, %Q: %d}", "width",
                         model_width, "height", model_height, "base64_data",
                         frame_buffer.getBufferSize(), frame_buffer.getBuffer(),
                         "bayered", bayered);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  // Turn on Status LED to show the board is on.
  pinMode(PIN_LED_STATUS, OUTPUT);
  digitalWrite(PIN_LED_STATUS, HIGH);
  Serial.println("Arduino Camera Classification!");

  pinMode(buttonPin, INPUT);

  SD.begin();
  Serial.println("Loading Model");

  if (!SD.exists(kModelPath.c_str())) {
    Serial.println("Model file not found");
    return;
  }

  SDFile model_file = SD.open(kModelPath.c_str());
  uint32_t model_size = model_file.size();
  model_data.resize(model_size);
  if (model_file.read(model_data.data(), model_size) != model_size) {
    Serial.print("Error loading model");
    return;
  }

  model = tflite::GetModel(model_data.data());
  context = coralmicro::EdgeTpuManager::GetSingleton()->OpenDevice();
  if (!context) {
    Serial.println("Failed to get EdgeTpuContext");
    return;
  }
  Serial.println("model and context created");

  tflite::MicroErrorReporter error_reporter;
  resolver.AddDequantize();
  resolver.AddDetectionPostprocess();
  resolver.AddCustom(coralmicro::kCustomOp, coralmicro::RegisterCustomOp());

  interpreter = std::make_unique<tflite::MicroInterpreter>(
      model, resolver, tensor_arena, kTensorArenaSize, &error_reporter);

  if (interpreter->AllocateTensors() != kTfLiteOk) {
    Serial.println("allocate tensors failed");
    return;
  }

  if (!interpreter) {
    Serial.println("Failed to make interpreter");
    return;
  }
  if (interpreter->inputs().size() != 1) {
    Serial.println("Bad inputs size");
    Serial.println(interpreter->inputs().size());
    return;
  }

  input_tensor = interpreter->input_tensor(0);
  model_height = input_tensor->dims->data[1];
  model_width = input_tensor->dims->data[2];
  if (Camera.begin(model_width, model_height, coralmicro::CameraFormat::kRaw,
                   coralmicro::CameraFilterMethod::kBilinear,
                   coralmicro::CameraRotation::k0,
                   true) != CameraStatus::SUCCESS) {
    Serial.println("Failed to start camera");
    return;
  }
  bayered = kModelPath.find("bayered") != std::string::npos;

  Serial.println("Initializing classification server...");
  jsonrpc_init(nullptr, nullptr);
  jsonrpc_export("classify_from_camera", ClassifyRpc);
  UseHttpServer(new JsonRpcHttpServer);
  Serial.println("Classification server ready!");

  setup_success = true;
  Serial.println("Initialized");
}

void loop() {
  pulseIn(buttonPin, HIGH);  // Hold until the user button is triggered.
  if (!setup_success) {
    Serial.println("Cannot run because of a problem during setup!");
    return;
  }

  if (!ClassifyFromCamera()) {
    Serial.println("Failed to run classification");
    return;
  }
  Serial.print("Results count: ");
  Serial.println(results.size());
  for (const auto& result : results) {
    Serial.print("Label ID: ");
    Serial.print(result.id);
    Serial.print(" Score: ");
    Serial.println(result.score);
  }
  delay(1000);
}
// [end-snippet:ardu-classification]

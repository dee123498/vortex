#pragma once

#include "quantized_model.h"
#include <Arduino.h>
#include <vector>

// Simple quantized inference: one hidden layer MLP with int8 weights and float scales.

static void matvec_int8(const int8_t *W, int rows, int cols, const int32_t *x_int32, int32_t *y_int32) {
  for (int r = 0; r < rows; ++r) {
    int32_t acc = 0;
    for (int c = 0; c < cols; ++c) {
      acc += ((const int8_t*)W)[r*cols + c] * x_int32[c];
    }
    y_int32[r] = acc;
  }
}

// Run inference from token ids -> output token id (simple)
int run_quantized_model(const std::vector<int> &token_ids) {
  // Create input vector (QM_INPUT_DIM) as small bag-of-ids modulo input dim
  int32_t input_int32[QM_INPUT_DIM] = {0};
  for (size_t i = 0; i < token_ids.size(); ++i) {
    int idx = token_ids[i] % QM_INPUT_DIM;
    input_int32[idx] += 1; // counts
  }

  int32_t hidden_int32[QM_HIDDEN_DIM];
  matvec_int8((const int8_t*)QM_W1, QM_HIDDEN_DIM, QM_INPUT_DIM, input_int32, hidden_int32);
  for (int i = 0; i < QM_HIDDEN_DIM; ++i) {
    int32_t b = (int32_t)QM_B1[i];
    float y = QM_SCALE_W1 * (float)hidden_int32[i] + QM_SCALE_B1 * (float)b;
    y = y > 0 ? y : 0;
    hidden_int32[i] = (int32_t)roundf(y / QM_SCALE_W2);
  }

  int32_t out_int32[QM_OUTPUT_DIM];
  matvec_int8((const int8_t*)QM_W2, QM_OUTPUT_DIM, QM_HIDDEN_DIM, hidden_int32, out_int32);
  int best = 0;
  float best_score = -1e30f;
  for (int i = 0; i < QM_OUTPUT_DIM; ++i) {
    float f = QM_SCALE_W2 * (float)out_int32[i] + QM_SCALE_B2 * (float)QM_B2[i];
    if (f > best_score) { best_score = f; best = i; }
  }

  return QM_OUTPUT_VOCAB[best];
}

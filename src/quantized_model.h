#pragma once

// Tiny placeholder quantized model (int8) — replace with converted model header

#define QM_INPUT_DIM 8
#define QM_HIDDEN_DIM 16
#define QM_OUTPUT_DIM 12

// Example quantized weights (small toy values)
static const int8_t QM_W1[QM_HIDDEN_DIM][QM_INPUT_DIM] = {
  {1,0,0,0,0,0,0,0},
  {0,1,0,0,0,0,0,0},
  {0,0,1,0,0,0,0,0},
  {0,0,0,1,0,0,0,0},
  {0,0,0,0,1,0,0,0},
  {0,0,0,0,0,1,0,0},
  {0,0,0,0,0,0,1,0},
  {0,0,0,0,0,0,0,1},
  {1,1,0,0,0,0,0,0},
  {0,1,1,0,0,0,0,0},
  {0,0,1,1,0,0,0,0},
  {0,0,0,1,1,0,0,0},
  {0,0,0,0,1,1,0,0},
  {0,0,0,0,0,1,1,0},
  {0,0,0,0,0,0,1,1},
  {1,0,0,0,0,0,0,1}
};

static const int8_t QM_B1[QM_HIDDEN_DIM] = {0};

static const int8_t QM_W2[QM_OUTPUT_DIM][QM_HIDDEN_DIM] = {
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0},
  {1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0},
  {0,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0},
  {1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0}
};

static const int8_t QM_B2[QM_OUTPUT_DIM] = {0};

// Per-tensor scales (example)
static const float QM_SCALE_W1 = 0.1f;
static const float QM_SCALE_B1 = 0.1f;
static const float QM_SCALE_W2 = 0.1f;
static const float QM_SCALE_B2 = 0.1f;

// Output token mapping (small)
static const int QM_OUTPUT_VOCAB[QM_OUTPUT_DIM] = {0,1,2,3,4,5,6,7,8,9,10,11};

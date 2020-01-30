#include "blackbox.h"

#include <string.h>

#include "rx_frsky.h"
#include "usb_configurator.h"

uint32_t blackbox_rate = 2;
uint8_t blackbox_enabled = 0;
blackbox_t state;

extern uint8_t usb_is_active;

extern float cpu_load;
extern float vbattfilt;

extern float rx[4];
extern float rx_filtered[4];
extern uint8_t aux[AUX_CHANNEL_MAX];

extern float gyro[3];
extern float gyro_raw[3];
extern float GEstG[3];

extern float accel[3];
extern float accel_filter[3];

#define CHECK_CBOR_ERROR(expr) \
  expr;                        \
  if (res < CBOR_OK) {         \
    return res;                \
  }

cbor_result_t cbor_encode_float_array(cbor_value_t *enc, const float *array, uint32_t size) {
  CHECK_CBOR_ERROR(cbor_result_t res = cbor_encode_array(enc, size))

  for (uint32_t i = 0; i < size; i++) {
    CHECK_CBOR_ERROR(res = cbor_encode_float(enc, &array[i]));
  }

  return res;
}

cbor_result_t cbor_encode_uint8_array(cbor_value_t *enc, const uint8_t *array, uint32_t size) {
  CHECK_CBOR_ERROR(cbor_result_t res = cbor_encode_array(enc, size));

  for (uint32_t i = 0; i < size; i++) {
    CHECK_CBOR_ERROR(res = cbor_encode_uint8(enc, &array[i]));
  }

  return res;
}

cbor_result_t cbor_encode_blackbox_t(cbor_value_t *enc, const blackbox_t *b) {
  CHECK_CBOR_ERROR(cbor_result_t res = cbor_encode_map_indefinite(enc));

  CHECK_CBOR_ERROR(res = cbor_encode_str(enc, "cpu_load"));
  CHECK_CBOR_ERROR(res = cbor_encode_float(enc, &b->cpu_load));

  CHECK_CBOR_ERROR(res = cbor_encode_str(enc, "vbat_filter"));
  CHECK_CBOR_ERROR(res = cbor_encode_float(enc, &b->vbat_filter));

  CHECK_CBOR_ERROR(res = cbor_encode_str(enc, "gyro_raw"));
  CHECK_CBOR_ERROR(res = cbor_encode_float_array(enc, b->gyro_raw, 3));
  CHECK_CBOR_ERROR(res = cbor_encode_str(enc, "gyro_filter"));
  CHECK_CBOR_ERROR(res = cbor_encode_float_array(enc, b->gyro_filter, 3));
  CHECK_CBOR_ERROR(res = cbor_encode_str(enc, "gyro_vector"));
  CHECK_CBOR_ERROR(res = cbor_encode_float_array(enc, b->gyro_vector, 3));

  CHECK_CBOR_ERROR(res = cbor_encode_str(enc, "rx_raw"));
  CHECK_CBOR_ERROR(res = cbor_encode_float_array(enc, b->rx_raw, 4));
  CHECK_CBOR_ERROR(res = cbor_encode_str(enc, "rx_filter"));
  CHECK_CBOR_ERROR(res = cbor_encode_float_array(enc, b->rx_filter, 4));
  CHECK_CBOR_ERROR(res = cbor_encode_str(enc, "rx_aux"));
  CHECK_CBOR_ERROR(res = cbor_encode_uint8_array(enc, b->rx_aux, AUX_CHANNEL_MAX));

  CHECK_CBOR_ERROR(res = cbor_encode_str(enc, "accel_raw"));
  CHECK_CBOR_ERROR(res = cbor_encode_float_array(enc, b->accel_raw, 3));
  CHECK_CBOR_ERROR(res = cbor_encode_str(enc, "accel_filter"));
  CHECK_CBOR_ERROR(res = cbor_encode_float_array(enc, b->accel_filter, 3));

  CHECK_CBOR_ERROR(res = cbor_encode_end_indefinite(enc));

  return res;
}

void blackbox_update() {
  static uint32_t loop_counter = 0;

  if (blackbox_enabled == 0)
    return;

  state.cpu_load = cpu_load;

  state.vbat_filter = vbattfilt;

  state.rx_raw[0] = rx[0];
  state.rx_raw[1] = rx[1];
  state.rx_raw[2] = rx[2];
  state.rx_raw[3] = rx[3];

  state.rx_filter[0] = rx_filtered[0];
  state.rx_filter[1] = rx_filtered[1];
  state.rx_filter[2] = rx_filtered[2];
  state.rx_filter[3] = rx_filtered[3];

  for (uint32_t i = 0; i < AUX_CHANNEL_MAX; i++) {
    state.rx_aux[i] = aux[i];
  }

  state.gyro_raw[0] = gyro_raw[0];
  state.gyro_raw[1] = gyro_raw[1];
  state.gyro_raw[2] = gyro_raw[2];

  state.gyro_filter[0] = gyro[0];
  state.gyro_filter[1] = gyro[1];
  state.gyro_filter[2] = gyro[2];

  state.gyro_vector[0] = GEstG[0];
  state.gyro_vector[1] = GEstG[1];
  state.gyro_vector[2] = GEstG[2];

  state.accel_raw[0] = accel[0];
  state.accel_raw[1] = accel[1];
  state.accel_raw[2] = accel[2];

  state.accel_filter[0] = accel_filter[0];
  state.accel_filter[1] = accel_filter[1];
  state.accel_filter[2] = accel_filter[2];

  if (usb_is_active != 0 && (loop_counter % (uint32_t)((1000000.0f / (float)blackbox_rate) / LOOPTIME)) == 0) {
    quic_blackbox(&state);
  }

  if ((loop_counter % (uint32_t)((1000000.0f / LOOPTIME)) == 0)) {
    smart_port_payload_t payload = {
        .frame_id = FSSP_MSPS_FRAME,
        .value_id = 0x5000,
        .value = 0x1337,
    };

    frsky_d16_write_telemetry(&payload);
  }

  loop_counter++;
}
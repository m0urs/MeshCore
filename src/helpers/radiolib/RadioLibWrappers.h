#pragma once

#include <Mesh.h>
#include <RadioLib.h>
// PowerSaving variant
#include "RXPowerSaving.h"

#ifdef USE_CC310_HW_CRYPTO
#include <Adafruit_nRFCrypto.h>
#endif
struct PacketMillis {
  uint32_t preambleMillis;  // preamble-detect -> header-valid deadline
  uint32_t payloadMillis;   // header-valid   -> rx-done deadline
};

// PowerSaving variant
class RadioLibWrapper : public mesh::Radio, public RxPowerSavingControl {
protected:
  PhysicalLayer* _radio;
  mesh::MainBoard* _board;
  uint32_t n_recv, n_sent, n_recv_errors;
  // mups: n_recv_errors split by cause (n_recv_err_crc + n_recv_err_other == n_recv_errors)
  uint32_t n_recv_err_crc = 0;        // readData() returned RADIOLIB_ERR_CRC_MISMATCH
  // mups: n_recv_err_crc split by what the chip flagged (pay + hdr + unk == n_recv_err_crc)
  uint32_t n_recv_err_pay = 0;        //   payload CRC error (header was valid)
  uint32_t n_recv_err_hdr = 0;        //   LoRa header error (no valid header)
  uint32_t n_recv_err_unk = 0;        //   chip driver cannot tell / neither flag set
  uint32_t n_recv_err_other = 0;      // any other readData() error (SPI errors, ...)
  int16_t _last_recv_err_other = 0;   // RadioLib code of the latest "other" error, 0 = none yet
  // mups: histograms of received frames: [0 = SNR, 1 = RSSI, 2 = declared packet length]
  //       [0 = received correctly, 1 = payload CRC error, 2 = header error][5 buckets, see rxHistAdd()]
  uint32_t _rx_hist[3][3][5] = {};
  void rxHistAdd(uint8_t cls, int pkt_len);
  // mups: tells why the chip flagged a CRC_MISMATCH frame, from the IRQ flags read BEFORE readData()
  // (readData() clears them). Returns 1 = payload CRC error, 2 = header error, 0 = cannot tell.
  uint8_t classifyCrcError(uint32_t irq);
  // mups: ring buffer of the latest CRC-error frames, shown by "get rx.errors last"
  struct RxErrRec {
    uint32_t t_ms;     // millis() when the frame was received
    int16_t rssi;      // dBm, rounded
    int8_t snr4;       // SNR in quarter dB (the chip reports 0.25 dB steps)
    uint8_t cls;       // 1 = payload CRC error, 2 = header error, 0 = chip cannot tell
    uint16_t len;      // declared packet length
    uint8_t b[4];      // first bytes of the (corrupted) frame, 0 if the frame is shorter
  };
  static const int RX_ERR_LOG_SIZE = 6;
  RxErrRec _rx_err_log[RX_ERR_LOG_SIZE] = {};
  uint8_t _rx_err_log_next = 0;   // slot the next record goes to
  uint8_t _rx_err_log_n = 0;      // number of valid records (max RX_ERR_LOG_SIZE)
  void rxErrLogAdd(uint8_t cls, int pkt_len, const uint8_t* bytes);
  // PowerSaving variant
  uint32_t _cad_busy_count = 0;
  int16_t _noise_floor, _threshold;
  // PowerSaving variant
  float _last_rssi, _last_snr;
  bool _last_metrics_valid;
  bool _cad_enabled;
  uint16_t _num_floor_samples;
  int32_t _floor_sample_sum;
  uint8_t _preamble_sf;

  // PowerSaving variant
  bool _rx_ps_restart_pending = false;
  bool _rx_ps_config_error = false;
  bool _rx_ps_enabled;
  bool _rx_ps_armed;
  uint32_t _rx_ps_rx_us;
  uint32_t _rx_ps_sleep_us;
  uint32_t _rx_ps_eff_rx_us;      // periods the driver actually armed with
  uint32_t _rx_ps_eff_sleep_us;
  int16_t _rx_ps_last_error;
  uint32_t n_rx_ps_arm_failures;
  RxPowerSavingArmRetryState _rx_ps_arm_retry;

  bool _nf_calib_active;
  unsigned long _nf_last_calib;
  unsigned long _nf_calib_deadline;
  unsigned long _nf_sample_from;

  void idle();
  void startRecv();

  // PowerSaving variant
  void requestRestartRecv();
  bool isPacketPendingOrReceiving();
  void prepareForRadioConfig();
  void sampleNoiseFloorOnce();
  bool publishNoiseFloor();
  void noiseFloorCalibCheck();
  void endNoiseFloorCalib(unsigned long now);
  int16_t startReceiveMode();
  void stopReceiveDutyCycle();
  // eff_rx_us/eff_sleep_us report the periods the driver really programmed,
  // which may be clamped to satisfy a hardware constraint.
  virtual int16_t armDutyCycle(RadioLibIrqFlags_t irq_flags,
                               RadioLibIrqFlags_t irq_mask,
                               uint32_t* eff_rx_us, uint32_t* eff_sleep_us) {
    (void)irq_flags;
    (void)irq_mask;
    (void)eff_rx_us;
    (void)eff_sleep_us;
    return RADIOLIB_ERR_UNSUPPORTED;
  }
  virtual int16_t stopDutyCycleHardware() { return _radio->standby(); }
  virtual int16_t startContinuousReceive() { return _radio->startReceive(); }
  virtual bool isPacketReady();
  virtual bool isChipBusy() { return false; }

  float packetScoreInt(float snr, int sf, int packet_len);
  virtual bool isReceivingPacket() =0;
  virtual void doResetAGC();

public:
  // PowerSaving variant
  RadioLibWrapper(PhysicalLayer& radio, mesh::MainBoard& board)
      : _radio(&radio), _board(&board), _last_rssi(0), _last_snr(0), _last_metrics_valid(false),
        _preamble_sf(0), _rx_ps_enabled(false), _rx_ps_armed(false),
        _rx_ps_rx_us(RX_POWERSAVING_DEFAULT_RX_US),
        _rx_ps_sleep_us(RX_POWERSAVING_DEFAULT_SLEEP_US), _rx_ps_eff_rx_us(0),
        _rx_ps_eff_sleep_us(0), _rx_ps_last_error(RADIOLIB_ERR_NONE),
        n_rx_ps_arm_failures(0), _nf_calib_active(false),
        _nf_last_calib(0), _nf_calib_deadline(0), _nf_sample_from(0) {
    n_recv = n_sent = n_recv_errors = 0;
  }

  void begin() override;
  virtual void powerOff();
  int recvRaw(uint8_t* bytes, int sz) override;
  uint32_t getEstAirtimeFor(int len_bytes) override;
  bool startSendRaw(const uint8_t* bytes, int len) override;
  bool isSendComplete() override;
  void onSendFinished() override;
  bool isInRecvMode() const override;
  bool isChannelActive();

  // PowerSaving variant
  virtual bool supportsRxPowerSaving() const { return false; }
  void setRxPowerSavingConfigError(bool invalid) override { _rx_ps_config_error = invalid; }
  bool setRxPowerSaving(bool enabled, uint32_t rx_us, uint32_t sleep_us) override;
  RxPowerSavingStatus getRxPowerSavingStatus() const override;
  bool isRxPowerSavingCalibrationActive() const override { return _nf_calib_active; }

  bool hasPendingReceive() { return isPacketPendingOrReceiving(); }

  bool isReceiving() override {
    if (isReceivingPacket()) return true;

    return isChannelActive();
  }

  virtual void setParams(float freq, float bw, uint8_t sf, uint8_t cr) = 0;
  uint32_t getRngSeed();
  void setTxPower(int8_t dbm);

  virtual float getCurrentRSSI() =0;
  virtual uint8_t getSpreadingFactor() const { return LORA_SF; }
  static uint16_t preambleLengthForSF(uint8_t sf) { return sf <= 8 ? 32 : 16; }
  void updatePreamble(uint8_t sf) { _preamble_sf = sf; _radio->setPreambleLength(preambleLengthForSF(sf)); }
  PacketMillis calcMaxPacketMillis(uint8_t sf, float bw, uint8_t cr, uint8_t preambleSymbols);
  virtual int16_t performChannelScan();

  int getNoiseFloor() const override { return _noise_floor; }
  void triggerNoiseFloorCalibrate(int threshold) override;
  void setCADEnabled(bool enable) override { _cad_enabled = enable; }
  void resetAGC() override;

  void loop() override;

  // PowerSaving variant
  uint32_t getCADBusyCount() const { return _cad_busy_count; }

  uint32_t getPacketsRecv() const { return n_recv; }
  uint32_t getPacketsRecvErrors() const { return n_recv_errors; }
  // mups: see n_recv_err_crc / n_recv_err_other
  uint32_t getPacketsRecvErrCrc() const { return n_recv_err_crc; }
  uint32_t getPacketsRecvErrOther() const { return n_recv_err_other; }
  uint32_t getPacketsRecvErrPay() const { return n_recv_err_pay; }
  uint32_t getPacketsRecvErrHdr() const { return n_recv_err_hdr; }
  uint32_t getPacketsRecvErrUnk() const { return n_recv_err_unk; }
  int16_t getLastRecvErrOther() const { return _last_recv_err_other; }
  // mups: kind 0 = SNR, 1 = RSSI, 2 = length; cls 0 = ok, 1 = payload CRC error, 2 = header error; bucket 0..4
  uint32_t getRxHist(uint8_t kind, uint8_t cls, uint8_t bucket) const {
    return (kind < 3 && cls < 3 && bucket < 5) ? _rx_hist[kind][cls][bucket] : 0;
  }
  // mups: idx 0 = newest CRC-error frame; returns false if there is no such record
  bool getRxErrLast(uint8_t idx, uint32_t* age_s, int* rssi, int* snr10, int* len, uint8_t* cls, uint8_t* b4) const;
  uint32_t getPacketsSent() const { return n_sent; }

  // PowerSaving variant
  void resetStats() {
    n_recv = n_sent = n_recv_errors = n_rx_ps_arm_failures = 0;
    n_recv_err_crc = n_recv_err_other = 0;   // mups
    n_recv_err_pay = n_recv_err_hdr = n_recv_err_unk = 0;   // mups
    _rx_err_log_next = _rx_err_log_n = 0;                   // mups
    _last_recv_err_other = 0;                // mups
    for (int k = 0; k < 3; k++) for (int c = 0; c < 3; c++) for (int b = 0; b < 5; b++) _rx_hist[k][c][b] = 0;   // mups
    _cad_busy_count = 0;
    // Clearing diagnostics also grants a fresh set of arm attempts. If they
    // fail again, the normal three-failure continuous-RX fallback applies.
    _rx_ps_arm_retry.reset();
  }

  virtual float getLastRSSI() const override;
  virtual float getLastSNR() const override;

  float packetScore(float snr, int packet_len) override { return packetScoreInt(snr, 10, packet_len); }  // assume sf=10

  virtual bool setRxBoostedGainMode(bool) { return false; }
  virtual bool getRxBoostedGainMode() const { return false; }
  
  virtual bool configSideDetectors(const uint8_t sideDetSFs[], uint8_t num, float bw) { return false; }
};

/**
 * \brief  an RNG impl using the noise from the LoRa radio as entropy.
 *         NOTE: this is VERY SLOW!  Use only for things like creating new LocalIdentity
*/
class RadioNoiseListener : public mesh::RNG {
  PhysicalLayer* _radio;
public:
  RadioNoiseListener(PhysicalLayer& radio): _radio(&radio) { }

  void random(uint8_t* dest, size_t sz) override {
#ifdef USE_CC310_HW_CRYPTO
    nRFCrypto.Random.generate(dest, (uint16_t)sz);
    for (int i = 0; i < sz; i++) {
      dest[i] ^= _radio->randomByte() ^ (::random(0, 256) & 0xFF); // combine with Radio's entropy
    }
#else
    for (int i = 0; i < sz; i++) {
      dest[i] = _radio->randomByte() ^ (::random(0, 256) & 0xFF);
    }
#endif
  }
};

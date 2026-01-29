#include "music_leds.h"
#include "audio_source.h"

#include "esphome/components/fastled_helper/utils.h"
#include "esphome/components/light/addressable_light_effect.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#define FASTLED_INTERNAL  // remove annoying pragma messages
#include <FastLED.h>

#include <arduinoFFT.h>

#if BITS_PER_SAMPLE == 16
#define I2S_datatype int16_t
#define I2S_unsigned_datatype uint16_t
#undef I2S_SAMPLE_DOWNSCALE_TO_16BIT
#else
#define I2S_datatype int32_t
#define I2S_unsigned_datatype uint32_t
#define I2S_SAMPLE_DOWNSCALE_TO_16BIT
#endif

#define NUM_GEQ_CHANNELS 16  // number of frequency channels. Don't change !!
#define SAMPLES_FFT 512      // Samples in an FFT batch - This value MUST ALWAYS be a power of 2

namespace esphome {
namespace music_leds {

static const uint32_t BUFFER_SIZE = sizeof(I2S_datatype) * SAMPLES_FFT;

enum EventGroupBits : uint32_t {
  COMMAND_STOP = (1 << 0),  // Signals the FFT task should stop

  TASK_STARTING = (1 << 3),
  TASK_RUNNING = (1 << 4),
  TASK_STOPPING = (1 << 5),
  TASK_STOPPED = (1 << 6),
#ifdef DEBUG
  TASK_INFO = (1 << 7),
#endif

  ERROR_MEMORY = (1 << 9),
  ERROR_FFT = (1 << 10),

  WARNING_FULL_RING_BUFFER = (1 << 13),

  ERROR_BITS = ERROR_MEMORY | ERROR_FFT,
  ALL_BITS = 0xfffff,  // 24 total bits available in an event group
};

static const LogString *music_leds_state_to_string(State state) {
  switch (state) {
    case State::STARTING:
      return LOG_STR("STARTING");
    case State::STOPPING:
      return LOG_STR("STOPPING");
    case State::STOPPED:
      return LOG_STR("STOPPED");
    case State::RUNNING:
      return LOG_STR("RUNNING");
    default:
      return LOG_STR("UNKNOWN");
  }
}

void MusicLeds::setup() {
  this->event_group_ = xEventGroupCreate();
  if (this->event_group_ == nullptr) {
    ESP_LOGE(TAG, "Failed to create event group");
    this->mark_failed();
    return;
  }

#ifdef USE_OTA_STATE_LISTENER
  ota::get_global_ota_callback()->add_global_state_listener(this);
#endif

  ESP_LOGCONFIG(TAG, "Music Leds initialized");
  this->start();
}

#ifdef USE_OTA_STATE_LISTENER
void MusicLeds::on_ota_global_state(ota::OTAState state, float progress, uint8_t error, ota::OTAComponent *comp) {
  if (state == ota::OTA_STARTED) {
    this->on_shutdown();
  }
}
#endif

void MusicLeds::loop() {
  uint32_t event_group_bits = xEventGroupGetBits(this->event_group_);

  if (event_group_bits & EventGroupBits::ERROR_MEMORY) {
    xEventGroupClearBits(this->event_group_, EventGroupBits::ERROR_MEMORY);
    ESP_LOGE(TAG, "Encountered an error allocating buffers");
  }

  if (event_group_bits & EventGroupBits::ERROR_FFT) {
    xEventGroupClearBits(this->event_group_, EventGroupBits::ERROR_FFT);
    ESP_LOGE(TAG, "Encountered an error while performing an FFT");
  }

  if (event_group_bits & EventGroupBits::WARNING_FULL_RING_BUFFER) {
    xEventGroupClearBits(this->event_group_, EventGroupBits::WARNING_FULL_RING_BUFFER);
    ESP_LOGW(TAG, "Not enough free bytes in ring buffer to store incoming audio data. Resetting the ring buffer.");
  }

  if (event_group_bits & EventGroupBits::TASK_STARTING) {
    ESP_LOGD(TAG, "FFT task has started, attempting to allocate memory for buffers");
    xEventGroupClearBits(this->event_group_, EventGroupBits::TASK_STARTING);
  }

  if (event_group_bits & EventGroupBits::TASK_RUNNING) {
    ESP_LOGD(TAG, "FFT task is running");
    xEventGroupClearBits(this->event_group_, EventGroupBits::TASK_RUNNING);
    this->set_state_(State::RUNNING);
  }

  if (event_group_bits & EventGroupBits::TASK_STOPPING) {
    ESP_LOGD(TAG, "FFT task is stopping, deallocating buffers");
    xEventGroupClearBits(this->event_group_, EventGroupBits::TASK_STOPPING);
  }

  if ((event_group_bits & EventGroupBits::TASK_STOPPED)) {
    ESP_LOGD(TAG, "FFT task is finished, freeing task resources");
    this->on_stop();
    xEventGroupClearBits(this->event_group_, ALL_BITS);
    this->set_state_(State::STOPPED);
  }

  switch (this->state_) {
    case State::STARTING:
      this->on_start();
      break;
    case State::RUNNING:
      this->on_loop();
      break;
    case State::STOPPING:
      xEventGroupSetBits(this->event_group_, EventGroupBits::COMMAND_STOP);
      break;
    case State::STOPPED:
      break;
  }
}

void MusicLeds::dump_config() {
  ESP_LOGCONFIG(TAG, "Music Leds version: %s", MUSIC_LEDS_VERSION);
  if (this->is_failed()) {
    ESP_LOGCONFIG(TAG, "Music Leds failed!");
  }
  ESP_LOGCONFIG(TAG, "         Task Core: %u", FFTTASK_CORE);
  ESP_LOGCONFIG(TAG, "     Task Priority: %u", FFTTASK_PRIORITY);
  ESP_LOGCONFIG(TAG, "           Samples: %dbit", BITS_PER_SAMPLE);
  ESP_LOGCONFIG(TAG, "       Sample rate: %d", SAMPLE_RATE);
  ESP_LOGCONFIG(TAG, "       Sample gain: %u", SR_GAIN);
  ESP_LOGCONFIG(TAG, "     Squelch value: %u", SR_SQUELCH);
  ESP_LOGCONFIG(TAG, " FFTResult scaling: %u", FFT_SCALING);
  ESP_LOGCONFIG(TAG, "      Gain control: %u", GAIN_CONTROL);
#ifdef USE_BANDPASSFILTER
  ESP_LOGCONFIG(TAG, "  Band Pass Filter: Yes");
#endif
#ifdef USE_SOUND_DYNAMICS_LIMITER
  ESP_LOGCONFIG(TAG, "  Dynamics Limiter: Yes");
#else
  ESP_LOGCONFIG(TAG, "  Dynamics Limiter: No");
#endif
}  // dump_config()

void MusicLeds::on_shutdown() { this->stop(); }

void MusicLeds::start() {
  if (this->state_ != State::STOPPED)
    return;

  ESP_LOGD(TAG, "Starting MusicLeds");
  this->state_ = State::STARTING;
}

void MusicLeds::stop() {
  if (this->state_ == State::STOPPED)
    return;

  ESP_LOGD(TAG, "Stopping MusicLeds...");
  this->set_state_(State::STOPPING);
}

void MusicLeds::set_state_(State state) {
  if (this->state_ != state) {
    ESP_LOGD(TAG, "State changed from %s to %s", LOG_STR_ARG(music_leds_state_to_string(this->state_)),
             LOG_STR_ARG(music_leds_state_to_string(state)));
    this->state_ = state;
  }
}

// *****************************************************************************
// Audioreactive variables
// *****************************************************************************
static float micDataReal = 0.0f;      // MicIn data with full 24bit resolution - lowest 8bit after decimal point
static float multAgc = 1.0f;          // sample * multAgc = sampleAgc. Our AGC multiplier
static float sampleAvg = 0.0f;        // Smoothed Average sample - sampleAvg < 1 means "quiet" (simple noise gate)
static float sampleAgc = 0.0f;        // Smoothed AGC sample
static float FFT_MajorPeak = 1.0f;    // FFT: strongest (peak) frequency
static float FFT_Magnitude = 0.0f;    // FFT: volume (magnitude) of peak frequency
static bool samplePeak = false;       // Boolean flag for peak - used in effects.
                                      // Responding routine may reset this flag. Auto-reset after 50ms
static unsigned long timeOfPeak = 0;  // time of last sample peak detection
static uint8_t fftResult[NUM_GEQ_CHANNELS] = {0};  // Our calculated freq. channel result table to be used by effects

// Peak detection
static void detectSamplePeak(void);  // peak detection function (needs scaled FFT results in vReal[])
static void autoResetPeak(void);     // peak auto-reset function
static uint8_t maxVol = 31;          // (was 10) Reasonable value for constant volume for 'peak detector',
                                     // as it won't always trigger  (deprecated)
static uint8_t binNum = 8;           // Used to select the bin for FFT based beat detection  (deprecated)

// clang-format off
// AGC presets
#define AGC_NUM_PRESETS 3 // AGC presets:          normal,   vivid,    lazy
const double agcSampleDecay[AGC_NUM_PRESETS]  = { 0.9994f, 0.9985f, 0.9997f}; // decay factor for sampleMax, in case the current sample is below sampleMax
const float agcZoneLow[AGC_NUM_PRESETS]       = {      32,      28,      36}; // low volume emergency zone
const float agcZoneHigh[AGC_NUM_PRESETS]      = {     240,     240,     248}; // high volume emergency zone
const float agcZoneStop[AGC_NUM_PRESETS]      = {     336,     448,     304}; // disable AGC integrator if we get above this level
const float agcTarget0[AGC_NUM_PRESETS]       = {     112,     144,     164}; // first AGC setPoint -> between 40% and 65%
const float agcTarget0Up[AGC_NUM_PRESETS]     = {      88,      64,     116}; // setpoint switching value (a poor man's bang-bang)
const float agcTarget1[AGC_NUM_PRESETS]       = {     220,     224,     216}; // second AGC setPoint -> around 85%
const double agcFollowFast[AGC_NUM_PRESETS]   = { 1/192.f, 1/128.f, 1/256.f}; // quickly follow setpoint - ~0.15 sec
const double agcFollowSlow[AGC_NUM_PRESETS]   = {1/6144.f,1/4096.f,1/8192.f}; // slowly follow setpoint  - ~2-15 secs
const double agcControlKp[AGC_NUM_PRESETS]    = {    0.6f,    1.5f,   0.65f}; // AGC - PI control, proportional gain parameter
const double agcControlKi[AGC_NUM_PRESETS]    = {    1.7f,   1.85f,    1.2f}; // AGC - PI control, integral gain parameter
const float agcSampleSmooth[AGC_NUM_PRESETS]  = {  1/12.f,   1/6.f,  1/16.f}; // smoothing factor for sampleAgc (use rawSampleAgc if you want the non-smoothed value)
// AGC presets end
// clang-format on

// Globals
static uint8_t inputLevel = 128;    // UI slider value
uint8_t soundSquelch = SR_SQUELCH;  // squelch value for volume reactive routines (config value)
uint8_t sampleGain = SR_GAIN;       // sample gain (config value)

// User settable options
static uint8_t FFTScalingMode = FFT_SCALING;  // FFTResult scaling: 0 none;
                                              // 1 optimized logarithmic;
                                              // 2 optimized linear;
                                              // 3 optimized square root
static uint8_t soundAgc = GAIN_CONTROL;       // Automagic gain control: 0 - none,
                                              // 1 - normal,
                                              // 2 - vivid,
                                              // 3 - lazy (config value)

// User settable parameters for limitSoundDynamics()
#ifdef USE_SOUND_DYNAMICS_LIMITER
static uint16_t attackTime = 80;   // int: attack time in milliseconds. Default 0.08sec
static uint16_t decayTime = 1400;  // int: decay time in milliseconds.  Default 1.40sec
#endif

// *****************************************************************************
// Begin FFT Code
// *****************************************************************************

// some prototypes, to ensure consistent interfaces
static float fftAddAvg(int from, int to);  // average of several FFT result bins
#ifdef USE_BANDPASSFILTER
static void runMicFilter(uint16_t numSamples, float *sampleBuffer);  // pre-filtering of raw samples (band-pass)
#endif
static void postProcessFFTResults(bool noiseGateOpen,
                                  int numberOfChannels);  // post-processing and post-amp of GEQ channels

// Table of multiplication factors so that we can even out the frequency response.
static float fftResultPink[NUM_GEQ_CHANNELS] = {1.70f, 1.71f, 1.73f, 1.78f, 1.68f, 1.56f, 1.55f, 1.63f,
                                                1.79f, 1.62f, 1.80f, 2.06f, 2.47f, 3.35f, 6.83f, 9.55f};

// FFT Task variables (filtering and post-processing)
static float fftCalc[NUM_GEQ_CHANNELS] = {
    0.0f};  // Try and normalize fftBin values to a max of 4096, so that 4096/16 = 256.
#ifdef USE_SOUND_DYNAMICS_LIMITER
static float fftAvg[NUM_GEQ_CHANNELS] = {
    0.0f};  // Calculated frequency channel results, with smoothing (used if dynamics limiter is ON)
#endif

// audio source parameters and constant
// constexpr SRate_t SAMPLE_RATE = 22050;  // Base sample rate in Hz - 22Khz is a standard rate. Physical sample time ->
// 23ms constexpr SRate_t SAMPLE_RATE = 16000;  // 16kHz - use if FFTtask takes more than 20ms.       Physical sample
// time -> 32ms constexpr SRate_t SAMPLE_RATE = 20480;  // Base sample rate in Hz - 20Khz is experimental.    Physical
// sample time -> 25ms constexpr SRate_t SAMPLE_RATE = 10240;  // Base sample rate in Hz - previous default. Physical
// sample time -> 50ms

// #define FFT_MIN_CYCLE 21                // minimum time before FFT task is repeated. Use with 22Khz sampling
// #define FFT_MIN_CYCLE 30                // Use with 16Khz sampling
// #define FFT_MIN_CYCLE 23                // minimum time before FFT task is repeated. Use with 20Khz sampling
// #define FFT_MIN_CYCLE 46                // minimum time before FFT task is repeated. Use with 10Khz sampling

// FFT Constants
// the following are observed values, supported by a bit of "educated guessing"
// #define FFT_DOWNSCALE 0.65f             // 20kHz - downscaling factor for FFT results - "Flat-Top" window @20Khz, old
// freq channels
#define FFT_DOWNSCALE 0.46f  // downscaling factor for FFT results - for "Flat-Top" window @22Khz, new freq channels
#define LOG_256 5.54517744f  // log(256)

// These are the input and output vectors.  Input vectors receive computed results from FFT.
static float *vReal = nullptr;  // FFT sample inputs / freq output -  these are our raw result bins
static float *vImag = nullptr;  // imaginary parts

// these options actually cause slow-downs on all esp32 processors, don't use them.
// #define FFT_SPEED_OVER_PRECISION        // enables use of reciprocals (1/x etc) - not faster on ESP32
// #define FFT_SQRT_APPROXIMATION          // enables "quake3" style inverse sqrt  - slower on ESP32

///////////////////////////
// Helper functions      //
///////////////////////////

// Compute average of several FFT result bins
static float fftAddAvg(int from, int to) {
  float result = 0.0f;
  for (int i = from; i <= to; i++) {
    result += vReal[i];
  }
  return result / float(to - from + 1);
}

///////////////////////////
// Pre / Postprocessing  //
///////////////////////////

#ifdef USE_BANDPASSFILTER
static void runMicFilter(uint16_t numSamples, float *sampleBuffer)  // pre-filtering of raw samples (band-pass)
{
  // clang-format off
  // low frequency cutoff parameter - see https://dsp.stackexchange.com/questions/40462/exponential-moving-average-cut-off-frequency
  // constexpr float alpha = 0.04f;     // 150Hz
  // constexpr float alpha = 0.03f;     // 110Hz
  constexpr float alpha = 0.0225f;      // 80hz
  // constexpr float alpha = 0.01693f;  // 60hz
  // high frequency cutoff  parameter
  // constexpr float beta1 = 0.75f;     // 11Khz
  // constexpr float beta1 = 0.82f;     // 15Khz
  // constexpr float beta1 = 0.8285f;   // 18Khz
  constexpr float beta1 = 0.85f;        // 20Khz

  constexpr float beta2 = (1.0f - beta1) / 2.0f;
  static float last_vals[2] = {0.0f};   // FIR high freq cutoff filter
  static float lowfilt = 0.0f;          // IIR low frequency cutoff filter
  // clang-format on

  for (int i = 0; i < numSamples; i++) {
    // FIR lowpass, to remove high frequency noise
    float highFilteredSample;
    if (i < (numSamples - 1)) {
      // smooth out spikes
      highFilteredSample = beta1 * sampleBuffer[i] + beta2 * last_vals[0] + beta2 * sampleBuffer[i + 1];
    } else {
      // special handling for last sample in array
      highFilteredSample = beta1 * sampleBuffer[i] + beta2 * last_vals[0] + beta2 * last_vals[1];
    }
    last_vals[1] = last_vals[0];
    last_vals[0] = sampleBuffer[i];
    sampleBuffer[i] = highFilteredSample;
    // IIR highpass, to remove low frequency noise
    lowfilt += alpha * (sampleBuffer[i] - lowfilt);
    sampleBuffer[i] = sampleBuffer[i] - lowfilt;
  }
}
#endif

// post-processing and post-amp of GEQ channels
static void postProcessFFTResults(bool noiseGateOpen, int numberOfChannels) {
  for (int i = 0; i < numberOfChannels; i++) {
    if (noiseGateOpen) {  // noise gate open
      // Adjustment for frequency curves.
      fftCalc[i] *= fftResultPink[i];
      // Adjustment related to FFT windowing function
      if (FFTScalingMode > 0)
        fftCalc[i] *= FFT_DOWNSCALE;
      // Manual linear adjustment of gain using sampleGain adjustment for different input types.
      // Apply gain, with inputLevel adjustment
      fftCalc[i] *= soundAgc ? multAgc : ((float) sampleGain / 40.0f * (float) inputLevel / 128.0f + 1.0f / 16.0f);
      if (fftCalc[i] < 0)
        fftCalc[i] = 0;
    }
    // Constrain internal vars - just to be sure
    fftCalc[i] = constrain(fftCalc[i], 0.0f, 1023.0f);

#ifdef USE_SOUND_DYNAMICS_LIMITER
    // Smooth results - rise fast, fall slower
    if (fftCalc[i] > fftAvg[i]) {  // rise fast
      fftAvg[i] =
          fftCalc[i] * 0.75f + 0.25f * fftAvg[i];  // will need approx 2 cycles (50ms) for converging against fftCalc[i]
    } else {                                       // fall slow
      if (decayTime < 1000)
        fftAvg[i] = fftCalc[i] * 0.22f + 0.78f * fftAvg[i];  // approx  5 cycles (225ms) for falling to zero
      else if (decayTime < 2000)
        fftAvg[i] = fftCalc[i] * 0.17f + 0.83f * fftAvg[i];  // default - approx  9 cycles (225ms) for falling to zero
      else if (decayTime < 3000)
        fftAvg[i] = fftCalc[i] * 0.14f + 0.86f * fftAvg[i];  // approx 14 cycles (350ms) for falling to zero
      else
        fftAvg[i] = fftCalc[i] * 0.1f + 0.9f * fftAvg[i];  // approx 20 cycles (500ms) for falling to zero
    }
    // Constrain internal vars - just to be sure
    fftAvg[i] = constrain(fftAvg[i], 0.0f, 1023.0f);
#endif

    float currentResult;
#ifdef USE_SOUND_DYNAMICS_LIMITER
    currentResult = fftAvg[i];
#else
    currentResult = fftCalc[i];
#endif

    switch (FFTScalingMode) {
      case 1:
        // Logarithmic scaling
        currentResult *= 0.42f;  // 42 is the answer ;-)
        currentResult -= 8.0f;   // this skips the lowest row, giving some room for peaks
        if (currentResult > 1.0f)
          currentResult = logf(currentResult);  // log to base "e", which is the fastest log() function
        else
          currentResult = 0.0f;                       // special handling, because log(1) = 0; log(0) = undefined
        currentResult *= 0.85f + (float(i) / 18.0f);  // extra up-scaling for high frequencies
        currentResult = remap(currentResult, 0.0f, LOG_256, 0.0f, 255.0f);  // map [log(1) ... log(255)] to [0 ... 255]
        break;
      case 2:
        // Linear scaling
        currentResult *= 0.30f;  // needs a bit more damping, get stay below 255
        currentResult -= 4.0f;   // giving a bit more room for peaks
        if (currentResult < 1.0f)
          currentResult = 0.0f;
        currentResult *= 0.85f + (float(i) / 1.8f);  // extra up-scaling for high frequencies
        break;
      case 3:
        // square root scaling
        currentResult *= 0.38f;
        currentResult -= 6.0f;
        if (currentResult > 1.0f)
          currentResult = sqrtf(currentResult);
        else
          currentResult = 0.0f;                      // special handling, because sqrt(0) = undefined
        currentResult *= 0.85f + (float(i) / 4.5f);  // extra up-scaling for high frequencies
        currentResult = remap(currentResult, 0.0f, 16.0f, 0.0f, 255.0f);  // map [sqrt(1) ... sqrt(256)] to [0 ... 255]
        break;
      case 0:
      default:
        // no scaling - leave freq bins as-is
        currentResult -= 4;  // just a bit more room for peaks
        break;
    }

    // Now, let's dump it all into fftResult.
    // Need to do this, otherwise other routines might grab fftResult values prematurely.
    if (soundAgc > 0) {  // apply extra "GEQ Gain" if set by user
      float post_gain = (float) inputLevel / 128.0f;
      if (post_gain < 1.0f)
        post_gain = ((post_gain - 1.0f) * 0.8f) + 1.0f;
      currentResult *= post_gain;
    }
    fftResult[i] = constrain((int) currentResult, 0, 255);
  }
}

////////////////////
// Peak detection //
////////////////////

// peak detection is called from FFT task when vReal[] contains valid FFT results
static void detectSamplePeak(void) {
  // softhack007: this code continuously triggers while amplitude in the selected bin is above a certain threshold.
  // So it does not detect peaks - it detects high activity in a frequency bin.
  // Poor man's beat detection by seeing if sample > Average + some value.
  // This goes through ALL of the 255 bins - but ignores stupid settings
  // Then we got a peak, else we don't. The peak has to time out on its own in order to support UDP sound sync.
  if ((sampleAvg > 1.0f) && (maxVol > 0) && (binNum > 4) && (vReal[binNum] > maxVol) &&
      ((millis() - timeOfPeak) > 100)) {
    samplePeak = true;
    timeOfPeak = millis();
  }
}

static void autoResetPeak(void) {
  if (millis() - timeOfPeak > uint16_t(50)) {  // Auto-reset of samplePeak after 50ms
    samplePeak = false;
  }
}

// *****************************************************************************
// FFT main task
// audio processing task: read samples, run FFT, fill GEQ channels from FFT results
// *****************************************************************************
void MusicLeds::getSamples(float *buffer) {
  if (!this->microphone_is_running()) {
    return;
  }

  // Get fresh samples
  uint8_t samples[BUFFER_SIZE] = {0};
  size_t bytes_read = ((AudioSource *) this->microphone_)->read_(samples, BUFFER_SIZE, pdMS_TO_TICKS(FFT_MIN_CYCLE));

  // For correct operation, we need to read exactly sizeof(samples) bytes from i2s
  if (bytes_read != BUFFER_SIZE) {
    ESP_LOGE("ASR", "AS: Failed to get enough samples: wanted: %d read: %d", BUFFER_SIZE, bytes_read);
    return;
  }

  // Intermediary sample storage
  I2S_datatype *newSamples = reinterpret_cast<I2S_datatype *>(samples);

  // Store samples in sample buffer and update DC offset
  for (int i = 0; i < SAMPLES_FFT; i++) {
    float currSample = 0.0f;
#ifdef I2S_SAMPLE_DOWNSCALE_TO_16BIT
    currSample = (float) newSamples[i] / 65536.0f;  // 32bit input -> 16bit; keeping lower 16bits as decimal places
#else
    currSample = (float) newSamples[i];  // 16bit input -> use as-is
#endif
    buffer[i] = currSample;  // store sample
  }
}

void MusicLeds::FFTcode(void *parameter) {
  MusicLeds *this_task = (MusicLeds *) parameter;
  ESP_LOGCONFIG(TAG, "FFT: started on core: %u", FFTTASK_CORE);

  xEventGroupSetBits(this_task->event_group_, EventGroupBits::TASK_STARTING);

  {  // Ensures any C++ objects fall out of scope to deallocate before deleting the task
    // Allocate FFT buffers on first call
    if (vReal == nullptr) {
      vReal = (float *) calloc(sizeof(float), SAMPLES_FFT);
    }
    if (vImag == nullptr) {
      vImag = (float *) calloc(sizeof(float), SAMPLES_FFT);
    }

    if ((vReal == nullptr) || (vImag == nullptr)) {
      // Something went wrong
      if (vReal) {
        free(vReal);
        vReal = nullptr;
      }
      if (vImag) {
        free(vImag);
        vImag = nullptr;
      }
      ESP_LOGW(TAG, "Allocate FFT buffers failed.");
      xEventGroupSetBits(this_task->event_group_, EventGroupBits::ERROR_MEMORY);
      this_task->status_set_warning();
      return;
    }

    // Create FFT object with weighing factor storage
    ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, SAMPLES_FFT, SAMPLE_RATE, true);

    xEventGroupSetBits(this_task->event_group_, EventGroupBits::TASK_RUNNING);

    this_task->microphone_->start();

    // see https://www.freertos.org/vtaskdelayuntil.html
    const TickType_t xFrequency = FFT_MIN_CYCLE * portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (!(xEventGroupGetBits(this_task->event_group_) & EventGroupBits::COMMAND_STOP)) {
      // Only run the FFT computing code if microphone running
      if (!this_task->microphone_is_running()) {
        this_task->status_momentary_warning("Microphone not running!");
        vTaskDelay(FFT_MIN_CYCLE / portTICK_PERIOD_MS);
        continue;
      }
      this_task->status_clear_warning();

      this_task->getSamples(vReal);
      memset(vImag, 0, SAMPLES_FFT * sizeof(float));  // Set imaginary parts to 0

#ifdef USE_BANDPASSFILTER
      // band pass filter - can reduce noise floor by a factor of 50
      // downside: frequencies below 100Hz will be ignored
      runMicFilter(SAMPLES_FFT, vReal);
#endif

      // find highest sample in the batch
      float maxSample = 0.0f;  // max sample from FFT batch
      for (int i = 0; i < SAMPLES_FFT; i++) {
        // pick our  our current mic sample - we take the max value from all samples that go into FFT
        // skip extreme values - normally these are artefacts
        if ((vReal[i] <= (INT16_MAX - 1024)) && (vReal[i] >= (INT16_MIN + 1024))) {
          if (fabsf((float) vReal[i]) > maxSample)
            maxSample = fabsf((float) vReal[i]);
        }
      }

      // release highest sample to volume reactive effects early - not strictly necessary here - could also be done at
      // the end of the function early release allows the filters (getSample() and agcAvg()) to work with fresh values -
      // we will have matching gain and noise gate values when we want to process the FFT results.
      micDataReal = maxSample;

      if (sampleAvg >
          0.25f) {  // noise gate open means that FFT results will be used. Don't run FFT if results are not needed.
        // run FFT (takes 3-5ms on ESP32, ~12ms on ESP32-S2)
        FFT.dcRemoval();  // remove DC offset
        // Weigh data using "Flat Top" function better amplitude accuracy
        FFT.windowing(FFTWindow::Flat_top, FFTDirection::Forward);
        // Weigh data using "Blackman- Harris" window - sharp peaks due to excellent sideband rejection
        // FFT.windowing(FFTWindow::Blackman_Harris, FFTDirection::Forward);
        FFT.compute(FFTDirection::Forward);  // Compute FFT
        FFT.complexToMagnitude();            // Compute magnitudes

        //
        // vReal[3 .. 255] contain useful data, each a 20Hz interval (60Hz - 5120Hz).
        // There could be interesting data at bins 0 to 2, but there are too many artifacts.
        //
        vReal[0] = 0;  // The remaining DC offset on the signal produces a strong spike on position 0 that should be
                       // eliminated to avoid issues.

        FFT.majorPeak(&FFT_MajorPeak, &FFT_Magnitude);             // let the effects know which freq was most dominant
        FFT_MajorPeak = constrain(FFT_MajorPeak, 1.0f, 11025.0f);  // restrict value to range expected by effects
        FFT_Magnitude = fabsf(FFT_Magnitude);
      } else {  // noise gate closed - only clear results as FFT was skipped.
                // MIC samples are still valid when we do this.
        memset(vReal, 0, SAMPLES_FFT * sizeof(float));
        FFT_MajorPeak = 1;
        FFT_Magnitude = 0.001;
      }

      for (int i = 0; i < SAMPLES_FFT; i++) {
        float cSample = fabsf(vReal[i]);  // just to be sure - values in fft bins should be positive any way
        vReal[i] = cSample / 16.0f;       // Reduce magnitude. Want end result to be scaled linear and ~4096 max.
      }

      // clang-format off
      // mapping of FFT result bins to frequency channels
      if (sampleAvg > 0.5f) {  // noise gate open
        /*
        * This FFT post processing is a DIY endeavour.
        * What we really need is someone with sound engineering expertise to do a great job here AND most importantly, that the animations look GREAT as a result.
        * Andrew's updated mapping of 256 bins down to the 16 result bins with Sample Freq = 10240, samplesFFT = 512 and some overlap.
        * Based on testing, the lowest/Start frequency is 60 Hz (with bin 3) and a highest/End frequency of 5120 Hz in bin 255.
        * Now, Take the 60Hz and multiply by 1.320367784 to get the next frequency and so on until the end. Then determine the bins.
        * End frequency = Start frequency * multiplier ^ 16
        * Multiplier = (End frequency/ Start frequency) ^ 1/16
        * Multiplier = 1.320367784
        * new mapping, optimized for 22050 Hz by softhack007
        */

        //                                          // bins frequency  range
        #ifdef USE_BANDPASSFILTER
        // skip frequencies below 100hz
        fftCalc[ 0] = 0.8f * fftAddAvg(3,4);
        fftCalc[ 1] = 0.9f * fftAddAvg(4,5);
        fftCalc[ 2] = fftAddAvg(5,6);
        fftCalc[ 3] = fftAddAvg(6,7);
        // don't use the last bins from 206 to 255.
        fftCalc[15] = fftAddAvg(165,205) * 0.75f;   // 40 7106 - 8828 high  -- with some damping
        #else
        fftCalc[ 0] = fftAddAvg(1,2);               // 1    43 - 86   sub-bass
        fftCalc[ 1] = fftAddAvg(2,3);               // 1    86 - 129  bass
        fftCalc[ 2] = fftAddAvg(3,5);               // 2   129 - 216  bass
        fftCalc[ 3] = fftAddAvg(5,7);               // 2   216 - 301  bass + midrange
        // don't use the last bins from 216 to 255. They are usually contaminated by aliasing (aka noise)
        fftCalc[15] = fftAddAvg(165,215) * 0.70f;   // 50 7106 - 9259 high  -- with some damping
        #endif
        fftCalc[ 4] = fftAddAvg(7,10);              // 3   301 - 430  midrange
        fftCalc[ 5] = fftAddAvg(10,13);             // 3   430 - 560  midrange
        fftCalc[ 6] = fftAddAvg(13,19);             // 5   560 - 818  midrange
        fftCalc[ 7] = fftAddAvg(19,26);             // 7   818 - 1120 midrange  -- 1Khz should always be the center !
        fftCalc[ 8] = fftAddAvg(26,33);             // 7  1120 - 1421 midrange
        fftCalc[ 9] = fftAddAvg(33,44);             // 9  1421 - 1895 midrange
        fftCalc[10] = fftAddAvg(44,56);             // 12 1895 - 2412 midrange + high mid
        fftCalc[11] = fftAddAvg(56,70);             // 14 2412 - 3015 high mid
        fftCalc[12] = fftAddAvg(70,86);             // 16 3015 - 3704 high mid
        fftCalc[13] = fftAddAvg(86,104);            // 18 3704 - 4479 high mid
        fftCalc[14] = fftAddAvg(104,165) * 0.88f;   // 61 4479 - 7106 high mid + high  -- with slight damping
      } else {                                      // noise gate closed - just decay old values
        for (int i = 0; i < NUM_GEQ_CHANNELS; i++) {
          fftCalc[i] *= 0.85f;  // decay to zero
          if (fftCalc[i] < 4.0f)
            fftCalc[i] = 0.0f;
        }
      }
      // clang-format on

      // post-processing of frequency channels (pink noise adjustment, AGC, smoothing, scaling)
      postProcessFFTResults((fabsf(sampleAvg) > 0.25f) ? true : false, NUM_GEQ_CHANNELS);

      // run peak detection
      autoResetPeak();
      detectSamplePeak();
#ifdef DEBUG
      xEventGroupSetBits(this_task->event_group_, EventGroupBits::TASK_INFO);
#endif
      vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }  // while (!(xEventGroupGetBits(this_task->event_group_) & COMMAND_STOP))

    xEventGroupSetBits(this_task->event_group_, EventGroupBits::TASK_STOPPING);

    this_task->microphone_->stop();

    xEventGroupSetBits(this_task->event_group_, EventGroupBits::TASK_STOPPED);

    while (true) {
      // Continuously delay until the loop method deletes the task
      vTaskDelay(FFT_MIN_CYCLE / portTICK_PERIOD_MS);
    }
  }
}  // FFTcode() task end

// *****************************************************************************
// Audio Processing
// *****************************************************************************

/*
 * A "PI controller" multiplier to automatically adjust sound sensitivity.
 *
 * A few tricks are implemented so that sampleAgc does't only utilize 0% and 100%:
 * 0. don't amplify anything below squelch (but keep previous gain)
 * 1. gain input = maximum signal observed in the last 5-10 seconds
 * 2. we use two setpoints, one at ~60%, and one at ~80% of the maximum signal
 * 3. the amplification depends on signal level:
 *    a) normal zone - very slow adjustment
 *    b) emergency zone (<10% or >90%) - very fast adjustment
 */
void MusicLeds::agcAvg(unsigned long the_time) {
  // Make sure the _compiler_ knows this value will not change while we are inside the function
  const int AGC_preset = (soundAgc > 0) ? (soundAgc - 1) : 0;

  float lastMultAgc = multAgc;                // last multiplier used
  float multAgcTemp = multAgc;                // new multiplier
  float tmpAgc = this->sampleReal * multAgc;  // what-if amplified signal

  float control_error;  // "control error" input for PI control

  if (this->last_soundAgc != soundAgc) {
    this->control_integrated = 0.0;  // new preset - reset integrator
  }

  // For PI controller, we need to have a constant "frequency"
  // so let's make sure that the control loop is not running at insane speed
  static unsigned long last_time = 0;
  unsigned long time_now = millis();
  if ((the_time > 0) && (the_time < time_now))
    time_now = the_time;  // allow caller to override my clock

  if (time_now - last_time > 2) {
    last_time = time_now;

    if ((fabsf(this->sampleReal) < 2.0f) || (this->sampleMax < 1.0)) {
      // MIC signal is "squelched" - deliver silence
      tmpAgc = 0;
      // we need to "spin down" the intgrated error buffer
      if (fabs(this->control_integrated) < 0.01) {
        this->control_integrated = 0.0;
      } else {
        this->control_integrated *= 0.91;
      }
    } else {
      // compute new setpoint
      if (tmpAgc <= agcTarget0Up[AGC_preset]) {
        // Make the multiplier so that sampleMax * multiplier = first setpoint
        multAgcTemp = agcTarget0[AGC_preset] / this->sampleMax;
      } else {
        // Make the multiplier so that sampleMax * multiplier = second setpoint
        multAgcTemp = agcTarget1[AGC_preset] / this->sampleMax;
      }
    }

    // limit amplification
    if (multAgcTemp > 32.0f)
      multAgcTemp = 32.0f;
    if (multAgcTemp < 1.0f / 64.0f)
      multAgcTemp = 1.0f / 64.0f;

    // compute error terms
    control_error = multAgcTemp - lastMultAgc;

    if (((multAgcTemp > 0.085f) && (multAgcTemp < 6.5f))           // integrator anti-windup by clamping
        && (multAgc * this->sampleMax < agcZoneStop[AGC_preset]))  // integrator ceiling (>140% of max)
      this->control_integrated += control_error * 0.002 * 0.25;    // 2ms = integration time; 0.25 for damping
    else
      this->control_integrated *= 0.9;  // spin down that beasty integrator

    // apply PI Control
    tmpAgc = this->sampleReal * lastMultAgc;  // check "zone" of the signal using previous gain
    if ((tmpAgc > agcZoneHigh[AGC_preset]) ||
        (tmpAgc < soundSquelch + agcZoneLow[AGC_preset])) {  // upper/lower energy zone
      multAgcTemp = lastMultAgc + agcFollowFast[AGC_preset] * agcControlKp[AGC_preset] * control_error;
      multAgcTemp += agcFollowFast[AGC_preset] * agcControlKi[AGC_preset] * this->control_integrated;
    } else {  // "normal zone"
      multAgcTemp = lastMultAgc + agcFollowSlow[AGC_preset] * agcControlKp[AGC_preset] * control_error;
      multAgcTemp += agcFollowSlow[AGC_preset] * agcControlKi[AGC_preset] * this->control_integrated;
    }

    // limit amplification again - PI controller sometimes "overshoots"
    // multAgcTemp = constrain(multAgcTemp, 0.015625f, 32.0f); // 1/64 < multAgcTemp < 32
    if (multAgcTemp > 32.0f)
      multAgcTemp = 32.0f;
    if (multAgcTemp < 1.0f / 64.0f)
      multAgcTemp = 1.0f / 64.0f;
  }

  // NOW finally amplify the signal
  tmpAgc = this->sampleReal * multAgcTemp;  // apply gain to signal
  if (fabsf(this->sampleReal) < 2.0f)
    tmpAgc = 0.0f;  // apply squelch threshold
  // tmpAgc = constrain(tmpAgc, 0, 255);
  if (tmpAgc > 255)
    tmpAgc = 255.0f;  // limit to 8bit
  if (tmpAgc < 1)
    tmpAgc = 0.0f;  // just to be sure

  // update global vars ONCE - multAgc, sampleAGC, rawSampleAgc
  multAgc = multAgcTemp;
  this->rawSampleAgc = 0.8f * tmpAgc + 0.2f * (float) this->rawSampleAgc;
  // update smoothed AGC sample
  if (fabsf(tmpAgc) < 1.0f)
    sampleAgc = 0.5f * tmpAgc + 0.5f * sampleAgc;  // fast path to zero
  else
    sampleAgc += agcSampleSmooth[AGC_preset] * (tmpAgc - sampleAgc);  // smooth path

  sampleAgc = fabsf(sampleAgc);  // make sure we have a positive value
  this->last_soundAgc = soundAgc;
}  // agcAvg()

// post-processing and filtering of MIC sample (micDataReal) from FFTcode()
void MusicLeds::getSample() {
  float sampleAdj;               // Gain adjusted sample value
  float tmpSample;               // An interim sample variable used for calculations.
  const float weighting = 0.2f;  // Exponential filter weighting. Will be adjustable in a future release.
  const int AGC_preset =
      (soundAgc > 0) ? (soundAgc - 1)
                     : 0;  // make sure the _compiler_ knows this value will not change while we are inside the function

  this->micIn = int(micDataReal);  // micDataSm = ((micData * 3) + micData)/4;

  this->micLev += (micDataReal - this->micLev) / 12288.0f;
  if (this->micIn < this->micLev) {
    this->micLev = ((this->micLev * 31.0f) + micDataReal) / 32.0f;  // align MicLev to lowest input signal
  }

  this->micIn -= this->micLev;  // Let's center it to 0 now
  // Using an exponential filter to smooth out the signal. We'll add controls for this in a future release.
  float micInNoDC = fabsf(micDataReal - this->micLev);
  this->expAdjF = (weighting * micInNoDC + (1.0f - weighting) * this->expAdjF);
  this->expAdjF = fabsf(this->expAdjF);  // Now (!) take the absolute value

  this->expAdjF = (this->expAdjF <= soundSquelch) ? 0 : this->expAdjF;  // simple noise gate
  if ((soundSquelch == 0) && (this->expAdjF < 0.25f))
    this->expAdjF = 0;  // do something meaningfull when "squelch = 0"

  tmpSample = this->expAdjF;
  this->micIn = abs(this->micIn);  // And get the absolute value of each sample

  sampleAdj = tmpSample * sampleGain / 40.0f * inputLevel / 128.0f +
              tmpSample / 16.0f;  // Adjust the gain. with inputLevel adjustment
  this->sampleReal = tmpSample;

  sampleAdj = fmax(fmin(sampleAdj, 255), 0);  // Question: why are we limiting the value to 8 bits ???
  this->sampleRaw = (int16_t) sampleAdj;      // ONLY update sample ONCE!!!!

  // keep "peak" sample, but decay value if current sample is below peak
  if ((this->sampleMax < this->sampleReal) && (this->sampleReal > 0.5f)) {
    this->sampleMax = this->sampleMax + 0.5f * (this->sampleReal - this->sampleMax);  // new peak - with some filtering
    // another simple way to detect samplePeak - cannot detect beats, but reacts on peak volume
    if (((binNum < 12) || ((maxVol < 1))) && (millis() - timeOfPeak > 80) && (sampleAvg > 1.0f)) {
      samplePeak = true;
      timeOfPeak = millis();
    }
  } else {
    if ((multAgc * this->sampleMax > agcZoneStop[AGC_preset]) && (soundAgc > 0))
      this->sampleMax += 0.5f * (this->sampleReal - this->sampleMax);  // over AGC Zone - get back quickly
    else
      this->sampleMax *= agcSampleDecay[AGC_preset];  // signal to zero --> 5-8sec
  }
  if (this->sampleMax < 0.5f)
    this->sampleMax = 0.0f;

  sampleAvg = ((sampleAvg * 15.0f) + sampleAdj) / 16.0f;  // Smooth it out over the last 16 samples.
  sampleAvg = fabsf(sampleAvg);                           // make sure we have a positive value
}  // getSample()

#ifdef USE_SOUND_DYNAMICS_LIMITER
/*
 * Limits the dynamics of volumeSmth (= sampleAvg or sampleAgc).
 * does not affect FFTResult[] or volumeRaw ( = sample or rawSampleAgc)
 * effects: Gravimeter, Gravcenter, Gravcentric, Noisefire, Plasmoid, Freqpixels, Freqwave, Gravfreq, (2D Swirl, 2D
 * Waverly)
 */
void MusicLeds::limitSampleDynamics(void) {
  const float bigChange = 196;  // just a representative number - a large, expected sample value
  static unsigned long last_time = 0;
  static float last_volumeSmth = 0.0f;

  long delta_time = millis() - last_time;
  delta_time = constrain(delta_time, 1, 1000);  // below 1ms -> 1ms; above 1sec -> sily lil hick-up
  float deltaSample = this->volumeSmth - last_volumeSmth;

  if (attackTime > 0) {  // user has defined attack time > 0
    float maxAttack = bigChange * float(delta_time) / float(attackTime);
    if (deltaSample > maxAttack)
      deltaSample = maxAttack;
  }
  if (decayTime > 0) {  // user has defined decay time > 0
    float maxDecay = -bigChange * float(delta_time) / float(decayTime);
    if (deltaSample < maxDecay)
      deltaSample = maxDecay;
  }

  this->volumeSmth = last_volumeSmth + deltaSample;

  last_volumeSmth = this->volumeSmth;
  last_time = millis();
}
#endif

// *****************************************************************************************************************************************************************

void MusicLeds::on_start() {
  // Init sound data
  micDataReal = 0.0f;
  sampleAgc = 0;
  sampleAvg = 0;
  FFT_Magnitude = 0;
  FFT_MajorPeak = 1;
  multAgc = 1;

  this->volumeRaw = 0;
  this->volumeSmth = 0;
  this->sampleRaw = 0;
  this->rawSampleAgc = 0;
  this->my_magnitude = 0;

  // Reset FFT data
  memset(fftCalc, 0, sizeof(fftCalc));
#ifdef USE_SOUND_DYNAMICS_LIMITER
  memset(fftAvg, 0, sizeof(fftAvg));
#endif
  memset(fftResult, 0, sizeof(fftResult));
  for (int i = 0; i < NUM_GEQ_CHANNELS; i += 2) {
    fftResult[i] = 16;  // make a tiny pattern
  }
  inputLevel = 128;  // reset level slider to default
  autoResetPeak();

  // Define the FFT Task and lock it to core
  xTaskCreatePinnedToCore(MusicLeds::FFTcode,  // Function to implement the task
                          "FFT",               // Name of the task
                          5000,                // Stack size in words
                          (void *) this,       // Task input parameter
                          FFTTASK_PRIORITY,    // Priority of the task
                          &this->FFT_Task,     // Task handle
                          FFTTASK_CORE);       // Core where the task should run
  if (this->FFT_Task == nullptr) {
    this->status_momentary_error("MusicLeds task failed to start...", 1000);
  }
}

void MusicLeds::on_stop() {
  vTaskDelete(this->FFT_Task);
  this->FFT_Task = nullptr;

  fastled_helper::FreeLeds();

  this->status_clear_error();
}

void MusicLeds::on_loop() {
  static unsigned long lastUMRun = millis();
#if defined(MUSIC_LEDS_TRIGGERS)
  static unsigned long lastTrigger = millis();
#endif

  if (soundAgc > AGC_NUM_PRESETS)
    soundAgc = 0;  // make sure that AGC preset is valid (to avoid array bounds violation)

  unsigned long t_now = millis();  // remember current time
  int userloopDelay = int(t_now - lastUMRun);
  if (lastUMRun == 0)
    userloopDelay = 0;  // startup - don't have valid data from last run.

  // run filters, and repeat in case of loop delays (hick-up compensation)
  if (userloopDelay < 2)
    userloopDelay = 0;  // minor glitch, no problem
  if (userloopDelay > 200)
    userloopDelay = 200;  // limit number of filter re-runs
  do {
    this->getSample();                    // run microphone sampling filters
    this->agcAvg(t_now - userloopDelay);  // Calculated the PI adjusted value as sampleAvg
    userloopDelay -= 2;                   // advance "simulated time" by 2ms
  } while (userloopDelay > 0);
  lastUMRun = t_now;  // update time keeping

  // update samples for effects (raw, smooth)
  this->volumeSmth = (soundAgc) ? sampleAgc : sampleAvg;
  this->volumeRaw = (soundAgc) ? this->rawSampleAgc : this->sampleRaw;

  // update FFTMagnitude, taking into account AGC amplification
  this->my_magnitude = FFT_Magnitude;  // / 16.0f, 8.0f, 4.0f done in effects
  if (soundAgc)
    this->my_magnitude *= multAgc;
  if (this->volumeSmth < 1) {
    this->my_magnitude = 0.001f;  // noise gate closed - mute
  }

#ifdef USE_SOUND_DYNAMICS_LIMITER
  this->limitSampleDynamics();
#endif
  autoResetPeak();  // auto-reset sample peak after strip minShowDelay

  uint32_t event_group_bits = xEventGroupGetBits(this->event_group_);

#ifdef DEBUG
  if ((event_group_bits & EventGroupBits::TASK_INFO)) {
    if (millis() % 50 == 0)
      ESP_LOGE(TAG, "Samples: High: %f | volumeSmth: %f | sampleAgc: %f | sampleAvg: %f", micDataReal, this->volumeSmth,
               sampleAgc, sampleAvg);
    xEventGroupClearBits(this->event_group_, EventGroupBits::TASK_INFO);
  }
#endif

#if defined(MUSIC_LEDS_TRIGGERS)
  if (t_now - lastTrigger > 200) {
    for (auto *t : on_sound_loop_triggers_) {
      t->process(this->volumeSmth, this->volumeRaw, FFT_MajorPeak, samplePeak);
    }
    lastTrigger = t_now;
  }
#endif
}

// *****************************************************************************
// Data
// *****************************************************************************

// allocates effect data buffer on heap and initialises (erases) it
bool MusicLeds::allocateData(size_t len) {
  if (len == 0) {
    return false;  // nothing to do
  }
  if (this->data && this->_dataLen >= len) {  // already allocated enough (reduce fragmentation)
    if (this->start_effect_) {
      memset(data, 0, len);  // erase buffer if called during effect initialisation
    }
    return true;
  }

  this->deallocateData();  // if the old buffer was smaller release it first
  // Do not use SPI RAM on ESP32 since it is slow
  this->data = (byte *) calloc(len, sizeof(byte));
  if (!this->data) {
    this->status_momentary_warning("Effect data, allocation failed!");
    return false;
  }  // allocation failed

  this->_dataLen = len;
  return true;
}

void MusicLeds::deallocateData() {
  if (!this->data) {
    this->_dataLen = 0;
    return;
  }
  // check that we don't have a dangling / inconsistent data pointer
  if (this->_dataLen > 0) {
    free(this->data);
  }
  this->data = nullptr;
  _dataLen = 0;
}

// *****************************************************************************
// Speed and Variant
// *****************************************************************************
void MusicLeds::set_speed(int index) { this->speed = index; }

void MusicLeds::set_variant(int index) { this->variant = index; }

// *****************************************************************************
// Effects
// *****************************************************************************
void MusicLeds::ShowFrame(PLAYMODE CurrentMode, esphome::Color current_color, light::AddressableLight *p_it) {
  if (!this->is_running() && !this->microphone_is_running()) {
    return;
  }

  fastled_helper::InitLeds(p_it->size());

  this->leds_num = p_it->size();

  this->main_color = CRGB(current_color.r, current_color.g, current_color.b);
  if ((int) fastled_helper::current_palette == 0) {
    // 5% from main color
    this->back_color = CRGB(current_color.r / 100 * 5, current_color.g / 100 * 5, current_color.b / 100 * 5);
  } else {
    this->back_color = CRGB::Black;
  }

  switch (CurrentMode) {
#ifdef DEF_GRAV
    case MODE_GRAV:
      this->visualize_gravfreq(fastled_helper::leds);
      break;
#endif
#ifdef DEF_GRAVICENTER
    case MODE_GRAVICENTER:
      this->visualize_gravcenter(fastled_helper::leds);
      break;
#endif
#ifdef DEF_GRAVICENTRIC
    case MODE_GRAVICENTRIC:
      this->visualize_gravcentric(fastled_helper::leds);
      break;
#endif
#ifdef DEF_GRAVIMETER
    case MODE_GRAVIMETER:
      this->visualize_gravmeter(fastled_helper::leds);
      break;
#endif
#ifdef DEF_PIXELS
    case MODE_PIXELS:
      this->visualize_pixels(fastled_helper::leds);
      break;
#endif
#ifdef DEF_JUNGLES
    case MODE_JUNGLES:
      this->visualize_juggles(fastled_helper::leds);
      break;
#endif
#ifdef DEF_MIDNOISE
    case MODE_MIDNOISE:
      this->visualize_midnoise(fastled_helper::leds);
      break;
#endif
#ifdef DEF_RIPPLEPEAK
    case MODE_RIPPLEPEAK:
      this->visualize_ripplepeak(fastled_helper::leds);
      break;
#endif
#ifdef DEF_MATRIPIX
    case MODE_MATRIPIX:
      this->visualize_matripix(fastled_helper::leds);
      break;
#endif
#ifdef DEF_NOISEFIRE
    case MODE_NOISEFIRE:
      this->visualize_noisefire(fastled_helper::leds);
      break;
#endif
#ifdef DEF_PIXELWAVE
    case MODE_PIXELWAVE:
      this->visualize_pixelwave(fastled_helper::leds);
      break;
#endif
#ifdef DEF_PLASMOID
    case MODE_PLASMOID:
      this->visualize_plasmoid(fastled_helper::leds);
      break;
#endif
#ifdef DEF_PUDDLEPEAK
    case MODE_PUDDLEPEAK:
      this->visualize_puddlepeak(fastled_helper::leds);
      break;
#endif
#ifdef DEF_PUDDLES
    case MODE_PUDDLES:
      this->visualize_puddles(fastled_helper::leds);
      break;
#endif
#ifdef DEF_DJLIGHT
    case MODE_DJLIGHT:
      this->visualize_DJLight(fastled_helper::leds);
      break;
#endif
#ifdef DEF_WATERFALL
    case MODE_WATERFALL:
      this->visualize_waterfall(fastled_helper::leds);
      break;
#endif
  }

  for (int i = 0; i < p_it->size(); i++) {
    (*p_it)[i] = Color(fastled_helper::leds[i].r, fastled_helper::leds[i].g, fastled_helper::leds[i].b);
  }

  this->start_effect_ = false;
  delay_microseconds_safe(1);
}

// *****************************************************************************************************************************************************************
#if defined(DEF_GRAV) || defined(DEF_GRAVICENTER) || defined(DEF_GRAVICENTRIC) || defined(DEF_GRAVIMETER)

#define MAX_FREQUENCY 11025      // sample frequency / 2 (as per Nyquist criterion)
#define MAX_FREQ_LOG10 4.04238f  // log10(MAX_FREQUENCY)

// Gravity struct requited for GRAV* effects
typedef struct Gravity {
  int topLED;
  int gravityCounter;
} gravity;

// Gravcenter effects By Andrew Tuline.
// Gravcenter base function for Gravcenter (0), Gravcentric (1), Gravimeter (2), Gravfreq (3)
void MusicLeds::mode_gravcenter_base(unsigned mode, CRGB *physic_leds) {
  const unsigned dataSize = sizeof(gravity);
  if (!this->allocateData(dataSize)) {
    return;  // allocation failed
  }
  Gravity *gravcen = reinterpret_cast<Gravity *>(this->data);

  if (mode == 1) {  // Gravcentric
    fastled_helper::fade_out(physic_leds, this->leds_num, 253, this->back_color);
  } else if (mode == 2) {  // Gravimeter
    fastled_helper::fade_out(physic_leds, this->leds_num, 249, this->back_color);
  } else if (mode == 3) {  // Gravfreq
    fastled_helper::fade_out(physic_leds, this->leds_num, 250, this->back_color);
  } else {  // Gravcenter
    fastled_helper::fade_out(physic_leds, this->leds_num, 251, this->back_color);
  }

  float mySampleAvg;
  int tempsamp;
  float segmentSampleAvg = this->volumeSmth * (float) this->variant / 255.0f;

  if (mode == 2) {             // Gravimeter
    segmentSampleAvg *= 0.25;  // divide by 4, to compensate for later "sensitivity" upscaling
    // map to pixels availeable in current segment
    mySampleAvg = remap(segmentSampleAvg * 2.0f, 0.0f, 64.0f, 0.0f, (float) (this->leds_num - 1));
    tempsamp = constrain(mySampleAvg, 0, this->leds_num - 1);  // Keep the sample from overflowing.
  } else {                                                     // Gravcenter or Gravcentric or Gravfreq
    segmentSampleAvg *= 0.125f;  // divide by 8, to compensate for later "sensitivity" upscaling
    // map to pixels availeable in current segment
    mySampleAvg = remap(segmentSampleAvg * 2.0f, 0.0f, 32.0f, 0.0f, (float) this->leds_num / 2.0f);
    tempsamp = constrain(mySampleAvg, 0, this->leds_num / 2);  // Keep the sample from overflowing.
  }

  uint8_t gravity = 8 - this->speed / 32;
  int offset = (mode == 2) ? 0 : 1;
  if (tempsamp >= gravcen->topLED)
    gravcen->topLED = tempsamp - offset;
  else if (gravcen->gravityCounter % gravity == 0)
    gravcen->topLED--;

  if (mode == 1) {  // Gravcentric
    for (int i = 0; i < tempsamp; i++) {
      uint8_t index = segmentSampleAvg * 24 + millis() / 200;
      physic_leds[i + this->leds_num / 2] = fastled_helper::color_from_palette(index, this->main_color);
      physic_leds[this->leds_num / 2 - 1 - i] = fastled_helper::color_from_palette(index, this->main_color);
    }
    if (gravcen->topLED >= 0) {
      physic_leds[gravcen->topLED + this->leds_num / 2] = CRGB::Gray;
      physic_leds[this->leds_num / 2 - 1 - gravcen->topLED] = CRGB::Gray;
    }
  } else if (mode == 2) {  // Gravimeter
    for (int i = 0; i < tempsamp; i++) {
      uint8_t index = fastled_helper::perlin8(i * segmentSampleAvg + millis(), 5000 + i * segmentSampleAvg);
      physic_leds[i] = fastled_helper::color_blend(
          this->back_color, fastled_helper::color_from_palette(index, this->main_color), segmentSampleAvg * 8);
    }
    if (gravcen->topLED > 0) {
      physic_leds[gravcen->topLED] = fastled_helper::color_from_palette(millis(), this->main_color);
    }
  } else if (mode == 3) {  // Gravfreq
    for (int i = 0; i < tempsamp; i++) {
      float fft_MajorPeak = FFT_MajorPeak;  // used in mode 3: Gravfreq
      if (fft_MajorPeak < 1) {
        fft_MajorPeak = 1;
      }
      uint8_t index = (log10f(fft_MajorPeak) - (MAX_FREQ_LOG10 - 1.78f)) * 255;
      physic_leds[i + this->leds_num / 2] = fastled_helper::color_from_palette(index, this->main_color);
      physic_leds[this->leds_num / 2 - i - 1] = fastled_helper::color_from_palette(index, this->main_color);
    }
    if (gravcen->topLED >= 0) {
      physic_leds[gravcen->topLED + this->leds_num / 2] = CRGB::Gray;
      physic_leds[this->leds_num / 2 - 1 - gravcen->topLED] = CRGB::Gray;
    }
  } else {  // Gravcenter
    for (int i = 0; i < tempsamp; i++) {
      uint8_t index = fastled_helper::perlin8(i * segmentSampleAvg + millis(), 5000 + i * segmentSampleAvg);
      physic_leds[i + this->leds_num / 2] = fastled_helper::color_blend(
          this->back_color, fastled_helper::color_from_palette(index, this->main_color), segmentSampleAvg * 8);
      physic_leds[this->leds_num / 2 - i - 1] = fastled_helper::color_blend(
          this->back_color, fastled_helper::color_from_palette(index, this->main_color), segmentSampleAvg * 8);
    }
    if (gravcen->topLED >= 0) {
      physic_leds[gravcen->topLED + this->leds_num / 2] =
          fastled_helper::color_from_palette(millis(), this->main_color);
      physic_leds[this->leds_num / 2 - 1 - gravcen->topLED] =
          fastled_helper::color_from_palette(millis(), this->main_color);
    }
  }
  gravcen->gravityCounter = (gravcen->gravityCounter + 1) % gravity;
}
#endif

// *****************************************************************************************************************************************************************
#ifdef DEF_GRAVICENTER
void MusicLeds::visualize_gravcenter(CRGB *physic_leds)  // Gravcenter. By Andrew Tuline.
{
  mode_gravcenter_base(0, physic_leds);
}  // visualize_gravcenter()
#endif

// *****************************************************************************************************************************************************************
#ifdef DEF_GRAVICENTRIC
void MusicLeds::visualize_gravcentric(CRGB *physic_leds)  // Gravcentric. By Andrew Tuline.
{
  mode_gravcenter_base(1, physic_leds);
}  // visualize_gravcentric
#endif

// *****************************************************************************************************************************************************************
#ifdef DEF_GRAVIMETER
void MusicLeds::visualize_gravmeter(CRGB *physic_leds)  // Gravmeter. By Andrew Tuline.
{
  mode_gravcenter_base(2, physic_leds);
}  // visualize_gravcentric
#endif

// *****************************************************************************************************************************************************************
#ifdef DEF_GRAV
void MusicLeds::visualize_gravfreq(CRGB *physic_leds)  // Gravfreq. By Andrew Tuline.
{
  return mode_gravcenter_base(3, physic_leds);
}  // visualize_gravfreq
#endif

// *****************************************************************************************************************************************************************
#ifdef DEF_PIXELS
void MusicLeds::visualize_pixels(CRGB *physic_leds)  // Pixels. By Andrew Tuline.
{
  if (!this->allocateData(32 * sizeof(uint8_t))) {
    return;  // allocation failed
  }
  uint8_t *myVals = reinterpret_cast<uint8_t *>(this->data);

  myVals[millis() % 32] = this->volumeSmth;  // filling values semi randomly

  fastled_helper::fade_out(physic_leds, this->leds_num, 64 + (this->speed >> 1), this->back_color);

  for (int i = 0; i < (int) this->variant / 8; i++) {
    uint16_t segLoc = fastled_helper::hw_random16(this->leds_num);  // 16 bit for larger strands of LED's.
    physic_leds[segLoc] = fastled_helper::color_blend(
        this->back_color, fastled_helper::color_from_palette(myVals[i % 32] + i * 4, this->main_color),
        this->volumeSmth);
  }
}  // visualize_pixels()
#endif

// *****************************************************************************************************************************************************************
#ifdef DEF_JUNGLES
void MusicLeds::visualize_juggles(CRGB *physic_leds)  // Juggles. By Andrew Tuline.
{
  fastled_helper::fade_out(physic_leds, this->leds_num, 224, this->back_color);

  int my_sampleAgc = fmax(fmin(this->volumeSmth, 255.0), 0);

  for (int i = 0; i < (int) this->variant / 32 + 1; i++) {
    physic_leds[beatsin16((int) this->speed / 4 + i * 2, 0, this->leds_num - 1)] = fastled_helper::color_blend(
        this->back_color, fastled_helper::color_from_palette(millis() / 4 + i * 2, this->main_color), my_sampleAgc);
  }
}  // visualize_juggles()
#endif

// *****************************************************************************************************************************************************************
#ifdef DEF_MIDNOISE
void MusicLeds::visualize_midnoise(CRGB *physic_leds)  // Midnoise. By Andrew Tuline.
{
  static int x = 0;
  static int y = 0;

  // Same as two fade-out runs
  fastled_helper::fade_out(physic_leds, this->leds_num, (int) this->speed * (int) this->speed / 255, this->back_color);

  float tmpSound = this->volumeSmth;
  float tmpSound2 = tmpSound * (float) this->variant / 256.0;  // Too sensitive.
  tmpSound2 *= (float) this->variant / 128.0;                  // Reduce sensitity/length.

  unsigned maxLen = remap((float) tmpSound2, 0.0f, 127.0f, 0.0f, (float) this->leds_num / 2);
  if (maxLen > this->leds_num / 2)
    maxLen = this->leds_num / 2;

  for (int i = (this->leds_num / 2 - maxLen); i < (this->leds_num / 2 + maxLen); i++) {
    // Get a value from the noise function. I'm using both x and y axis.
    uint8_t index = fastled_helper::perlin8(i * tmpSound + x, y + i * tmpSound);
    physic_leds[i] = fastled_helper::color_from_palette(index, this->main_color);
  }

  x = x + beatsin8(5, 0, 10);
  y = y + beatsin8(4, 0, 10);
}  // visualize_midnoise()
#endif

#ifdef DEF_RIPPLEPEAK
typedef struct Ripple {
  uint8_t state;
  uint8_t color;
  uint16_t pos;
} ripple;

void MusicLeds::visualize_ripplepeak(CRGB *physic_leds)  // Ripple peak. By Andrew Tuline.
{                                                        // This currently has no controls.
#define MAXSTEPS 16                                      // Case statement wouldn't allow a variable.

  unsigned maxRipples = 16;
  unsigned dataSize = sizeof(Ripple) * maxRipples;
  if (!this->allocateData(dataSize)) {
    return;  // allocation failed
  }
  Ripple *ripples = reinterpret_cast<Ripple *>(this->data);

  // Lower frame rate means less effective fading than FastLED
  // 225 should be the same as 240 applied twice
  fastled_helper::fade_out(physic_leds, this->leds_num, 225, this->back_color);

  for (int i = 0; i < this->variant / 16; i++) {  // Limit the number of ripples.
    if (samplePeak)
      ripples[i].state = 255;

    switch (ripples[i].state) {
      case 254:  // Inactive mode
        break;

      case 255:  // Initialize ripple variables.
        ripples[i].pos = fastled_helper::hw_random16(this->leds_num);
        if (FFT_MajorPeak > 1)  // log10(0) is "forbidden" (throws exception)
          ripples[i].color = (int) (log10f(FFT_MajorPeak) * 128);
        else
          ripples[i].color = 0;
        ripples[i].state = 0;
        break;

      case 0:
        physic_leds[ripples[i].pos] = fastled_helper::color_from_palette(ripples[i].color, this->main_color);
        ripples[i].state++;
        break;

      case MAXSTEPS:  // At the end of the ripples. 254 is an inactive mode.
        ripples[i].state = 254;
        break;

      default:  // Middle of the ripples.
        physic_leds[(ripples[i].pos + ripples[i].state + this->leds_num) % this->leds_num] =
            fastled_helper::color_blend(this->back_color,
                                        fastled_helper::color_from_palette(ripples[i].color, this->main_color),
                                        uint8_t(2 * 255 / ripples[i].state));
        physic_leds[(ripples[i].pos - ripples[i].state + this->leds_num) % this->leds_num] =
            fastled_helper::color_blend(this->back_color,
                                        fastled_helper::color_from_palette(ripples[i].color, this->main_color),
                                        uint8_t(2 * 255 / ripples[i].state));
        ripples[i].state++;  // Next step.
        break;
    }  // switch step
  }  // for i
}  // visualize_ripplepeak()
#endif

#ifdef DEF_MATRIPIX
void MusicLeds::visualize_matripix(CRGB *physic_leds)  // Matripix. By Andrew Tuline.
{
  // effect can work on single pixels, we just lose the shifting effect
  unsigned dataSize = sizeof(CRGB) * this->leds_num;
  if (!this->allocateData(dataSize)) {
    return;  // allocation failed
  }
  CRGB *pixels = reinterpret_cast<CRGB *>(this->data);

  uint8_t secondHand = micros() / (256 - this->speed) / 500 % 16;
  if (this->store != secondHand) {
    this->store = secondHand;

    uint16_t pixBri = this->volumeRaw * this->variant / 64;
    unsigned k = this->leds_num - 1;
    // loop will not execute if SEGLEN equals 1
    for (unsigned i = 0; i < k; i++) {
      pixels[i] = pixels[i + 1];  // shift left
      physic_leds[i] = pixels[i];
    }
    pixels[k] = fastled_helper::color_blend(this->back_color,
                                            fastled_helper::color_from_palette(millis(), this->main_color), pixBri);
    physic_leds[k] = pixels[k];
  }
}  // visualize_matripix()
#endif

#ifdef DEF_NOISEFIRE
// I am the god of hellfire. . . Volume (only) reactive fire routine. Oh, look how short this is.
void MusicLeds::visualize_noisefire(CRGB *physic_leds)  // Noisefire. By Andrew Tuline.
{
  // Fire palette definition. Lower value = darker.
  CRGBPalette16 myPal =
      CRGBPalette16(CHSV(0, 255, 2), CHSV(0, 255, 4), CHSV(0, 255, 8), CHSV(0, 255, 8), CHSV(0, 255, 16), CRGB::Red,
                    CRGB::Red, CRGB::Red, CRGB::DarkOrange, CRGB::DarkOrange, CRGB::Orange, CRGB::Orange, CRGB::Yellow,
                    CRGB::Orange, CRGB::Yellow, CRGB::Yellow);

  for (unsigned i = 0; i < this->leds_num; i++) {
    // X location is constant, but we move along the Y at the rate of millis(). By Andrew Tuline.
    unsigned index = fastled_helper::perlin8(i * this->speed / 64, millis() * this->speed / 64 * this->leds_num / 255);
    // Now we need to scale index so that it gets blacker as we get close to one of the ends.
    // This is a simple y=mx+b equation that's been scaled. index/128 is another scaling.
    index = (255 - i * 256 / this->leds_num) * index / (256 - this->variant);

    physic_leds[i] = ColorFromPalette(myPal, index, this->volumeSmth * 2, LINEARBLEND);  // Use my own palette.
  }
}  // visualize_noisefire()
#endif

#ifdef DEF_PIXELWAVE
void MusicLeds::visualize_pixelwave(CRGB *physic_leds)  // Pixelwave. By Andrew Tuline.
{
  uint8_t secondHand = micros() / (256 - this->speed) / 500 + 1 % 16;
  if (this->store != secondHand) {
    this->store = secondHand;

    uint8_t pixBri = this->volumeRaw * this->variant / 64;

    physic_leds[this->leds_num / 2] = fastled_helper::color_blend(
        this->back_color, fastled_helper::color_from_palette(millis(), this->main_color), pixBri);
    for (unsigned i = this->leds_num - 1; i > this->leds_num / 2; i--) {
      physic_leds[i] = physic_leds[i - 1];  // move to the left
    }
    for (unsigned i = 0; i < this->leds_num / 2; i++) {
      physic_leds[i] = physic_leds[i + 1];  // move to the right
    }
  }

}  // visualize_pixelwave()
#endif

#ifdef DEF_PLASMOID
typedef struct Plasphase {
  int16_t thisphase;
  int16_t thatphase;
} plasphase;

void MusicLeds::visualize_plasmoid(CRGB *physic_leds)  // Plasmoid. By Andrew Tuline.
{
  if (!this->allocateData(sizeof(plasphase))) {
    return;  // allocation failed
  }
  Plasphase *plasmoip = reinterpret_cast<Plasphase *>(this->data);

  fastled_helper::fade_out(physic_leds, this->leds_num, 32, this->back_color);

  plasmoip->thisphase += beatsin8(6, -4, 4);  // You can change direction and speed individually.
  plasmoip->thatphase += beatsin8(7, -4, 4);  // Two phase values to make a complex pattern. By Andrew Tuline.

  for (unsigned i = 0; i < this->leds_num;
       i++) {  // For each of the LED's in the strand, set a brightness based on a wave as follows.
    // updated, similar to "plasma" effect - softhack007
    uint8_t thisbright = cubicwave8(((i * (1 + (3 * this->speed / 32))) + plasmoip->thisphase) & 0xFF) / 2;
    // Let's munge the brightness a bit and animate it all with the phases.
    thisbright += cos8(((i * (97 + (5 * this->speed / 32))) + plasmoip->thatphase) & 0xFF) / 2;

    uint8_t colorIndex = thisbright;
    if (this->volumeSmth * this->variant / 64 < thisbright) {
      thisbright = 0;
    }

    physic_leds[i] = fastled_helper::color_blend(
        this->back_color, fastled_helper::color_from_palette(colorIndex, this->main_color), thisbright);
  }
}  // visualize_plasmoid()
#endif

#if defined(DEF_PUDDLES) || defined(DEF_PUDDLEPEAK)
// Puddles / Puddlepeak By Andrew Tuline.
void MusicLeds::puddles_base(CRGB *physic_leds, bool peakdetect) {
  unsigned size = 0;

  uint8_t fadeVal = map(this->speed, 0, 255, 224, 254);
  unsigned pos = fastled_helper::hw_random16(this->leds_num);  // Set a random starting position.
  fastled_helper::fade_out(physic_leds, this->leds_num, fadeVal, this->back_color);

  if (peakdetect) {  // Puddles peak
    if (samplePeak) {
      size = this->volumeSmth * this->variant / 256 / 4 + 1;  // Determine size of the flash based on the volume.
      if (pos + size >= this->leds_num)
        size = this->leds_num - pos;
    }
  } else {  // Puddles
    if (this->volumeRaw > 1) {
      size = this->volumeRaw * this->variant / 256 / 8 + 1;  // Determine size of the flash based on the volume.
      if (pos + size >= this->leds_num)
        size = this->leds_num - pos;
    }
  }

  for (unsigned i = 0; i < size; i++) {  // Flash the LED's.
    physic_leds[pos + i] = fastled_helper::color_from_palette(millis(), this->main_color);
  }
}  // puddles_base()
#endif

#ifdef DEF_PUDDLEPEAK
void MusicLeds::visualize_puddlepeak(CRGB *physic_leds)  // Puddlepeak. By Andrew Tuline.
{
  puddles_base(physic_leds, true);
}  // visualize_puddlepeak()
#endif

#ifdef DEF_PUDDLES
void MusicLeds::visualize_puddles(CRGB *physic_leds)  // Puddles. By Andrew Tuline.
{
  puddles_base(physic_leds, false);
}  // visualize_puddles()
#endif

#ifdef DEF_DJLIGHT
void MusicLeds::visualize_DJLight(CRGB *physic_leds)  // DJLight. Written by ??? Adapted by Will Tatam.
{
  // No need to prevent from executing on single led strips, only mid will be set (mid = 0)
  const int mid = this->leds_num / 2;

  uint8_t secondHand = micros() / (256 - this->speed) / 500 + 1 % 64;
  if (this->store != secondHand) {
    this->store = secondHand;

    CRGB color = CRGB(fftResult[15] / 2, fftResult[5] / 2, fftResult[0] / 2);   // 16-> 15 as 16 is out of bounds
    physic_leds[mid] = color.fadeToBlackBy(map(fftResult[4], 0, 255, 255, 4));  // TODO - Update

    for (int i = this->leds_num - 1; i > mid; i--) {
      physic_leds[i] = physic_leds[i - 1];  // move to the left
    }
    for (int i = 0; i < mid; i++) {
      physic_leds[i] = physic_leds[i + 1];  // move to the right
    }
  }
}  // visualize_DJLight()
#endif

#ifdef DEF_WATERFALL
// Combines peak detection with FFT_MajorPeak and FFT_Magnitude.
void MusicLeds::visualize_waterfall(CRGB *physic_leds)  // Waterfall. By: Andrew Tuline
{
  unsigned dataSize = sizeof(CRGB) * this->leds_num;
  if (!this->allocateData(dataSize)) {
    return;  // allocation failed
  }
  CRGB *pixels = reinterpret_cast<CRGB *>(this->data);

  if (FFT_MajorPeak < 1)
    FFT_MajorPeak = 1;  // log10(0) is "forbidden" (throws exception)

  uint8_t secondHand = micros() / (256 - this->speed) / 500 + 1 % 16;
  if (this->store != secondHand) {  // Triggered millis timing.
    this->store = secondHand;

    // 10Khz sampling - log10 frequency range is from 2.26 (182hz) to 3.7 (5012hz). Let's scale accordingly.
    // uint8_t pixCol = (log10f((float)FFT_MajorPeak) - 2.26f) * 177;
    // 22Khz sampling - log10 frequency range is from 2.26 (182hz) to 3.967 (9260hz). Let's scale accordingly.
    uint8_t pixCol = (log10f(FFT_MajorPeak) - 2.26f) * 150;
    if (FFT_MajorPeak < 182.0f)
      pixCol = 0;  // handle underflow

    unsigned k = this->leds_num - 1;
    if (samplePeak) {
      pixels[k] = CRGB(CHSV(92, 92, 92));
    } else {
      pixels[k] = fastled_helper::color_blend(
          this->back_color, fastled_helper::color_from_palette(pixCol + this->variant, this->main_color),
          (uint8_t) (my_magnitude / 8.0f));
    }
    physic_leds[k] = pixels[k];

    for (unsigned i = 0; i < k; i++) {
      pixels[i] = pixels[i + 1];  // shift left
      physic_leds[i] = pixels[i];
    }
  }
}  // visualize_waterfall()
#endif

#if defined(MUSIC_LEDS_TRIGGERS)
void MusicLedsSoundLoopTrigger::process(float volume_smth, int16_t volume_raw, float fft_major_peak, bool sample_peak) {
  this->trigger(volume_smth, volume_raw, fft_major_peak, sample_peak);
}
#endif

}  // namespace music_leds
}  // namespace esphome

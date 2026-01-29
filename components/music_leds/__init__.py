"""Music Leds component for ESPHome - Fixed for ESPHome 2025.12+
   Original: https://github.com/andrewjswan/esphome-components
   Fix: Removed ota.request_ota_state_listeners() which was removed in ESPHome 2025.12
"""

import logging

import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome import automation, core
from esphome.components import microphone
from esphome.components.light.effects import register_addressable_effect
from esphome.components.light.types import AddressableLightEffect
from esphome.const import (
    CONF_BITS_PER_SAMPLE,
    CONF_ID,
    CONF_MICROPHONE,
    CONF_MODE,
    CONF_NAME,
    CONF_PLATFORM,
    CONF_SAMPLE_RATE,
    CONF_TRIGGER_ID,
)

from .const import (
    CONF_BAND_PASS_FILTER,
    CONF_FFT_SCALING,
    CONF_GAINCONTROL,
    CONF_MUSIC_LEDS_ID,
    CONF_ON_SOUND_LOOP,
    CONF_SOUND_DYNAMICS_LIMITER,
    CONF_SR_GAIN,
    CONF_SR_SQUELCH,
    CONF_TASK_CORE,
    CONF_TASK_PRIORITY,
    SAMPLE_RATE_10,
    SAMPLE_RATE_16,
    SAMPLE_RATE_20,
    SAMPLE_RATE_22,
)

_LOGGER = logging.getLogger(__name__)

CODEOWNERS = ["@andrewjswan"]

DEPENDENCIES = ["light", "microphone"]

AUTO_LOAD = ["fastled_helper"]

logging.info("Load Music Leds component (local version)")

music_leds_ns = cg.esphome_ns.namespace("music_leds")
MUSIC_LEDS = music_leds_ns.class_("MusicLeds", cg.Component)
MUSIC_LEDS_EFECT = music_leds_ns.class_("MusicLedsLightEffect", AddressableLightEffect)

SoundLoopTrigger = music_leds_ns.class_(
    "MusicLedsSoundLoopTrigger",
    automation.Trigger.template(cg.std_string),
)

PlayMode = music_leds_ns.enum("PLAYMODE")
MUSIC_LEDS_EFFECTS = {
    "GRAV": PlayMode.MODE_GRAV,
    "GRAVICENTER": PlayMode.MODE_GRAVICENTER,
    "GRAVICENTRIC": PlayMode.MODE_GRAVICENTRIC,
    "GRAVIMETER": PlayMode.MODE_GRAVIMETER,
    "PIXELS": PlayMode.MODE_PIXELS,
    "JUNGLES": PlayMode.MODE_JUNGLES,
    "MIDNOISE": PlayMode.MODE_MIDNOISE,
    "RIPPLEPEAK": PlayMode.MODE_RIPPLEPEAK,
    "MATRIPIX": PlayMode.MODE_MATRIPIX,
    "NOISEFIRE": PlayMode.MODE_NOISEFIRE,
    "PIXELWAVE": PlayMode.MODE_PIXELWAVE,
    "PLASMOID": PlayMode.MODE_PLASMOID,
    "PUDDLEPEAK": PlayMode.MODE_PUDDLEPEAK,
    "PUDDLES": PlayMode.MODE_PUDDLES,
    "DJLIGHT": PlayMode.MODE_DJLIGHT,
    "WATERFALL": PlayMode.MODE_WATERFALL,
}

MUSIC_LEDS_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(MUSIC_LEDS),
        cv.Optional(CONF_BAND_PASS_FILTER, default=False): cv.boolean,
        cv.Optional(CONF_SOUND_DYNAMICS_LIMITER, default=True): cv.boolean,
        cv.Optional(CONF_BITS_PER_SAMPLE): cv.float_,
        cv.Optional(CONF_SAMPLE_RATE): cv.int_,
        cv.Optional(CONF_TASK_CORE, default=1): cv.int_range(0, 1),
        cv.Optional(CONF_TASK_PRIORITY, default=10): cv.int_range(1, 10),
        cv.Optional(CONF_GAINCONTROL, default=0): cv.int_range(0, 3),
        cv.Optional(CONF_FFT_SCALING, default=3): cv.int_range(0, 3),
        cv.Optional(CONF_SR_GAIN, default=60): cv.int_range(0, 255),
        cv.Optional(CONF_SR_SQUELCH, default=10): cv.int_range(0, 255),
        cv.Optional(CONF_ON_SOUND_LOOP): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(SoundLoopTrigger),
            },
        ),
    },
).extend(
    {
        cv.GenerateID(CONF_MICROPHONE): cv.use_id(microphone.Microphone),
    },
)

CONFIG_SCHEMA = cv.All(MUSIC_LEDS_SCHEMA)


def _final_validate(config):
    full_config = fv.full_config.get()

    path = full_config.get_path_for_id(config[CONF_ID])[:-1]
    this_config = full_config.get_config_for_path(path)

    mic_path = full_config.get_path_for_id(config[CONF_MICROPHONE])[:-1]
    mic_conf = full_config.get_config_for_path(mic_path)
    logging.info("Microphone: %s", mic_conf.get(CONF_PLATFORM))

    if CONF_SAMPLE_RATE in mic_conf:
        rate = mic_conf.get(CONF_SAMPLE_RATE)
        this_config[CONF_SAMPLE_RATE] = rate
        logging.info("Sample Rate: %s", rate)

    if CONF_BITS_PER_SAMPLE in mic_conf:
        bits = mic_conf.get(CONF_BITS_PER_SAMPLE)
        if bits not in [16, 32]:
            msg = "Music Leds support only 16 or 32 Bits Per Sample"
            raise cv.Invalid(msg)
        this_config[CONF_BITS_PER_SAMPLE] = bits
        logging.info("Bits Per Sample: %s", bits)


FINAL_VALIDATE_SCHEMA = _final_validate


async def to_code(config) -> None:
    """Code generation entry point."""
    var = cg.new_Pvariable(config[CONF_ID])

    # FIX: Removed ota.request_ota_state_listeners() - API removed in ESPHome 2025.12+
    # The C++ code handles OTA gracefully with #ifdef USE_OTA_STATE_LISTENER

    cg.add_library("kosme/arduinoFFT", None)
    cg.add_define("sqrt_internal", "sqrtf")

    cg.add_define("USE_MUSIC_LEDS")
    cg.add_define("FASTLED_USE_ADAFRUIT_NEOPIXEL")

    cg.add_build_flag("-Wno-narrowing")

    mic = await cg.get_variable(config[CONF_MICROPHONE])
    cg.add(var.set_microphone(mic))

    cg.add_define("BITS_PER_SAMPLE", int(config[CONF_BITS_PER_SAMPLE]))
    cg.add_define("FFTTASK_CORE", config[CONF_TASK_CORE])
    cg.add_define("FFTTASK_PRIORITY", config[CONF_TASK_PRIORITY])
    cg.add_define("SR_GAIN", config[CONF_SR_GAIN])
    cg.add_define("SR_SQUELCH", config[CONF_SR_SQUELCH])
    cg.add_define("FFT_SCALING", config[CONF_FFT_SCALING])
    cg.add_define("GAIN_CONTROL", config[CONF_GAINCONTROL])
    cg.add_define("USE_SOUND_DYNAMICS_LIMITER", config[CONF_SOUND_DYNAMICS_LIMITER])
    cg.add_define("USE_BANDPASSFILTER", config[CONF_BAND_PASS_FILTER])
    cg.add_define("SAMPLE_RATE", config[CONF_SAMPLE_RATE])

    if config[CONF_SAMPLE_RATE] >= SAMPLE_RATE_22:
        cg.add_define("FFT_MIN_CYCLE", 21)
    elif config[CONF_SAMPLE_RATE] >= SAMPLE_RATE_20:
        cg.add_define("FFT_MIN_CYCLE", 23)
    elif config[CONF_SAMPLE_RATE] >= SAMPLE_RATE_16:
        cg.add_define("FFT_MIN_CYCLE", 30)
    elif config[CONF_SAMPLE_RATE] >= SAMPLE_RATE_10:
        cg.add_define("FFT_MIN_CYCLE", 46)
    else:
        msg = "Low Sample rate for Music Leds plz increase."
        raise core.EsphomeError(msg)

    if config.get(CONF_ON_SOUND_LOOP, []):
        cg.add_define("MUSIC_LEDS_TRIGGERS")
        logging.info("[X] On Sound Loop trigger")
        for conf in config.get(CONF_ON_SOUND_LOOP, []):
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
            await automation.build_automation(
                trigger,
                [
                    (cg.float_, "volume_smth"),
                    (cg.int16, "volume_raw"),
                    (cg.float_, "fft_major_peak"),
                    (cg.bool_, "sample_peak"),
                ],
                conf,
            )

    await cg.register_component(var, config)


@register_addressable_effect(
    "music_leds_effect",
    MUSIC_LEDS_EFECT,
    "Music Leds",
    {
        cv.GenerateID(CONF_MUSIC_LEDS_ID): cv.use_id(MUSIC_LEDS),
        cv.Optional(CONF_MODE, default="PIXELS"): cv.enum(MUSIC_LEDS_EFFECTS, upper=True),
    },
)
async def music_leds_light_effect_to_code(config, effect_id) -> AddressableLightEffect:
    """Effect registration entry point."""
    parent = await cg.get_variable(config[CONF_MUSIC_LEDS_ID])

    effect = cg.new_Pvariable(effect_id, config[CONF_NAME])

    cg.add(effect.set_mode(config[CONF_MODE]))
    cg.add_define("DEF_" + config[CONF_MODE])
    cg.add(effect.set_music_leds(parent))
    return effect

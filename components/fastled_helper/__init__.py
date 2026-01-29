"""Fastled Helper component for ESPHome."""

import logging

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

CONF_PALETTES = "palettes"
CONF_MUSIC_LEDS = "music_leds"

_LOGGER = logging.getLogger(__name__)

CODEOWNERS = ["@andrewjswan"]

# No AUTO_LOAD needed - component is explicitly loaded

logging.info("Load Fastled Helper component https://github.com/andrewjswan/esphome-components")

fastled_helper_ns = cg.esphome_ns.namespace("fastled_helper")
FASTLED_HELPER_ = fastled_helper_ns.class_("FastledHelper", cg.Component)

FASTLED_HELPER_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(FASTLED_HELPER_),
        cv.Optional(CONF_PALETTES, default=True): cv.templatable(cv.boolean),
        cv.Optional(CONF_MUSIC_LEDS, default=False): cv.templatable(cv.boolean),
    },
)

CONFIG_SCHEMA = cv.All(FASTLED_HELPER_SCHEMA)


async def to_code(config) -> None:
    """Code generation entry point."""
    var = cg.new_Pvariable(config[CONF_ID])

    cg.add_library("fastled/FastLED", "3.9.7")

    if config[CONF_PALETTES]:
        cg.add_define("PALETTES")
        if config[CONF_MUSIC_LEDS]:
            cg.add_define("MUSIC_LEDS")

    await cg.register_component(var, config)

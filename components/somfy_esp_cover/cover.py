import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cover
from esphome.const import CONF_ID

somfy_esp_cover_ns = cg.esphome_ns.namespace("somfy_esp_cover")
SomfyESPCover = somfy_esp_cover_ns.class_("SomfyESPCover", cover.Cover, cg.Component)

CONF_NVS_NAME = "nvs_name"
CONF_NVS_KEY = "nvs_key"
CONF_REMOTE_CODE = "remote_code"
CONF_OPEN_DURATION = "open_duration"
CONF_CLOSE_DURATION = "close_duration"
CONF_MY_POSTION = "my_position"
CONF_CLOSED_POSITION = "closed_position"
CONF_HALF_CLOSED_POSITION = "half_closed_position"

CONFIG_SCHEMA = cover.COVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(SomfyESPCover),
        cv.Required(CONF_NVS_NAME): cv.string,
        cv.Required(CONF_NVS_KEY): cv.string,
        cv.Required(CONF_REMOTE_CODE): cv.uint32_t,
        cv.Optional(CONF_OPEN_DURATION, default = -1.0): cv.float_,
        cv.Optional(CONF_CLOSE_DURATION, default = -1.0): cv.float_,
        cv.Optional(CONF_MY_POSTION, default = -1.0): cv.float_,
        cv.Optional(CONF_CLOSED_POSITION, default = -1.0): cv.float_,
        cv.Optional(CONF_HALF_CLOSED_POSITION, default = -1.0): cv.float_
     }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cover.register_cover(var, config)

    cg.add(var.set_nvs_name(config[CONF_NVS_NAME]))
    cg.add(var.set_nvs_key(config[CONF_NVS_KEY]))
    cg.add(var.set_remote_code(config[CONF_REMOTE_CODE]))
    cg.add(var.set_open_duration(config[CONF_OPEN_DURATION]))
    cg.add(var.set_close_duration(config[CONF_CLOSE_DURATION]))
    cg.add(var.set_my_position(config[CONF_MY_POSTION]))
    cg.add(var.set_closed_position(config[CONF_CLOSED_POSITION]))
    cg.add(var.set_half_closed_position(config[CONF_HALF_CLOSED_POSITION]))

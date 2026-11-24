/**
 * @file zigbee_light.c
 * @brief Endpoint 1, Extended Color Light (0x010D): the ZCL attributes drive
 *        the lamp, and the last state survives a power cut.
 *
 * The SDK servers (On/Off, Level Control, Color Control, Groups, Scenes,
 * Identify) parse the commands and run the transitions; they only change
 * attributes. Every change of a light attribute schedules one "apply" event,
 * which reads OnOff, CurrentLevel and the colour of the current ColorMode and
 * hands them to lamp_apply(): the same gamma / balance / PWM path as the
 * console. Several attributes changed in one step (x and y) give one update.
 *
 * Persistence: all attributes are RAM attributes (the framework would write
 * an NVM attribute on every transition step). The application keeps one NVM3
 * object (APP_NVM3_KEY_LIGHT_STATE), written once the state has been stable
 * for APP_STATE_SAVE_DELAY_MS, at most once per APP_STATE_SAVE_MIN_INTERVAL_MS,
 * and only if it changed. At boot it is read back and StartUpOnOff,
 * StartUpCurrentLevel and StartUpColorTemperatureMireds are applied to it.
 */
#include "zigbee_light.h"
#include <string.h>
#include "app/framework/include/af.h"
#include "app/framework/util/attribute-storage.h"   /* sl_zigbee_af_reset_attributes */
#include "app_config.h"
#include "color.h"
#include "lamp.h"
#include "storage.h"

#define EP APP_ENDPOINT

static sl_zigbee_af_event_t apply_event;
static sl_zigbee_af_event_t save_event;
static sl_zigbee_af_event_t restore_event;

static storage_light_state_t saved;      /* what NVM3 holds (valid if have_saved) */
static bool have_saved;
static bool restored;                    /* no save before the boot restore ran */
static bool identifying;
static bool feedback;                    /* button feedback owns the LED (button.c) */
static uint32_t last_write_ms;
static bool written_once;
static uint32_t write_count;             /* since boot, shown by `zigbee status` */

/* ------------------------------------------------------------------------- */

static uint8_t read_u8(sl_zigbee_af_cluster_id_t cluster, sl_zigbee_af_attribute_id_t attr,
                       uint8_t fallback)
{
  uint8_t v = fallback;
  if (sl_zigbee_af_read_server_attribute(EP, cluster, attr, &v, sizeof(v))
      != SL_ZIGBEE_ZCL_STATUS_SUCCESS) {
    return fallback;
  }
  return v;
}

static uint16_t read_u16(sl_zigbee_af_cluster_id_t cluster, sl_zigbee_af_attribute_id_t attr,
                         uint16_t fallback)
{
  uint16_t v = fallback;
  if (sl_zigbee_af_read_server_attribute(EP, cluster, attr, (uint8_t *)&v, sizeof(v))
      != SL_ZIGBEE_ZCL_STATUS_SUCCESS) {
    return fallback;
  }
  return v;
}

static void write_u8(sl_zigbee_af_cluster_id_t cluster, sl_zigbee_af_attribute_id_t attr,
                     uint8_t v, sl_zigbee_af_attribute_type_t type)
{
  (void)sl_zigbee_af_write_server_attribute(EP, cluster, attr, &v, type);
}

static void write_u16(sl_zigbee_af_cluster_id_t cluster, sl_zigbee_af_attribute_id_t attr,
                      uint16_t v)
{
  (void)sl_zigbee_af_write_server_attribute(EP, cluster, attr, (uint8_t *)&v,
                                            ZCL_INT16U_ATTRIBUTE_TYPE);
}

static void read_state(storage_light_state_t *s)
{
  memset(s, 0, sizeof(*s));
  s->on_off = read_u8(ZCL_ON_OFF_CLUSTER_ID, ZCL_ON_OFF_ATTRIBUTE_ID, 0) ? 1u : 0u;
  s->level = read_u8(ZCL_LEVEL_CONTROL_CLUSTER_ID, ZCL_CURRENT_LEVEL_ATTRIBUTE_ID, 254);
  if (s->level == 0u) {
    s->level = 1u;
  } else if (s->level > 254u) {
    s->level = 254u;
  }
  s->color_mode = read_u8(ZCL_COLOR_CONTROL_CLUSTER_ID,
                          ZCL_COLOR_CONTROL_COLOR_MODE_ATTRIBUTE_ID, 1);
  if (s->color_mode > 2u) {
    s->color_mode = 1u;
  }
  s->hue = read_u8(ZCL_COLOR_CONTROL_CLUSTER_ID,
                   ZCL_COLOR_CONTROL_CURRENT_HUE_ATTRIBUTE_ID, 0);
  s->saturation = read_u8(ZCL_COLOR_CONTROL_CLUSTER_ID,
                          ZCL_COLOR_CONTROL_CURRENT_SATURATION_ATTRIBUTE_ID, 0);
  s->x = read_u16(ZCL_COLOR_CONTROL_CLUSTER_ID,
                  ZCL_COLOR_CONTROL_CURRENT_X_ATTRIBUTE_ID, 0x616B);
  s->y = read_u16(ZCL_COLOR_CONTROL_CLUSTER_ID,
                  ZCL_COLOR_CONTROL_CURRENT_Y_ATTRIBUTE_ID, 0x607D);
  s->mireds = read_u16(ZCL_COLOR_CONTROL_CLUSTER_ID,
                       ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID, 250);
  s->start_up_on_off = read_u8(ZCL_ON_OFF_CLUSTER_ID, ZCL_START_UP_ON_OFF_ATTRIBUTE_ID, 0xFF);
  s->start_up_level = read_u8(ZCL_LEVEL_CONTROL_CLUSTER_ID,
                              ZCL_START_UP_CURRENT_LEVEL_ATTRIBUTE_ID, 0xFF);
  s->on_level = read_u8(ZCL_LEVEL_CONTROL_CLUSTER_ID, ZCL_ON_LEVEL_ATTRIBUTE_ID, 0xFF);
  s->start_up_mireds = read_u16(ZCL_COLOR_CONTROL_CLUSTER_ID,
                                ZCL_START_UP_COLOR_TEMPERATURE_MIREDS_ATTRIBUTE_ID, 0xFFFF);
  s->on_off_transition = read_u16(ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                  ZCL_ON_OFF_TRANSITION_TIME_ATTRIBUTE_ID, 0);
}

static lamp_rgb_t color_of(const storage_light_state_t *s)
{
  switch (s->color_mode) {
    case 0:  return color_from_zcl_hs(s->hue, s->saturation);
    case 2:  return color_from_mireds(s->mireds);
    default: return color_from_zcl_xy(s->x, s->y);
  }
}

/* ------------------------------------------------------------------------- */

static void apply_handler(sl_zigbee_af_event_t *event)
{
  (void)event;
  if (identifying || feedback) {
    return;   /* applied when Identify / the button feedback stops */
  }
  storage_light_state_t s;
  read_state(&s);
  lamp_apply(s.on_off != 0u, s.level, color_of(&s), APP_ZIGBEE_FOLLOW_FADE_MS);
}

static void schedule_save(void)
{
  if (restored) {
    /* Restart the debounce: the write happens once nothing changed for
     * APP_STATE_SAVE_DELAY_MS. */
    sl_zigbee_af_event_set_delay_ms(&save_event, APP_STATE_SAVE_DELAY_MS);
  }
}

static void save_handler(sl_zigbee_af_event_t *event)
{
  (void)event;
  storage_light_state_t s;
  read_state(&s);
  if (have_saved && memcmp(&s, &saved, sizeof(s)) == 0) {
    return;   /* nothing new: no write */
  }
  uint32_t now = halCommonGetInt32uMillisecondTick();
  if (written_once && (now - last_write_ms) < APP_STATE_SAVE_MIN_INTERVAL_MS) {
    sl_zigbee_af_event_set_delay_ms(&save_event,
                                    APP_STATE_SAVE_MIN_INTERVAL_MS - (now - last_write_ms));
    return;
  }
  if (storage_save_light_state(&s)) {
    saved = s;
    have_saved = true;
    write_count++;
  }
  last_write_ms = now;
  written_once = true;
}

/* Boot: last state from NVM3, start-up attributes applied, then the lamp. */
static void restore_handler(sl_zigbee_af_event_t *event)
{
  (void)event;
  storage_light_state_t s;
  if (storage_load_light_state(&s)) {
    saved = s;
    have_saved = true;

    /* Configuration attributes first. */
    write_u8(ZCL_ON_OFF_CLUSTER_ID, ZCL_START_UP_ON_OFF_ATTRIBUTE_ID,
             s.start_up_on_off, ZCL_ENUM8_ATTRIBUTE_TYPE);
    write_u8(ZCL_LEVEL_CONTROL_CLUSTER_ID, ZCL_START_UP_CURRENT_LEVEL_ATTRIBUTE_ID,
             s.start_up_level, ZCL_INT8U_ATTRIBUTE_TYPE);
    write_u8(ZCL_LEVEL_CONTROL_CLUSTER_ID, ZCL_ON_LEVEL_ATTRIBUTE_ID,
             s.on_level, ZCL_INT8U_ATTRIBUTE_TYPE);
    write_u16(ZCL_COLOR_CONTROL_CLUSTER_ID,
              ZCL_START_UP_COLOR_TEMPERATURE_MIREDS_ATTRIBUTE_ID,
              s.start_up_mireds);
    write_u16(ZCL_LEVEL_CONTROL_CLUSTER_ID, ZCL_ON_OFF_TRANSITION_TIME_ATTRIBUTE_ID,
              s.on_off_transition);

    /* ZCL 3.8.2.2.5 StartUpOnOff: 0 off, 1 on, 2 toggle, 0xFF previous. */
    uint8_t on = s.on_off;
    if (s.start_up_on_off == 0x00u) {
      on = 0u;
    } else if (s.start_up_on_off == 0x01u) {
      on = 1u;
    } else if (s.start_up_on_off == 0x02u) {
      on = on ? 0u : 1u;
    }
    /* ZCL 3.10.2.2.14 StartUpCurrentLevel: 0 minimum, 1..254 that level,
     * 0xFF previous. */
    uint8_t level = s.level;
    if (s.start_up_level == 0x00u) {
      level = 1u;
    } else if (s.start_up_level <= 254u) {
      level = s.start_up_level;
    }
    /* ZCL 5.2.2.2.1.22 StartUpColorTemperatureMireds: 0xFFFF previous, else
     * that temperature and ColorMode = 2. */
    uint8_t mode = s.color_mode;
    uint16_t mireds = s.mireds;
    if (s.start_up_mireds != 0xFFFFu
        && s.start_up_mireds >= APP_COLOR_TEMP_MIN_MIREDS
        && s.start_up_mireds <= APP_COLOR_TEMP_MAX_MIREDS) {
      mode = 2u;
      mireds = s.start_up_mireds;
    }

    write_u8(ZCL_COLOR_CONTROL_CLUSTER_ID, ZCL_COLOR_CONTROL_CURRENT_HUE_ATTRIBUTE_ID,
             s.hue, ZCL_INT8U_ATTRIBUTE_TYPE);
    write_u8(ZCL_COLOR_CONTROL_CLUSTER_ID, ZCL_COLOR_CONTROL_CURRENT_SATURATION_ATTRIBUTE_ID,
             s.saturation, ZCL_INT8U_ATTRIBUTE_TYPE);
    write_u16(ZCL_COLOR_CONTROL_CLUSTER_ID, ZCL_COLOR_CONTROL_CURRENT_X_ATTRIBUTE_ID, s.x);
    write_u16(ZCL_COLOR_CONTROL_CLUSTER_ID, ZCL_COLOR_CONTROL_CURRENT_Y_ATTRIBUTE_ID, s.y);
    write_u16(ZCL_COLOR_CONTROL_CLUSTER_ID, ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
              mireds);
    write_u8(ZCL_COLOR_CONTROL_CLUSTER_ID, ZCL_COLOR_CONTROL_COLOR_MODE_ATTRIBUTE_ID,
             mode, ZCL_ENUM8_ATTRIBUTE_TYPE);
    write_u8(ZCL_COLOR_CONTROL_CLUSTER_ID, ZCL_COLOR_CONTROL_ENHANCED_COLOR_MODE_ATTRIBUTE_ID,
             mode, ZCL_ENUM8_ATTRIBUTE_TYPE);
    write_u8(ZCL_LEVEL_CONTROL_CLUSTER_ID, ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
             level, ZCL_INT8U_ATTRIBUTE_TYPE);
    write_u8(ZCL_ON_OFF_CLUSTER_ID, ZCL_ON_OFF_ATTRIBUTE_ID, on, ZCL_BOOLEAN_ATTRIBUTE_TYPE);
    sl_zigbee_app_debug_println("Last state restored: %s, level %u, colour mode %u",
                                on ? "on" : "off", level, mode);
  } else {
    sl_zigbee_app_debug_println("No saved light state: lamp off");
  }
  restored = true;
  /* The lamp follows the attributes (off on a first boot: OnOff defaults to 0).
   * If the start-up attributes changed the state, the debounced save stores it;
   * an unchanged state is never rewritten. */
  sl_zigbee_af_event_set_active(&apply_event);
  schedule_save();
}

/* ------------------------------------------------------------------------- */

void zigbee_light_init(void)
{
  sl_zigbee_af_event_init(&apply_event, apply_handler);
  sl_zigbee_af_event_init(&save_event, save_handler);
  sl_zigbee_af_event_init(&restore_event, restore_handler);
  /* Runs on the first pass of the main loop, after the framework has set the
   * attributes to their defaults (that happens after sl_zigbee_af_main_init_cb). */
  sl_zigbee_af_event_set_active(&restore_event);
}

void zigbee_light_factory_reset(void)
{
  sl_zigbee_af_event_set_inactive(&save_event);
  identifying = false;
  (void)storage_erase_light_state();
  have_saved = false;
  sl_zigbee_af_groups_cluster_clear_group_table_cb(EP);
  sl_zigbee_af_scenes_cluster_clear_scene_table_cb(EP);
  sl_zigbee_af_reset_attributes(EP);   /* RAM defaults from the .zap: OnOff 0 */
  sl_zigbee_af_event_set_inactive(&save_event);   /* nothing to store: defaults */
  lamp_apply(false, 254u, color_from_zcl_xy(0x616B, 0x607D), APP_ZIGBEE_FOLLOW_FADE_MS);
}

bool zigbee_light_toggle(void)
{
  /* The On/Off server's own path for a received Toggle command: OnOff
   * attribute (reported if Zigbee2MQTT configured reporting), the Level
   * Control on/off effect, the scene marked invalid. The attribute change
   * then drives the LED and the debounced save like any Zigbee command. */
  return sl_zigbee_af_on_off_cluster_set_value_cb(EP, ZCL_TOGGLE_COMMAND_ID, false)
         == SL_ZIGBEE_ZCL_STATUS_SUCCESS;
}

void zigbee_light_feedback(bool active)
{
  feedback = active;
  if (active) {
    return;
  }
  if (identifying) {
    lamp_set_effect(LAMP_EFFECT_BLINK);
  } else {
    sl_zigbee_af_event_set_active(&apply_event);   /* back to the attribute state */
  }
}

uint32_t zigbee_light_nvm_write_count(void)
{
  return write_count;
}

/* ---- framework callbacks ------------------------------------------------ */

void sl_zigbee_af_post_attribute_change_cb(uint8_t endpoint,
                                           sl_zigbee_af_cluster_id_t clusterId,
                                           sl_zigbee_af_attribute_id_t attributeId,
                                           uint8_t mask,
                                           uint16_t manufacturerCode,
                                           uint8_t type,
                                           uint8_t size,
                                           uint8_t *value)
{
  (void)type;
  (void)size;
  (void)value;
  if (endpoint != EP || manufacturerCode != 0u || (mask & CLUSTER_MASK_SERVER) == 0u) {
    return;
  }
  bool light = false;
  bool persisted = false;
  switch (clusterId) {
    case ZCL_ON_OFF_CLUSTER_ID:
      light = (attributeId == ZCL_ON_OFF_ATTRIBUTE_ID);
      persisted = light || attributeId == ZCL_START_UP_ON_OFF_ATTRIBUTE_ID;
      break;
    case ZCL_LEVEL_CONTROL_CLUSTER_ID:
      light = (attributeId == ZCL_CURRENT_LEVEL_ATTRIBUTE_ID);
      persisted = light
                  || attributeId == ZCL_START_UP_CURRENT_LEVEL_ATTRIBUTE_ID
                  || attributeId == ZCL_ON_LEVEL_ATTRIBUTE_ID
                  || attributeId == ZCL_ON_OFF_TRANSITION_TIME_ATTRIBUTE_ID;
      break;
    case ZCL_COLOR_CONTROL_CLUSTER_ID:
      light = (attributeId == ZCL_COLOR_CONTROL_CURRENT_HUE_ATTRIBUTE_ID
               || attributeId == ZCL_COLOR_CONTROL_CURRENT_SATURATION_ATTRIBUTE_ID
               || attributeId == ZCL_COLOR_CONTROL_CURRENT_X_ATTRIBUTE_ID
               || attributeId == ZCL_COLOR_CONTROL_CURRENT_Y_ATTRIBUTE_ID
               || attributeId == ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID
               || attributeId == ZCL_COLOR_CONTROL_COLOR_MODE_ATTRIBUTE_ID);
      persisted = light
                  || attributeId == ZCL_START_UP_COLOR_TEMPERATURE_MIREDS_ATTRIBUTE_ID;
      break;
    default:
      break;
  }
  if (light && restored) {
    sl_zigbee_af_event_set_active(&apply_event);
  }
  if (persisted) {
    schedule_save();
  }
}

void sl_zigbee_af_identify_start_feedback_cb(uint8_t endpoint, uint16_t identifyTime)
{
  (void)identifyTime;
  if (endpoint != EP) {
    return;
  }
  identifying = true;
  if (!feedback) {
    lamp_set_effect(LAMP_EFFECT_BLINK);
  }
  sl_zigbee_app_debug_println("Identify: blinking for %u s", identifyTime);
}

void sl_zigbee_af_identify_stop_feedback_cb(uint8_t endpoint)
{
  if (endpoint != EP) {
    return;
  }
  identifying = false;
  sl_zigbee_af_event_set_active(&apply_event);   /* back to the attribute state */
}

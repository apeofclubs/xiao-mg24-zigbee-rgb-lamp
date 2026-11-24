// Zigbee2MQTT external converter: XIAO MG24 RGB battery lamp (OpenSource / XIAO-MG24-RGB-LiPo).
// Written for Zigbee2MQTT 2.x (modern extends). Installation: docs/zigbee.md section 6.
// Checked (read-only, not run) against zigbee-herdsman-converters 26.125.0 / Zigbee2MQTT 2.14.3: every option below
// exists in that version (dist/lib/modernExtend.d.ts LightArgs, IdentifyArgs; dist/lib/types.d.ts Definition.ota).
//
// What the firmware implements (firmware/config/zcl/zcl_config.zap, endpoint 1, Extended Color Light 0x010D):
//   On/Off with StartUpOnOff, Level Control (1..254, OnLevel, OnOffTransitionTime, StartUpCurrentLevel),
//   Color Control: hue/saturation (normal hue only), CIE xy and colour temperature 153..500 mireds
//   (ColorCapabilities 0x0019: no enhanced hue, no colour loop), Groups, Scenes, Identify.
import * as m from 'zigbee-herdsman-converters/lib/modernExtend';

export default {
    zigbeeModel: ['XIAO-MG24-RGB-LiPo'],
    model: 'XIAO-MG24-RGB-LiPo',
    vendor: 'OpenSource',
    description: 'Battery RGB lamp (Seeed XIAO MG24, open-source firmware)',
    extend: [
        m.light({
            colorTemp: {range: [153, 500]},
            // The SDK colour server has no "enhanced" hue commands: Z2M must send normal hue/saturation.
            color: {modes: ['xy', 'hs'], enhancedHue: false},
            // StartUpOnOff is implemented and saved: exposes power_on_behavior.
            powerOnBehavior: true,
            // Identify TriggerEffect (blink, breathe, okay...) is not implemented: no "effect" expose.
            effect: false,
            // OnLevel, OnOffTransitionTime, StartUpCurrentLevel and Options (execute_if_off) exist on the lamp.
            // OnTransitionTime (0x0012) and OffTransitionTime (0x0013) do not: list the features explicitly,
            // because levelConfig: {} would expose all six and the lamp would refuse two of them.
            levelConfig: {features: ['on_off_transition_time', 'on_level', 'current_level_startup', 'execute_if_off']},
            // colorTemp.startup defaults to true: exposes color_temp_startup (StartUpColorTemperatureMireds,
            // implemented and saved by the firmware).
            configureReporting: true,
        }),
        // Sleepy end device: Z2M tells the user an identify may wait until the lamp's next poll.
        m.identify({isSleepy: true}),
    ],
    ota: false,
};

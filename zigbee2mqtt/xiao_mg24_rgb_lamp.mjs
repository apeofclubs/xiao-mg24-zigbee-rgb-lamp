// Zigbee2MQTT external converter: XIAO MG24 RGB battery lamp (OpenSource / XIAO-MG24-RGB-LiPo).
// Written for Zigbee2MQTT 2.x (modern extends). Installation: docs/zigbee.md section 6.
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
            // OnLevel, OnOffTransitionTime and StartUpCurrentLevel are implemented and saved.
            levelConfig: {},
            configureReporting: true,
        }),
        m.identify(),
    ],
    ota: false,
};

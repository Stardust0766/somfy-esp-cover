# somfy-esp-cover
Implementation of a time-based cover for remote operation of RTS radio-controlled SOMFY blinds or shutters in ESPHome via an ESP32 and a CC1101 radio module.

## Installation
* Download the somfy.yaml example from the root folder.
* Create a new device in ESPHome and modify it to your own needs and configurations based on the provided somfy.yaml example.
* The the required sources will be downloaded from github at compile time.
** SmartRC-CC1101-Driver-Lib (v2.5.6)
** Somfy_Remote_Lib (v0.4.1)
** somfy-esp-cover external component

## Sensor Configuration Variables

| Configuration variables | Optional | Description |
| ------------- | ------------- | ------------- |
| nvs_name | No | Name of the component. Used in the NVS (non volatile storage) to store the rolling code. A string of maximum 30 characters. |
| nvs_key | No | Key (variable name) in the NVS to store the rolling code of the specific simulated somfy remote.. A string of maximum 30 characters. |
| remote_code | No | Code of the simulated somfy remote. Required to identify the simulated remote at the somfy cover. A 3 byte (6 nibble) integer value e.g. 0x123456 |
| open_duration | Yes | Duration in seconds to open the cover. Omit to disable the time-based position feature. |
| close_duration | Yes | Duration in seconds to close the cover. Omit to disable the time-based position feature. |
| my_position | Yes | Motor position for the 'my' position. Omit to disable the 'my' position feature. If the cover isn't moving and the stop button is pressed the cover moves to this position. |
| closed_position | Yes | Motor position until where the cover (shutter) is still closed. Omit to disable this feature. |
| half_closed_position | Yes | Motor position where the cover (shutter) is half closed. Omit to disable this feature. |
| invert_behavior | Yes | Invert behavior of the remote control. This seems to be needed for awnings. |

</br>
</br>

# Compile locally

For developing purpose it is useful to install ESPHome loacally so that you can compile your component locally before testing it on your Home Assistent system.
For installtion please refer to: https://esphome.io/guides/installing_esphome

To compile the **somy-esp-cover** component:

```
cd <Path to your local Git reposizories>\somfy-esp-cover
esphome.exe compile .\somfy.yaml
```
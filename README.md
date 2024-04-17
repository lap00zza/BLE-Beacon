# Simple BLE Beacon

This is a very simple "beacon" made using the NimBLE stack in ESP IDF. The advertisement is in the following format

`3 bytes flags | 12 bytes Complete_Local_Name | 16 bytes Manufacturer_Specific_Data`

## Setup

- Open `idf.py menuconfig`
- Enable Bluetooth (`Component config > Bluetooth`)
- select `NimBLE` as the Host
- _Optionally_, disable the `Central` and `Observer` roles under `NimBLE Options` as this is a beacon only (`Broadcast` as a `Peripheral`). You can also set the `BLE GAP default device name` to something relevant. I set mine to `BLE Beacon`.

## LICENSE

Copyright Jewel Mahanta <jewelmahanta@gmail.com> 2024, [MIT LICENSE](/LICENSE)

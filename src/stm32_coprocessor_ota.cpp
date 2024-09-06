/**
 * @file   stm32_coprocessor_ota.cpp
 * @author Ben Freudberg
 * @date   9-6-2024
 * @brief  File controlling coprocessor OTA firmware updates for stm32 MCUs.
 * Modeled after https://github.com/particle-iot/asset-ota-examples/tree/main
 * */

// Particle Libraries
#include "Particle.h"                                 // Because it is a CPP file not INO
#include "STM32_Flash.h"							  // Library for flashing firmware to stm32 coprocessor
// Application Libraries / Class Files
#include "device_pinout.h"							  // Define pinouts and initialize them

// Tell Device OS to call handleAssets() when new firmware assets are available
// In this case, the asset is the STM32 binary. It will be flashed before the main program starts

void handleAssets(spark::Vector<ApplicationAsset> assets);
STARTUP(System.onAssetOta(handleAssets));

void handleAssets(spark::Vector<ApplicationAsset> assets) {
  initializePinModes();
  bool flashed = false;
  for (auto& asset: assets) {
    if (asset.name() == "sensor_hub_data_collection_firmware.bin") {
      // Flash the STM32 binary
      LOG(INFO, "Flashing STM32 from asset");
      // the STM32_Flash library expects a reset pin rather than a power enable pin, but it can work this way too
      flashStm32Binary(asset, SENSOR_HUB_DTR_PIN, SENSOR_HUB_ENABLE_PIN, STM32_BOOT_NONINVERTED);
      flashed = true;
      digitalWrite(SENSOR_HUB_ENABLE_PIN, HIGH); //turn off sensor hub
    } else {
      LOG(WARN, "Unknown asset %s", asset.name().c_str());
    }
  }

  if (!flashed) {
    LOG(WARN, "No STM32 binary found in the assets");
  }
  System.assetsHandled(true);
}

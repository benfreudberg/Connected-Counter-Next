#include "Asset_Communicator.h"
#include "Serial1_Listener.h"
#include "MyPersistentData.h"						  // Persistent Storage

Asset_Communicator *Asset_Communicator::_instance;

// [static]
Asset_Communicator &Asset_Communicator::instance() {
    if (!_instance) {
        _instance = new Asset_Communicator();
    }
    return *_instance;
}

Asset_Communicator::Asset_Communicator() {
}

Asset_Communicator::~Asset_Communicator() {
}

void Asset_Communicator::setup() {
    Serial1_Listener::instance().setup();    // Initialize the Serial1_Listener
    waitFor(Serial1.available, 1000);
    /** Initialize other listeners here if needed **/
    Asset_Communicator::instance().checkIfSensorTypeNeedsUpdate();  // We need to check if the asset has been changed without the Boron's knowledge
    sysStatus.set_assetFirmwareRelease(Asset_Communicator::instance().retrieveAssetFirmwareVersion());
}

void Asset_Communicator::loop() {                        
}

void Asset_Communicator::sendMessage(String message) {     // This function will send a message to the asset described in sysStatus.sensorType
    switch(sysStatus.get_sensorType()) {                         // Perform different tasks based on sensorType
        case 0: {                                                /*** Pressure Sensor ***/
            // Send a message to a Pressure Sensor here if needed
        } break;
        case 1:	{												                         /*** PIR Sensor ***/
            // Send a message to a PIR Sensor here if needed
        } break;
        case 2: {												                         /*** Magnetometer Sensor ***/
            Serial1.println(message + " \n");
        } break;
        case 3: {												                         /*** Accelerometer Sensor ***/
            // Send a message to an Accelerometer Sensor here if needed 
        } break;
        default: 
            Log.info("Failed to send message - Asset_Communicator can not determine which asset is connected.");
    }
}

bool Asset_Communicator::receiveMessage(char *response, int responseSize) {     // This function will receive a message from the asset defined in sysStatus.sensorType
    switch(sysStatus.get_sensorType()) {                         // Perform different tasks based on sensorType
        case 0: {                                                /*** Pressure Sensor ***/
            // Send a message to a Pressure Sensor here if needed
            Log.info("Failed to send message - Asset_Communicator could not receive a message from the Pressure Sensor."); 
            return false;
        } break;
        case 1:	{												                         /*** PIR Sensor ***/
            // Send a message to a PIR Sensor here if needed
            Log.info("Failed to send message - Asset_Communicator could not receive a message from the PIR Sensor."); 
            return false;
        } break;
        case 2: {												                         /*** Magnetometer Sensor ***/
            return Serial1_Listener::instance().getResponse(response, responseSize);
        } break;
        case 3: {												                         /*** Accelerometer Sensor ***/
            // Send a message to an Accelerometer Sensor here if needed
            Log.info("Failed to send message - Asset_Communicator could not receive a message from the Accelerometer."); 
            return false;
        } break;
        default: 
            Log.info("Failed to send message - Asset_Communicator can not determine which asset is connected.");
            return false;
    }
}

String Asset_Communicator::retrieveAssetFirmwareVersion() {
  char version[32];
  switch(sysStatus.get_sensorType()) {
    case 0: {                                                    /*** Pressure Sensor ***/
      // set or retrieve pressure sensor's firmware version here
      return "0.0";
    } break;
    case 1:	{												                             /*** PIR Sensor ***/
      // set or retrieve PIR sensor's firmware version here
      return "0.0";
    } break;
    case 2: {												                             /*** Magnetometer Sensor ***/								     
      Asset_Communicator::instance().sendMessage("*VER?");						 									// Query device for its Version	
      if (Asset_Communicator::instance().receiveMessage(version, sizeof(version))) {   	// Check if there is a response from the Serial1 device
        Log.info("AssetFirmwareVersion retrieved: %s", version);       						      // Return the response from the Serial1 device
        return version;
      } else {
        Log.info("No Response from Serial1. Could not retrieve asset firmware version.");
        return "0.0";
      }
    } break;
    case 3: {												                             /*** Accelerometer Sensor ***/
        // set or retrieve accelerometer sensor's firmware version here 
        return "0.0";
    } break;
    default:
      return "0.0";                                  		         // Default to 0.0
  }
}

void Asset_Communicator::checkIfSensorTypeNeedsUpdate() {
  char response[256];
  /* Check if we need to update our type to 2 (Magnetometer) */
  if(sysStatus.get_sensorType() != 2){											// Execute a response check on the serial line ONLY if the device is not already a Magnetometer
    Serial1.println("*IDN? \n");						                // Query device for identification using Serial1 print when checking if an update is
    if(Serial1_Listener::instance().getResponse(response, sizeof(response))){	// If we returned something ... 
      sysStatus.set_sensorType(2);						 						                          // ... take note that we are a magnetometer now by setting sysStatus.sensorType.		
      Log.info("Response from Serial. Setting sensor type to \"Magnetometer\"");
      Particle.publish("Magnetometer Sensor Detected. Setting Sensor Type.", "2 (Magnetometer)", PRIVATE);
    } else {
      Log.info("No Response from Serial. Not changing sensor type.");
    }
  } else {
	  Log.info("Sensor type is up to date! sensorType = %i", sysStatus.get_sensorType());
  }
  /* Check if we need to update to a different type below if needed */
}

void Asset_Communicator::performAssetFactoryReset() {
  char response[32];
  switch(sysStatus.get_sensorType()) {
    case 0: {                                                     /*** Pressure Sensor ***/
		// Execute a factory reset on the Pressure Sensor here if needed
    } break;
    case 1:	{												                              /*** PIR Sensor ***/
		// Execute a factory reset on the PIR Sensor here if needed
	  } break;
    case 2: {												                              /*** Magnetometer Sensor ***/
		Log.info("Performing a factory reset on the Magnetometer ..."); 
		Asset_Communicator::instance().sendMessage("*RESET");						    // Query device to begin factory reset	
		if(Asset_Communicator::instance().receiveMessage(response, sizeof(response))){	// If we returned something ... 
			Log.info("Factory reset completed! Response from device: %s", response);
		} else {
			Log.info("No Response from Serial1. Did not perform a factory reset.");
		}								 
    } break;
    case 3: {												                              /*** Accelerometer Sensor ***/
      	// Execute a factory reset on the Accelerometer Sensor here if needed 
    } break;
    default: 
      Log.info("Could not determine which asset is connected.");
  }
}
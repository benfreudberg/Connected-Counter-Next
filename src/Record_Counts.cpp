
//Particle Functions
#include "Particle.h"
#include "PublishQueuePosixRK.h"
#include "MyPersistentData.h"
#include "Record_Counts.h"
#include "Particle_Functions.h"

Record_Counts *Record_Counts::_instance;

// [static]
Record_Counts &Record_Counts::instance() {
  if (!_instance) {
      _instance = new Record_Counts();
  }
  return *_instance;
}

Record_Counts::Record_Counts() {
}

Record_Counts::~Record_Counts() {
}

void Record_Counts::setup() {
}

void Record_Counts::loop() {
    // Put your code to run during the application thread loop here
}

bool Record_Counts::recordCounts() // This is where we check to see if an interrupt is set when not asleep or act on a tap that woke the device
{
  bool doesItCount = false;                               //Whether or not each count should be recorded depends on the sensor and the settings

  switch(sysStatus.get_sensorType()) {
    case 0: {                                                // Pressure Sensor - only count the front tire
      static bool frontTire = true;                          // Keep track of which tire we are counting
      if (!frontTire) {                                      // We want to count the back tire 
        doesItCount = true;                                  // Then we should count it
        frontTire = true;                                    // And then set the flag to true as the next tire will be a front
      } else if (frontTire) {                                // Thie is a front tire
        doesItCount = false;                                 // Then we should not count it
        frontTire = false;                                   // And then set the flag to true so we count the front tire next time
      }
    } break;
    case 1:
      doesItCount = true;                                    // PIR Sensor - count all
      break;
    case 2: {
      if(Time.now() > (current.get_lastCountTime() + (time_t)sysStatus.get_delay())){ // Magnetometer Sensor - count if not within [delay] seconds of last count
          doesItCount = true;
      } else {
          doesItCount = false;                        
      }                                    
    } break;
    case 3: {
      doesItCount = true;                                    // Accelerometer Sensor - count all
      // Place holder for future code
    } break;
    default:
      doesItCount = false;                                   // Default to not counting
      break;  
  }

  if (doesItCount) {                                         // If we should count it
    char data[256]; 
    current.set_lastCountTime(Time.now());
    current.set_hourlyCount(current.get_hourlyCount()+1);    // Increment the PersonCount
    current.set_dailyCount(current.get_dailyCount() +1);     // Increment the PersonCount
    snprintf(data, sizeof(data), "Count, hourly: %i. daily: %i",current.get_hourlyCount(),current.get_dailyCount());
    Log.info(data);
    if (sysStatus.get_verboseMode() && Particle.connected()) {
      waitUntil(Particle_Functions::instance().meterParticlePublish);          // Keep within publish rate limit
      Particle.publish("Count",data, PRIVATE);               // Helpful for monitoring and calibration
    }
  }

  return doesItCount;
}
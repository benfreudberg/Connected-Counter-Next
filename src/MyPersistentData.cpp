#include "Particle.h"
#include "StorageHelperRK.h"
#include "MyPersistentData.h"

// *******************  SysStatus Storage Object **********************
//
// ********************************************************************

const char *persistentDataPathSystem = "/usr/sysStatus.dat";

sysStatusData *sysStatusData::_instance;

// [static]
sysStatusData &sysStatusData::instance() {
    if (!_instance) {
        _instance = new sysStatusData();
    }
    return *_instance;
}

sysStatusData::sysStatusData() : StorageHelperRK::PersistentDataFile(persistentDataPathSystem, &sysData.sysHeader, sizeof(SysData), SYS_DATA_MAGIC, SYS_DATA_VERSION) {

};

sysStatusData::~sysStatusData() {
}

void sysStatusData::setup() {
    sysStatus
    //  .withLogData(true)
        .withSaveDelayMs(100)
        .load();

    // Log.info("sizeof(SysData): %u", sizeof(SysData));
}

void sysStatusData::loop() {
    sysStatus.flush(false);
}

bool sysStatusData::validate(size_t dataSize) {
    bool valid = PersistentDataFile::validate(dataSize);
    if (valid) {
        // If test1 < 0 or test1 > 100, then the data is invalid

        if (sysStatus.get_openTime() < 0 || sysStatus.get_openTime() > 12) {
            Log.info("data not valid open time =%d", sysStatus.get_openTime());
            valid = false;
        }
        else if (sysStatus.get_lastConnection() < 0 || sysStatus.get_lastConnectionDuration() > 900) {
            Log.info("data not valid last connection duration =%d", sysStatus.get_lastConnectionDuration());
            valid = false;
        }
    }
    Log.info("sysStatus data is %s",(valid) ? "valid": "not valid");
    return valid;
}

void sysStatusData::initialize() {
    PersistentDataFile::initialize();

    const char message[26] = "Loading System Defaults";
    Log.info(message);
    if (Particle.connected()) Particle.publish("Mode",message, PRIVATE);
    Log.info("Loading system defaults");
    sysStatus.set_structuresVersion(1);
    sysStatus.set_verboseMode(true);
    sysStatus.set_lowBatteryMode(false);
    sysStatus.set_solarPowerMode(true);
    sysStatus.set_lowPowerMode(false);          // This should be changed to true once we have tested
    sysStatus.set_timeZoneStr("ANAT-12");       // NZ Time
    sysStatus.set_sensorType(2);                // Magnetometer sensor
    sysStatus.set_openTime(0);
    sysStatus.set_closeTime(24);                // New standard with v20
    sysStatus.set_lastConnectionDuration(0);    // New measure
    sysStatus.set_delay(2500);                  // delay(seconds) for pausing detections from Magnetometer interrupt line, default 2s
}

uint8_t sysStatusData::get_structuresVersion() const {
    return getValue<uint8_t>(offsetof(SysData, structuresVersion));
}

void sysStatusData::set_structuresVersion(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, structuresVersion), value);
}

bool sysStatusData::get_verboseMode() const {
    return getValue<bool>(offsetof(SysData,verboseMode));
}

void sysStatusData::set_verboseMode(bool value) {
    setValue<bool>(offsetof(SysData, verboseMode), value);
}

bool sysStatusData::get_solarPowerMode() const  {
    return getValue<bool>(offsetof(SysData,solarPowerMode ));
}
void sysStatusData::set_solarPowerMode(bool value) {
    setValue<bool>(offsetof(SysData, solarPowerMode), value);
}

bool sysStatusData::get_lowPowerMode() const  {
    return getValue<bool>(offsetof(SysData,lowPowerMode ));
}
void sysStatusData::set_lowPowerMode(bool value) {
    setValue<bool>(offsetof(SysData, lowPowerMode), value);
}

bool sysStatusData::get_lowBatteryMode() const  {
    return getValue<bool>(offsetof(SysData, lowBatteryMode));
}
void sysStatusData::set_lowBatteryMode(bool value) {
    setValue<bool>(offsetof(SysData, lowBatteryMode), value);
}

uint8_t sysStatusData::get_resetCount() const  {
    return getValue<uint8_t>(offsetof(SysData,resetCount));
}
void sysStatusData::set_resetCount(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, resetCount), value);
}

String sysStatusData::get_timeZoneStr() const {
	String result;
	getValueString(offsetof(SysData, timeZoneStr), sizeof(SysData::timeZoneStr), result);
	return result;
}

bool sysStatusData::set_timeZoneStr(const char *str) {
	return setValueString(offsetof(SysData, timeZoneStr), sizeof(SysData::timeZoneStr), str);
}

uint8_t sysStatusData::get_openTime() const  {
    return getValue<uint8_t>(offsetof(SysData,openTime));
}
void sysStatusData::set_openTime(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, openTime), value);
}

uint8_t sysStatusData::get_closeTime() const  {
    return getValue<uint8_t>(offsetof(SysData,closeTime));
}
void sysStatusData::set_closeTime(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, closeTime), value);
}

time_t sysStatusData::get_lastReport() const  {
    return getValue<time_t>(offsetof(SysData,lastReport));
}
void sysStatusData::set_lastReport(time_t value) {
    setValue<time_t>(offsetof(SysData, lastReport), value);
}

time_t sysStatusData::get_lastConnection() const  {
    return getValue<time_t>(offsetof(SysData,lastConnection));
}
void sysStatusData::set_lastConnection(time_t value) {
    setValue<time_t>(offsetof(SysData, lastConnection), value);
}

uint16_t sysStatusData::get_lastConnectionDuration() const  {
    return getValue<uint16_t>(offsetof(SysData,lastConnectionDuration));
}
void sysStatusData::set_lastConnectionDuration(uint16_t value) {
    setValue<uint16_t>(offsetof(SysData, lastConnectionDuration), value);
}

time_t sysStatusData::get_lastHookResponse() const  {
    return getValue<time_t>(offsetof(SysData,lastHookResponse));
}
void sysStatusData::set_lastHookResponse(time_t value) {
    setValue<time_t>(offsetof(SysData, lastHookResponse), value);
}

uint8_t sysStatusData::get_sensorType() const  {
    return getValue<uint8_t>(offsetof(SysData,sensorType));
}
void sysStatusData::set_sensorType(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, sensorType), value);
}

String sysStatusData::get_firmwareRelease() const {
    String value;
    getValueString(offsetof(SysData,firmwareRelease), sizeof(value), value);
    return value;

}

void sysStatusData::set_firmwareRelease(String value) {
    setValueString(offsetof(SysData, firmwareRelease), sizeof(value), value);
}

String sysStatusData::get_assetFirmwareRelease() const {
    String value;
    getValueString(offsetof(SysData,assetFirmwareRelease), sizeof(value), value);
    return value;
}

void sysStatusData::set_assetFirmwareRelease(String value) {
    setValueString(offsetof(SysData, assetFirmwareRelease), sizeof(value), value);
}

uint16_t sysStatusData::get_delay() const  {
    return getValue<uint16_t>(offsetof(SysData,delay));
}

void sysStatusData::set_delay(uint16_t value) {
    setValue<uint16_t>(offsetof(SysData, delay), value);
}

// *****************  Current Status Storage Object *******************
// 
// ********************************************************************

const char *persistentDataPathCurrent = "/usr/current.dat";

currentStatusData *currentStatusData::_instance;

// [static]
currentStatusData &currentStatusData::instance() {
    if (!_instance) {
        _instance = new currentStatusData();
    }
    return *_instance;
}

currentStatusData::currentStatusData() : StorageHelperRK::PersistentDataFile(persistentDataPathCurrent, &currentData.currentHeader, sizeof(CurrentData), CURRENT_DATA_MAGIC, CURRENT_DATA_VERSION) {
};

currentStatusData::~currentStatusData() {
}

void currentStatusData::setup() {
    current
    //    .withLogData(true)
        .withSaveDelayMs(250)
        .load();
}

void currentStatusData::loop() {
    current.flush(false);
}

void currentStatusData::resetEverything() {                             // The device is waking up in a new day or is a new install
  current.set_lastCountTime(Time.now());
  sysStatus.set_resetCount(0);                                          // Reset the reset count as well
  current.set_hourlyCount(0);
  current.set_dailyCount(0);
  current.set_alertCode(0);
}


bool currentStatusData::validate(size_t dataSize) {
    return true;

};                      // This needs to be fleshed out

void currentStatusData::initialize() {
    PersistentDataFile::initialize();

    Log.info("Current Data Initialized");

    currentStatusData::resetEverything();

    // If you manually update fields here, be sure to update the hash
    updateHash();
}

uint16_t currentStatusData::get_hourlyCount() const {
    return getValue<uint16_t>(offsetof(CurrentData, hourlyCount));
}

void currentStatusData::set_hourlyCount(uint16_t value) {
    setValue<uint16_t>(offsetof(CurrentData,hourlyCount), value);
}

uint16_t currentStatusData::get_dailyCount() const {
    return getValue<uint16_t>(offsetof(CurrentData, dailyCount));
}

void currentStatusData::set_dailyCount(uint16_t value) {
    setValue<uint16_t>(offsetof(CurrentData,dailyCount), value);
}

time_t currentStatusData::get_lastCountTime() const {
    return getValue<time_t>(offsetof(CurrentData, lastCountTime));
}

void currentStatusData::set_lastCountTime(time_t value) {
    setValue<time_t>(offsetof(CurrentData, lastCountTime), value);
}

float currentStatusData::get_internalTempC() const {
    return getValue<float>(offsetof(CurrentData, internalTempC));
}

void currentStatusData::set_internalTempC(float value) {
    setValue<float>(offsetof(CurrentData, internalTempC), value);
}

int8_t currentStatusData::get_alertCode() const {
    return getValue<int8_t>(offsetof(CurrentData, alertCode));
}

void currentStatusData::set_alertCode(int8_t value) {
    setValue<int8_t>(offsetof(CurrentData, alertCode), value);
}

float currentStatusData::get_stateOfCharge() const  {
    return getValue<float>(offsetof(CurrentData,stateOfCharge));
}
void currentStatusData::set_stateOfCharge(float value) {
    setValue<float>(offsetof(CurrentData, stateOfCharge), value);
}

uint8_t currentStatusData::get_batteryState() const  {
    return getValue<uint8_t>(offsetof(CurrentData, batteryState));
}
void currentStatusData::set_batteryState(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, batteryState), value);
}

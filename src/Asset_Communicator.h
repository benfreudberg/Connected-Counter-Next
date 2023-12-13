#ifndef __ASSET_COMMUNICATOR_H
#define __ASSET_COMMUNICATOR_H

#include "Particle.h"

/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * Asset_Communicator::instance().setup();
 * 
 * From global application loop you must call:
 * Asset_Communicator::instance().loop();
 */
class Asset_Communicator {
public:
    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use Asset_Communicator::instance() to instantiate the singleton.
     */
    static Asset_Communicator &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use Asset_Communicator::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use Asset_Communicator::instance().loop();
     */
    void loop();

    /**
     * @brief Sends a message to the asset described in sysStatus.sensorType
     * 
     * You typically use Asset_Communicator::instance().sendMessage(char *message);
    */
    void sendMessage(String message); 

    /**
     * @brief This function will receive a message from the asset defined in sysStatus.sensorType
     * 
     * You typically use Asset_Communicator::instance().receiveMessage(char *response, int responseSize);
    */
    bool receiveMessage(char *response, int responseSize);

    /**
     * @brief retrieveAssetFirmwareVersion queries a sensor connected to this particle device,
     * asking for its version. Sets the the returned version in MyPersistentData.cpp
     * 
     * You typically use Asset_Communicator::instance().retrieveAssetFirmwareVersion();
     */
    String retrieveAssetFirmwareVersion();

    /**
     * @brief checkIfSensorTypeNeedsUpdate communicates with the attached asset to determine if sysStatus.sensorType needs to be updated. Updates if it needs one.
     * 
     * You typically use Asset_Communicator::instance().checkIfSensorTypeNeedsUpdate();
     */
    void checkIfSensorTypeNeedsUpdate();

    /**
     * @brief performAssetFactoryReset communicates with an attached asset and triggers a factory reset.
     * 
     * You typically use Asset_Communicator::instance().performAssetFactoryReset();
     */
    void performAssetFactoryReset();

protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use Asset_Communicator::instance() to instantiate the singleton.
     */
    Asset_Communicator();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~Asset_Communicator();

    /**
     * This class is a singleton and cannot be copied
     */
    Asset_Communicator(const Asset_Communicator&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    Asset_Communicator& operator=(const Asset_Communicator&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static Asset_Communicator *_instance;

};
#endif  /* __ASSET_COMMUNICATOR_H */

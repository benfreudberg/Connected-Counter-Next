#ifndef __SERIAL1_LISTENER_H
#define __SERIAL1_LISTENER_H

#include "Particle.h"

/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * Serial1-Listener::instance().setup();
 * 
 * From global application loop you must call:
 * Serial1-Listener::instance().loop();
 */
class Serial1_Listener {
public:
    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use Serial1-Listener::instance() to instantiate the singleton.
     */
    static Serial1_Listener &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use Serial1-Listener::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use Serial1-Listener::instance().loop();
     */
    void loop();

    /**
     * @brief Get the response from the Serial1 device
    */
    bool getResponse(char *response, int responseSize);


protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use Serial1-Listener::instance() to instantiate the singleton.
     */
    Serial1_Listener();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~Serial1_Listener();

    /**
     * This class is a singleton and cannot be copied
     */
    Serial1_Listener(const Serial1_Listener&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    Serial1_Listener& operator=(const Serial1_Listener&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static Serial1_Listener *_instance;

};
#endif  /* __SERIAL1_LISTENER_H */

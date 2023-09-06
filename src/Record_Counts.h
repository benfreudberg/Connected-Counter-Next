/*
 * @file Record_Counts.h
 * @author Chip McClelland (chip@seeinisghts.com)
 * @brief This function will manage the record counts for the device
 * 
 * @version 0.1
 * @date 2022-08-28
 * 
 */



#ifndef __RECORD_COUNTS_H
#define __RECORD_COUNTS_H

#include "Particle.h"


/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * Record_Counts::instance().setup();
 * 
 * From global application loop you must call:
 * Record_Counts::instance().loop();
 */
class Record_Counts {
public:
    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use Record_Counts::instance() to instantiate the singleton.
     */
    static Record_Counts &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use Record_Counts::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use Record_Counts::instance().loop();
     */
    void loop();

    /**
     * @brief This function is called once a hardware interrupt is triggered by the device's sensor
     * 
     * @details The sensor may change based on the settings in sysSettings but the overall concept of operations
     * is the same regardless.  The sensor will trigger an interrupt, which will set a flag. In the main loop
     * that flag will call this function which will determine if this event should "count" as a visitor.
     * 
     */
    bool recordCounts();                               // Determine if a count should be recorded

protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use Record_Counts::instance() to instantiate the singleton.
     */
    Record_Counts();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~Record_Counts();

    /**
     * This class is a singleton and cannot be copied
     */
    Record_Counts(const Record_Counts&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    Record_Counts& operator=(const Record_Counts&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static Record_Counts *_instance;

};
#endif  /* __Record_Counts_H */

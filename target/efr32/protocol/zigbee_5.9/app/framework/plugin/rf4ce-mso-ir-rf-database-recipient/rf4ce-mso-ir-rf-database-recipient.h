// Copyright 2014 Silicon Laboratories, Inc.

#ifndef __RF4CE_MSO_IR_RF_DATABASE_RECIPIENT_H__
#define __RF4CE_MSO_IR_RF_DATABASE_RECIPIENT_H__

/**
 * @addtogroup rf4ce-mso-ir-rf-database-recipient
 *
 * The RF4CE Multiple System Operators (MSO) IR-RF Database Recipient plugin
 * implements the optional IR-RF database feature of the MSO profile for
 * targets.  The IR-RF database provides a standard mechanism for remapping
 * keys on a remote to control legacy IR devices or to perform simulataneous
 * IR and RF functions.  This plugin manages the storage and retrieval of these
 * mappings for targets.
 *
 * When a controller queries this device for IR-RF information, the @ref
 * rf4ce-mso plugin will pass the request to this plugin via
 * ::emberAfPluginRf4ceMsoHaveIrRfDatabaseAttributeCallback and
 * ::emberAfPluginRf4ceMsoGetIrRfDatabaseAttributeCallback.  This plugin,
 * in turn, will provide the IR-RF information so that it may be sent back to
 * the controller.

 * This plugin only stores IR-RF database entries on behalf of the application.
 * The actual IR and RF descriptors themselves must be provided by the
 * application.  Entries can be added to the database by calling
 * ::emberAfRf4ceMsoIrRfDatabaseRecipientAdd with the key code to be remapped.
 * ::emberAfRf4ceMsoIrRfDatabaseRecipientRemove can be used to clear a specific
 * entry or ::emberAfRf4ceMsoIrRfDatabaseRecipientRemoveAll may be used to
 * clear all entries in the database.  Note that database entries are stored in
 * RAM.  The amount of storage space dedicated to storing IR-RF database
 * entries is configuratable via the plugin options.
 *
 * Support for the optional IR-RF database feature for controllers is provided
 * by the @ref rf4ce-mso-ir-rf-database-originator plugin.
 * @{
 */

/** @brief Add an IR-RF entry to the database for a key code.
 *
 * @param keyCode The key code.
 * @param entry A pointer to the ::EmberAfRf4ceMsoIrRfDatabaseEntry.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceMsoIrRfDatabaseRecipientAdd(EmberAfRf4ceMsoKeyCode keyCode,
                                                    EmberAfRf4ceMsoIrRfDatabaseEntry *entry);


/** @brief Remove an IR-RF entry from the database.
 *
 * @param keyCode The key code which entry is to be removed.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceMsoIrRfDatabaseRecipientRemove(EmberAfRf4ceMsoKeyCode keyCode);


/** @brief Clear all of the IR-RF entries from the database. */
void emberAfRf4ceMsoIrRfDatabaseRecipientRemoveAll(void);


#endif // __RF4CE_MSO_IR_RF_DATABASE_RECIPIENT_H__

// END addtogroup

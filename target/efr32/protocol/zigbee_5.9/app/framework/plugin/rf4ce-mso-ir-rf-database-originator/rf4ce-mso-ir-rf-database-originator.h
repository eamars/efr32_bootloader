// Copyright 2014 Silicon Laboratories, Inc.

#ifndef __RF4CE_MSO_IF_RF_DATABASE_ORIGINATOR_H__
#define __RF4CE_MSO_IF_RF_DATABASE_ORIGINATOR_H__

/**
 * @addtogroup rf4ce-mso-ir-rf-database-originator
 *
 * The RF4CE Multiple System Operators (MSO) IR-RF Database Originator plugin
 * implements the optional IR-RF database feature of the MSO profile for
 * controllers.  The IR-RF database provides a standard mechanism for remapping
 * keys on a remote to control legacy IR devices or to perform simulataneous
 * IR and RF functions.  This plugin manages the storage and retrieval of these
 * mappings for controllers.
 *
 * When the application queries the target for IR-RF information via the @ref
 * rf4ce-mso plugin, the data from the target are passed to this plugin via
 * ::emberAfPluginRf4ceMsoIncomingIrRfDatabaseAttributeCallback.  This plugin,
 * in turn, will store the data in RAM.  The keys that may be remapped are
 * configuration in AppBuilder and the amount of storage space dedicated to
 * storing IR-RF database entries is configuratable via the plugin options.
 *
 * When ::emberAfRf4ceMsoUserControlPress and
 * ::emberAfRf4ceMsoUserControlRelease are called, the @ref rf4ce-mso plugin
 * will ask this plugin to retrieve RF mappings for the key via
 * ::emberAfPluginRf4ceMsoGetIrRfDatabaseEntryCallback.  If a database entry
 * exists for the key, it will be used in lieu of the original key.  The same
 * callback may be used by the application to retrieve IR mappings for keys.
 *
 * Support for the optional IR-RF database feature for targets is provided by
 * the @ref rf4ce-mso-ir-rf-database-recipient plugin.
 * @{
 */

#ifdef DOXYGEN_SHOULD_SKIP_THIS
  /** @brief Get the IR-RF entry from the database for a key code.
   *
   * @param keyCode The key code.
   * @param entry A pointer to the ::EmberAfRf4ceMsoIrRfDatabaseEntry to be
   * populated.
   *
   * @return An ::EmberStatus value that indicates the success or failure of
   * the command.
   */
  EmberStatus emberAfRf4ceMsoIrRfDatabaseOriginatorGet(EmberAfRf4ceMsoKeyCode keyCode,
                                                       EmberAfRf4ceMsoIrRfDatabaseEntry *entry);
#else
  #define emberAfRf4ceMsoIrRfDatabaseOriginatorGet \
    emberAfPluginRf4ceMsoGetIrRfDatabaseEntryCallback
#endif

/** @brief Set the IR-RF entry in the database for a key code.
 *
 * @param keyCode The key code.
 * @param entry A pointer to the ::EmberAfRf4ceMsoIrRfDatabaseEntry.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceMsoIrRfDatabaseOriginatorSet(EmberAfRf4ceMsoKeyCode keyCode,
                                                     const EmberAfRf4ceMsoIrRfDatabaseEntry *entry);

/** @brief Clear the IR-RF entry from the database for a key code.
 *
 * @param keyCode The key code.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceMsoIrRfDatabaseOriginatorClear(EmberAfRf4ceMsoKeyCode keyCode);

/** @brief Clear all of the IR-RF entries from the database. */
void emberAfRf4ceMsoIrRfDatabaseOriginatorClearAll(void);

#endif /* __RF4CE_MSO_IF_RF_DATABASE_ORIGINATOR_H__ */

// END addtogroup

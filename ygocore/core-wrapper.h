#include "core/ocgapi.h"

namespace ny {

/**
 * the type `ptr` in ocgapi.h holds a pointer value, which is not
 * always suitable for v8's safe integer.
 * here we map each ptr (duel *) to a duel instance id (safe integer).
 */
using duel_instance_id_t = int32;

/**
 * register a duel ptr.
 * @return duel instance id
 */
duel_instance_id_t register_duel(ptr duel_ptr);


/**
 * query duel by instance id.
 */
ptr                query_duel(duel_instance_id_t duel_id);

/**
 * mark a duel as ended.
 */
void               delete_duel(duel_instance_id_t duel_id);

/**
 * add card definition.
 *
 * see ocgapi's `card_reader`, `set_card_reader`
 */
void               global_storage_register_card(card_data card);


/**
 * load script into global storage.
 */
void               global_storage_register_script( const char *script_name
                                                 , const char *script_content);


void               initialize_global_storage();

} // namespace ny

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
void               add_static_card_data(card_data definition);

/**
 * set script path.
 *
 * see ocgapi's `script_reader`, `set_script_reader`
 */
void               set_script_directory(const char *path);

} // namespace ny

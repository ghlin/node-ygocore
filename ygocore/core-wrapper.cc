#include "core-wrapper.h"
#include "core/card.h"
#include <map>
#include <string>
#include <stack>

namespace ny {

class CoreWrapper
{
  std::map<std::string, std::string> script_by_name;
  std::map<uint32, card_data>        card_by_code;
  std::map<duel_instance_id_t, ptr>  duel_by_id;

  std::string                        script_directory;
  std::stack<duel_instance_id_t>     reusable_id_list;
  duel_instance_id_t                 last_id = 1; ///> starts from 1.

  duel_instance_id_t acquire_id()
  {
    if (reusable_id_list.empty())
      return last_id++;

    const auto id = reusable_id_list.top();
    reusable_id_list.pop();

    return id;
  }

  void collect_id(duel_instance_id_t reusable)
  {
    reusable_id_list.push(reusable);
  }
public:
  ptr    query_duel(duel_instance_id_t duel_id) const
  {
    const auto found = duel_by_id.find(duel_id);
    if (found == duel_by_id.cend())
      return 0;
    else
      return found->second;
  }

  duel_instance_id_t register_duel(ptr duel_ptr)
  {
    const auto id = acquire_id();
    duel_by_id[id] = duel_ptr;

    return id;
  }

  void   delete_duel(duel_instance_id_t id)
  {
    duel_by_id.erase(id);
    collect_id(id);
  }

  void add_static_card_data(card_data definition)
  {
    card_by_code[definition.code] = definition;
  }

  void   set_script_directory(const char *path)
  {
    script_directory = path;
  }
};

static CoreWrapper the_wrapper;

duel_instance_id_t register_duel(ptr duel_ptr)
{
  return the_wrapper.register_duel(duel_ptr);
}

ptr                query_duel(duel_instance_id_t id)
{
  return the_wrapper.query_duel(id);
}

void               delete_duel(duel_instance_id_t id)
{
  the_wrapper.delete_duel(id);
}

void               set_script_directory(const char *path)
{
  the_wrapper.set_script_directory(path);
}

void               add_static_card_data(card_data definition)
{
  the_wrapper.add_static_card_data(definition);
}

} // namespace ny

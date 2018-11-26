#include "core-wrapper.h"
#include "core/card.h"
#include <map>
#include <string>
#include <cstring>
#include <vector>
#include <stack>

namespace ny {

struct Storage
{
  std::map<std::string, std::vector<byte>> script_content_by_name;
  std::map<uint32, card_data>              card_data_by_code;
  std::map<duel_instance_id_t, ptr>        duel_by_id;
  std::stack<duel_instance_id_t>           reusable_id_list;

  duel_instance_id_t                       last_id = 1; ///> starts from 1.

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

  void   register_card(card_data definition)
  {
    card_data_by_code[definition.code] = definition;
    std::fprintf(stderr, "register_card: %d\n", definition.code);
  }

  void   register_script( const char *script_name
                        , const char *script_content)
  {
    const auto len = std::strlen(script_content);
    script_content_by_name[script_name] =
      std::vector<byte>( script_content
                       , script_content + len);
    std::fprintf(stderr, "register_script: %s\n", script_name);
  }
};

static Storage global_storage;

duel_instance_id_t register_duel(ptr duel_ptr)
{
  return global_storage.register_duel(duel_ptr);
}

ptr                query_duel(duel_instance_id_t id)
{
  return global_storage.query_duel(id);
}

void               delete_duel(duel_instance_id_t id)
{
  global_storage.delete_duel(id);
}

void               global_storage_register_card(card_data definition)
{
  global_storage.register_card(definition);
}

void               global_storage_register_script( const char *script_name
                                                 , const char *script_content)
{
  global_storage.register_script(script_name, script_content);
}

static
uint32 read_card_from_global_storage(uint32 code, card_data *data)
{
  const auto found = global_storage.card_data_by_code.find(code);
  if (found == global_storage.card_data_by_code.cend()) {
    std::fprintf(stderr, "read_card: card %u not found.\n", code);
    return 1;
  }

  *data = found->second;
  return 0;
}

static
byte *try_script( const char *script_name
                , int        *script_len)
{
  const auto found = global_storage.script_content_by_name.find(script_name);
  if (found == global_storage.script_content_by_name.cend())
    return nullptr;

  *script_len = found->second.size();

  // ocgcore won't actually modify the buffer.
  // hope so.
  return found->second.data();
}

static
byte *read_script_from_global_storage( const char *script_name
                                     , int        *script_len)
{
  const char *trials[6];
  const char **trial = trials;

  *trial++ = script_name;

  if (auto found = try_script(script_name, script_len)) {
    return found;
  }

  for (const char *probe_script_name = script_name; *probe_script_name; ++probe_script_name) {
    if (probe_script_name[0] != '/')
      continue;

    *trial++ = probe_script_name + 1;
    if (auto found = try_script(probe_script_name + 1, script_len)) {
      return found;
    }
  }

  std::fprintf( stderr
              , "read_script: script %s not found.\n"
              , script_name);
  for (const auto *trial_p = trials; trial_p != trial; ++trial_p) {
    std::fprintf( stderr
                , "               - tried: %s\n"
                , *trial_p);
  }

  *script_len = 0;
  static byte dummy_buffer[2] = { 0, 0 };
  return dummy_buffer;
}

void               initialize_global_storage()
{
  set_card_reader(read_card_from_global_storage);
  set_script_reader(read_script_from_global_storage);
}

} // namespace ny

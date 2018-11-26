#include "core-wrapper.h"
#include "core/card.h"
#include <nan.h>
#include <cstdio>
#include <string>

namespace ny {

const char *to_c_string(const v8::String::Utf8Value &value)
{
  return *value;
}

NAN_METHOD(setScriptDirectory)
{
  const auto arg0 = info[0];
  if (arg0.IsEmpty() || !arg0->IsString()) {
    return Nan::ThrowTypeError("String expected");
  }

  v8::String::Utf8Value value(arg0);
  const auto path = to_c_string(value);

  if (!path) {
    return Nan::ThrowError("Internal error: Cannot convert arg0 to c string");
  }
}

#define CHECK_ARG(n, type)                                                    \
  const auto arg##n = info[n];                                                \
  do { if (!arg##n->Is##type()) {                                             \
    char buff[200];                                                           \
    std::sprintf(buff, "%s: argument #%d, %s expected.", __func__, n, #type); \
    return Nan::ThrowTypeError(buff);                                         \
  } } while (false)

NAN_METHOD(registerScript)
{
  CHECK_ARG(0, String);
  CHECK_ARG(1, String);

  v8::String::Utf8Value hold_script_name(arg0);
  v8::String::Utf8Value hold_script_content(arg1);

  const auto script_name    = to_c_string(hold_script_name);
  const auto script_content = to_c_string(hold_script_content);

  global_storage_register_script(script_name, script_content);
}

#define GET_PROP_FROM(obj, field, v8Type, type)                          \
  const auto ref_##field = obj->Get(Nan::New(#field).ToLocalChecked());  \
  if (ref_##field.IsEmpty() || !ref_##field->Is##v8Type()) {             \
    return Nan::ThrowTypeError(                                          \
      "registerCard: Missing property "                                  \
      "" #field                                                          \
      ", or type mismatch ("                                             \
      "" #v8Type                                                         \
      " expected)");                                                     \
  }                                                                      \
  const auto _value_##field = ref_##field.As<v8::v8Type>();              \
  const auto field = static_cast<type>(_value_##field->Value())

NAN_METHOD(registerCard)
{
  CHECK_ARG(0, Object);
  const auto card = arg0.As<v8::Object>();

#define GET_PROP(key, type) GET_PROP_FROM(card, key, Int32, type)
  GET_PROP(code,        uint32);
  GET_PROP(alias,       uint32);
  GET_PROP(type,        uint32);
  GET_PROP(level,       uint32);
  GET_PROP(attribute,   uint32);
  GET_PROP(race,        uint32);
  GET_PROP(attack,      int32);
  GET_PROP(defense,     int32);
  GET_PROP(lscale,      uint32);
  GET_PROP(rscale,      uint32);
  GET_PROP(link_marker, uint32);

  // setcode is 64-bit, doesn't fit in a safe-integer
  GET_PROP(setcode_high, uint32);
  GET_PROP(setcode_low,  uint32);
#undef GET_PROP

  const auto setcode = (static_cast<uint64>(setcode_high) << 32) + setcode_low;

  global_storage_register_card(
    { code
    , alias
    , setcode
    , type
    , level
    , attribute
    , race
    , attack
    , defense
    , lscale
    , rscale
    , link_marker
    });
}

NAN_METHOD(initializeEngine)
{
  initialize_global_storage();
}

NAN_METHOD(createDuel)
{
  CHECK_ARG(0, Int32);
  const auto seed = static_cast<uint32>(arg0.As<v8::Int32>()->Value());
  const auto duel = create_duel(seed);
  const auto id   = register_duel(duel);

  info.GetReturnValue().Set(id);
}

#define CHECK_INT(n, name, type)                        \
  CHECK_ARG(n, Int32);                                  \
  const auto name =                                     \
    static_cast<type>(arg##n.As<v8::Int32>()->Value())

#define CHECK_DUEL(n)                           \
  CHECK_INT(n, duel_id, duel_instance_id_t);    \
  const auto duel = query_duel(duel_id);        \
  do { if (!duel) {                             \
    return Nan::ThrowError("Invalid duel id");  \
  } } while (false)

NAN_METHOD(startDuel)
{
  CHECK_DUEL(0);
  CHECK_INT(1, options, int32);

  start_duel(duel, options);
}

NAN_METHOD(endDuel)
{
  CHECK_DUEL(0);

  end_duel(duel);
}

NAN_METHOD(setPlayerInfo)
{
  CHECK_DUEL(0);
  CHECK_ARG(1, Object);

  const auto obj = arg1.As<v8::Object>();

  GET_PROP_FROM(obj, player, Int32, int32);
  GET_PROP_FROM(obj, lp,     Int32, int32);
  GET_PROP_FROM(obj, start,  Int32, int32);
  GET_PROP_FROM(obj, hand,   Int32, int32);

  set_player_info(duel, player, lp, start, hand);
}

NAN_METHOD(process)
{
  CHECK_DUEL(0);

  const auto process_result = ::process(duel);
  const auto message_length = process_result & 0xFFFF;
  const auto process_flags  = process_result >> 16;

  byte buff[0x1000];
  get_message(duel, buff);

  auto buffer_obj = Nan::CopyBuffer((char *)buff, message_length).ToLocalChecked();
  auto result_obj = Nan::New<v8::Object>();

  result_obj->Set( Nan::New("flags").ToLocalChecked()
                 , Nan::New(process_flags));
  result_obj->Set( Nan::New("messages").ToLocalChecked()
                 , buffer_obj);

  info.GetReturnValue().Set(result_obj);
}

NAN_METHOD(newCard)
{
  CHECK_DUEL(0);
  CHECK_ARG(1, Object);

  const auto new_card = arg1.As<v8::Object>();

  GET_PROP_FROM(new_card, code,     Int32, uint32);
  GET_PROP_FROM(new_card, owner,    Int32, uint8);
  GET_PROP_FROM(new_card, player,   Int32, uint8);
  GET_PROP_FROM(new_card, location, Int32, uint8);
  GET_PROP_FROM(new_card, sequence, Int32, uint8);
  GET_PROP_FROM(new_card, position, Int32, uint8);

  ::new_card(duel, code, owner, player, location, sequence, position);
}

NAN_METHOD(setResponse)
{
  CHECK_DUEL(0);

  const auto arg1 = info[1];
  if (arg1.IsEmpty() || !arg1->IsArrayBuffer()) {
    return Nan::ThrowTypeError("array buffer expected");
  }

  const auto response_obj     = arg1.As<v8::ArrayBuffer>();
  const auto response_content = response_obj->GetContents();
  const auto response_data    = response_content.Data();
  const auto response_length  = response_content.ByteLength();

  // ygopro-core always reads 64-bytes data at once.
  byte response_buffer[64];
  std::memcpy(response_buffer, response_data, response_length);

  set_responseb(duel, response_buffer);
}

NAN_MODULE_INIT(Init)
{
  NAN_EXPORT(target, initializeEngine);
  NAN_EXPORT(target, registerCard);
  NAN_EXPORT(target, registerScript);
  NAN_EXPORT(target, createDuel);
  NAN_EXPORT(target, startDuel);
  NAN_EXPORT(target, endDuel);
  NAN_EXPORT(target, setPlayerInfo);
  NAN_EXPORT(target, process);
  NAN_EXPORT(target, newCard);
  NAN_EXPORT(target, setResponse);
}

NODE_MODULE(ocgcore, Init)

} // namespace ny

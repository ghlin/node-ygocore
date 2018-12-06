#include "wrapper.h"
#include "core/card.h"
#include "core/mtrandom.h"
#include <nan.h>
#include <cstdio>
#include <string>

namespace ny {

const char *to_c_string(const v8::String::Utf8Value &value)
{
  return *value;
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
  GET_PROP(linkMarker,  uint32);

  uint64 setcode;

  // setcode is 64-bit, doesn't fit in a safe-integer
  const auto setcode_property = card->Get(Nan::New("setcode").ToLocalChecked());
  if (setcode_property.IsEmpty()) {
    return Nan::ThrowTypeError("Missing property 'setcode'");
  }

  if (setcode_property->IsString()) {
    // setcode is provided as a string
    const auto setcode_value = setcode_property.As<v8::String>();
    v8::String::Utf8Value hold_value(setcode_value);

    setcode = std::strtoll(to_c_string(hold_value), nullptr, 10);
  } else if (setcode_property->IsObject()) {
    // setcode is provided as two 32-bits integers
    const auto setcode_object = setcode_property.As<v8::Object>();

    GET_PROP_FROM(setcode_object, high, Int32, uint32);
    GET_PROP_FROM(setcode_object, low,  Int32, uint32);

    setcode = (static_cast<uint64>(high) << 32) + low;
  } else {
    return Nan::ThrowTypeError("Property 'setcode' should be either a string or { low: number, high: number }");
  }
#undef GET_PROP

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
    , linkMarker
    });
}

NAN_METHOD(createDuel)
{
  CHECK_ARG(0, Int32);
  const auto seed = static_cast<uint32>(arg0.As<v8::Int32>()->Value());

  const auto duel = create_duel(seed);
  const auto id   = register_duel(duel);

  info.GetReturnValue().Set(id);
}

NAN_METHOD(createYgoproReplayDuel)
{
  CHECK_ARG(0, Int32);
  const auto seed = static_cast<uint32>(arg0.As<v8::Int32>()->Value());

  // use a random seed to generate another random seed.
  // taken from ygopro's code.
  mtrandom rnd; rnd.reset(seed);
  const auto real_seed = rnd.rand();

  const auto duel = create_duel(real_seed);
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
  GET_PROP_FROM(obj, draw,   Int32, int32);

  set_player_info(duel, player, lp, start, draw);
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
  result_obj->Set( Nan::New("data").ToLocalChecked()
                 , buffer_obj);

  info.GetReturnValue().Set(result_obj);
}

NAN_METHOD(queryCard)
{
  CHECK_DUEL(0);
  CHECK_ARG(1, Object);

  const auto queryCardOptions = arg1.As<v8::Object>();

  GET_PROP_FROM(queryCardOptions, player,   Int32, uint32);
  GET_PROP_FROM(queryCardOptions, location, Int32, uint32);
  GET_PROP_FROM(queryCardOptions, queryFlags,    Int32, uint32);
  GET_PROP_FROM(queryCardOptions, useCache, Boolean, bool);
  GET_PROP_FROM(queryCardOptions, sequence, Int32, uint32);

  byte query_buffer[0x4000];
  auto length = query_card(duel, player, location, sequence, queryFlags, query_buffer, useCache);

  info.GetReturnValue().Set(Nan::CopyBuffer((char *)query_buffer, length).ToLocalChecked());
}

NAN_METHOD(queryFieldCard)
{
  CHECK_DUEL(0);
  CHECK_ARG(1, Object);

  const auto queryOptions = arg1.As<v8::Object>();

  GET_PROP_FROM(queryOptions, player,   Int32, uint32);
  GET_PROP_FROM(queryOptions, location, Int32, uint32);
  GET_PROP_FROM(queryOptions, queryFlags,    Int32, uint32);
  GET_PROP_FROM(queryOptions, useCache, Boolean, bool);

  byte query_buffer[0x4000];
  auto length = query_field_card(duel, player, location, queryFlags, query_buffer, useCache);

  info.GetReturnValue().Set(Nan::CopyBuffer((char *)query_buffer, length).ToLocalChecked());
}

NAN_METHOD(queryFieldCount)
{
  CHECK_DUEL(0);
  CHECK_INT(1, player, uint32);
  CHECK_INT(2, location, uint32);

  auto count = query_field_count(duel, player, location);

  info.GetReturnValue().Set(Nan::New(count));
}

NAN_METHOD(queryFieldInfo)
{
  CHECK_DUEL(0);

  return Nan::ThrowError("not impl!");
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

  if (response_length > 64) {
    // non-sense response
    const char *error_message = "response buffer is too large (> 64 bytes)";
    return Nan::ThrowError(Nan::New(error_message).ToLocalChecked());
  }

  // ygopro-core always reads 64-bytes data at once.
  byte response_buffer[64];
  std::memcpy(response_buffer, response_data, response_length);

  set_responseb(duel, response_buffer);
}

NAN_MODULE_INIT(Init)
{
  NAN_EXPORT(target, registerCard);
  NAN_EXPORT(target, registerScript);
  NAN_EXPORT(target, createDuel);
  NAN_EXPORT(target, createYgoproReplayDuel);
  NAN_EXPORT(target, startDuel);
  NAN_EXPORT(target, endDuel);
  NAN_EXPORT(target, setPlayerInfo);
  NAN_EXPORT(target, process);
  NAN_EXPORT(target, newCard);
  NAN_EXPORT(target, setResponse);
  NAN_EXPORT(target, queryCard);
  NAN_EXPORT(target, queryFieldCard);
  NAN_EXPORT(target, queryFieldCount);
  NAN_EXPORT(target, queryFieldInfo);

  // setup script reader & card reader
  initialize_global_storage();
}

NODE_MODULE(ocgcore, Init)

} // namespace ny

#include "wrapper.h"
#include "core/card.h"
#include "core/mtrandom.h"
#include <nan.h>
#include <cstdio>
#include <string>

namespace ny {

static inline
const char *to_c_string(const v8::String::Utf8Value &value)
{
  return *value;
}

template <typename I>
static inline
I to_integer(v8::Handle<v8::Value> val, I default_value)
{
  if (val->IsInt32()) {
    return static_cast<I>(val->Int32Value());
  }
  if (val->IsUint32()) {
    return static_cast<I>(val->Uint32Value());
  }
  if (val->IsNumber()) {
    return static_cast<I>(val->IntegerValue());
  }

  return default_value;
}

#define CHECK_ARG(n, type)                                                    \
  const auto arg##n = info[n];                                                \
  do { if (!arg##n->Is##type()) {                                             \
    char buff[200];                                                           \
    std::sprintf(buff, "%s: argument #%d, %s expected.", __func__, n, #type); \
    return Nan::ThrowTypeError(buff);                                         \
  } } while (false)

#define GET_PROP_OF_TYPE(obj, field, v8Type)                             \
  const auto ref_##field = obj->Get(Nan::New(#field).ToLocalChecked());  \
  if (ref_##field.IsEmpty() || !ref_##field->Is##v8Type()) {             \
    return Nan::ThrowTypeError(                                          \
      "missing property "                                                \
      "'" #field                                                         \
      "', or type mismatch ("                                            \
      "" #v8Type                                                         \
      " expected)");                                                     \
  }

#define GET_INTEGER_PROP(obj, field, type) \
  GET_PROP_OF_TYPE(obj, field, Number)     \
  const auto field = to_integer<type>(ref_##field, 0)

#define GET_PROP(obj, field, v8Type)                          \
  GET_PROP_OF_TYPE(obj, field, v8Type)                        \
  const auto field = ref_##field->v8Type##Value()

#define CHECK_INT(n, name, type)                        \
  CHECK_ARG(n, Number);                                 \
  const auto name = to_integer<type>(info[n], 0)        \

#define CHECK_DUEL(n)                           \
  CHECK_INT(n, duel_id, duel_instance_id_t);    \
  const auto duel = query_duel(duel_id);        \
  do { if (!duel) {                             \
    return Nan::ThrowError("Invalid duel id");  \
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


NAN_METHOD(registerCard)
{
  CHECK_ARG(0, Object);
  const auto card = arg0.As<v8::Object>();

  GET_INTEGER_PROP(card, code,       uint32);
  GET_INTEGER_PROP(card, alias,      uint32);
  GET_INTEGER_PROP(card, type,       uint32);
  GET_INTEGER_PROP(card, level,      uint32);
  GET_INTEGER_PROP(card, attribute,  uint32);
  GET_INTEGER_PROP(card, race,       uint32);
  GET_INTEGER_PROP(card, attack,     int32);
  GET_INTEGER_PROP(card, defense,    int32);
  GET_INTEGER_PROP(card, lscale,     uint32);
  GET_INTEGER_PROP(card, rscale,     uint32);
  GET_INTEGER_PROP(card, linkMarker, uint32);

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

    GET_INTEGER_PROP(setcode_object, high, uint32);
    GET_INTEGER_PROP(setcode_object, low,  uint32);

    setcode = (static_cast<uint64>(high) << 32) + low;
  } else {
    return Nan::ThrowTypeError("Property 'setcode' should be either a string or { low: number, high: number }");
  }

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
  CHECK_INT(0, seed, uint32);

  const auto duel = create_duel(seed);
  const auto id   = register_duel(duel);

  info.GetReturnValue().Set(id);
}

NAN_METHOD(createYgoproReplayDuel)
{
  CHECK_INT(0, seed, uint32);

  // use a random seed to generate another random seed.
  // taken from ygopro's code.
  mtrandom rnd; rnd.reset(seed);
  const auto real_seed = rnd.rand();

  const auto duel = create_duel(real_seed);
  const auto id   = register_duel(duel);

  info.GetReturnValue().Set(id);
}

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

  GET_INTEGER_PROP(obj, player, int32);
  GET_INTEGER_PROP(obj, lp,     int32);
  GET_INTEGER_PROP(obj, start,  int32);
  GET_INTEGER_PROP(obj, draw,   int32);

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

  GET_INTEGER_PROP(queryCardOptions, player,     uint32);
  GET_INTEGER_PROP(queryCardOptions, location,   uint32);
  GET_INTEGER_PROP(queryCardOptions, queryFlags, uint32);
  GET_INTEGER_PROP(queryCardOptions, sequence,   uint32);
  GET_PROP(queryCardOptions, useCache, Boolean);

  byte query_buffer[0x4000];
  auto length = query_card(duel, player, location, sequence, queryFlags, query_buffer, useCache);

  info.GetReturnValue().Set(Nan::CopyBuffer((char *)query_buffer, length).ToLocalChecked());
}

NAN_METHOD(queryFieldCard)
{
  CHECK_DUEL(0);
  CHECK_ARG(1, Object);

  const auto queryOptions = arg1.As<v8::Object>();

  GET_INTEGER_PROP(queryOptions, player,     uint32);
  GET_INTEGER_PROP(queryOptions, location,   uint32);
  GET_INTEGER_PROP(queryOptions, queryFlags, uint32);
  GET_PROP(queryOptions, useCache, Boolean);

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

  GET_INTEGER_PROP(new_card, code,     uint32);
  GET_INTEGER_PROP(new_card, owner,    uint8);
  GET_INTEGER_PROP(new_card, player,   uint8);
  GET_INTEGER_PROP(new_card, location, uint8);
  GET_INTEGER_PROP(new_card, sequence, uint8);
  GET_INTEGER_PROP(new_card, position, uint8);

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

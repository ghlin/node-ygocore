#include "core-wrapper.h"

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

  set_script_directory(path);
}

NAN_MODULE_INIT(Init)
{
  NAN_EXPORT(target, setScriptDirectory);
}

NODE_MODULE(ocgcore, Init)

} // namespace ny

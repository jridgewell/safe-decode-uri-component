#include <node.h>
#include <stdlib.h>
#include <string.h>

/**
 * Finds the next pointer to c in str.
 */
uint16_t *strchr16(const uint16_t *str, const uint16_t *const end, const uint16_t c) {
  while (*str != c) {
    if (++str >= end) {
      return (uint16_t *)end;
    }
  }
  return (uint16_t *)str;
}

/**
 * Decodes a single hex char into it's equivalent decimal value.
 * The value is then left shifted by `shift`.
 * If an invalid hex char is encountered, this returns `255`, which is
 * guaranteed to be rejected by the decoder FSM later on.
 */
uint8_t hex_char_to_int(const uint16_t c, const uint8_t shift) {
  switch (c) {
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    return (c - '0') << shift;
  case 'A':
  case 'B':
  case 'C':
  case 'D':
  case 'E':
  case 'F':
    return (c - 'A' + 10) << shift;
  case 'a':
  case 'b':
  case 'c':
  case 'd':
  case 'e':
  case 'f':
    return (c - 'a' + 10) << shift;
  default:
    return 255;
  }
}

/**
 * Shifts the "normal" characters from there current location in the string buffer
 * to the current decoded index. This is necessary because, as we decode percent-encoded
 * values, we take up less space.
 */
uint16_t *shift_chars(uint16_t *const dest, const uint16_t *const src, size_t n) {
  if (dest < src) {
    memmove(dest, src, n * sizeof(uint16_t));
  }
  return dest + n;
}

/**
 * The below algorithm is based on Bjoern Hoehrmann's DFA Unicode Decoder.
 * Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
 * See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
 */
#define SAFE_DECODE_UTF8_ACCEPT 12
#define SAFE_DECODE_UTF8_REJECT 0
uint32_t c_utf8_decode(uint8_t *const state, const uint32_t codep, const uint8_t byte) {
  static const uint8_t UTF8_DATA[] = {
      // The first part of the table maps bytes to character to a transition.
       0, 0, 0, 0,  0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0,  0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0,  0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0,  0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0,  0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0,  0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0,  0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0,  0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
       1, 1, 1, 1,  1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
       2, 2, 2, 2,  2, 2, 2, 2,   2, 2, 2, 2, 2, 2, 2, 2,
       3, 3, 3, 3,  3, 3, 3, 3,   3, 3, 3, 3, 3, 3, 3, 3,
       3, 3, 3, 3,  3, 3, 3, 3,   3, 3, 3, 3, 3, 3, 3, 3,
       4, 4, 5, 5,  5, 5, 5, 5,   5, 5, 5, 5, 5, 5, 5, 5,
       5, 5, 5, 5,  5, 5, 5, 5,   5, 5, 5, 5, 5, 5, 5, 5,
       6, 7, 7, 7,  7, 7, 7, 7,   7, 7, 7, 7, 7, 8, 7, 7,
      10, 9, 9, 9, 11, 4, 4, 4,   4, 4, 4, 4, 4, 4, 4, 4,

      // The second part of the table maps a state to a new state when adding a
      // transition.
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      12,  0,  0,  0,  0, 24, 36, 48, 60, 72, 84, 96,
       0, 12, 12, 12,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0, 24,  0,  0,  0,  0,  0,  0,  0,  0,
       0, 24, 24, 24,  0,  0,  0,  0,  0,  0,  0,  0,
       0, 24, 24,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0, 48, 48, 48,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0, 48, 48,  0,  0,  0,  0,  0,  0,  0,  0,
       0, 48,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,

      // The third part maps the current transition to a mask that needs to apply
      // to the byte.
      0x7F, 0x3F, 0x3F, 0x3F, 0x00, 0x1F, 0x0F, 0x0F, 0x0F, 0x07, 0x07, 0x07,
  };
  uint8_t type = UTF8_DATA[byte];

  *state = UTF8_DATA[256 + *state + type];
  return (codep << 6) | (byte & UTF8_DATA[364 + type]);
}

/**
 * The main code, this decodes the percent-encoded characters in place, shifting
 * all characters leftwards as it happens.
 * This returns the final length of the decoded string. If nothing is decoded, it
 * returns `0`.
 */
size_t safe_decode_utf8(uint16_t *encoded, const size_t length) {
  const uint16_t *const begin = encoded;
  const uint16_t *const end = begin + length;

  // The current octet that we are decoding.
  uint16_t *k = strchr16(encoded, end, '%');

  // The position of the first octet in this series.
  uint16_t *start_of_octets = k;

  // The position of the first character after the last decoded octet.
  // Everything from this point on (up to the start of the escape sequence)
  // will need to be moved leftwards to the insertion point if a new valid
  // octet series is encountered.
  uint16_t *start_of_chars = encoded;

  // The current "insertion" pointer, where we move our decoded and normal
  // characters.
  uint16_t *insertion = encoded;

  // The current octet series' accumulated code point.
  uint32_t codepoint = 0;

  // The state of the decoding FSM.
  uint8_t state = SAFE_DECODE_UTF8_ACCEPT;

  // If k goes beyond end-2, there's no way we can encounter another valid
  // percent encoded octet.
  while (k < end - 2) {
    // The character at k is always a '%', guaranteed by the strchr16 search.
    // So, interpret the hex chars as a single value.
    const uint8_t high = hex_char_to_int(*(k + 1), 4);
    const uint8_t low = hex_char_to_int(*(k + 2), 0);
    // If either is not a valid hex char, the result will be `255`, making
    // there OR 255, and making the FSM reject.
    // Otherwise, this will transition `state` into the next.
    codepoint = c_utf8_decode(&state, codepoint, high | low);

    switch (state) {
    case SAFE_DECODE_UTF8_ACCEPT:
      // A full codepoint has been found!
      // We first need to shift any characters leftwards, if any codepoints
      // have been found previously (since they decode into fewer characters).
      insertion = shift_chars(insertion, start_of_chars, start_of_octets - start_of_chars);

      // Push either a single character, or a surrogate character.
      if (codepoint <= 0xFFFF) {
        *insertion++ = codepoint;
      } else {
        *insertion++ = (0xD7C0 + (codepoint >> 10));
        *insertion++ = (0xDC00 + (codepoint & 0x3FF));
      }

      // Now, the first char after this escape is the next start_of_chars.
      // Find our next '%', and reset the accumulated code point.
      start_of_chars = k + 3;
      k = start_of_octets = strchr16(start_of_chars, end, '%');
      codepoint = 0;
      break;

    default:
      // We're in the middle of a multi-octet series.
      // Move to the next octet.
      k += 3;
      // If we find a '%', break out and continue with the loop.
      // Otherwise, we've encountered an invalid octet series, and need to
      // reject and reset.
      if (k < end && *k == '%') {
        break;
      }

    // Intentional fall-through
    case SAFE_DECODE_UTF8_REJECT:
      // We've encountered an invalid octet series.
      // Find the next '%' after this point, and reset the FSM and codepoint.
      k = start_of_octets = strchr16(start_of_octets + 1, end, '%');
      codepoint = 0;
      state = SAFE_DECODE_UTF8_ACCEPT;
      break;
    }
  }

  // Finally, if there are any normal characters after the last decoded octets,
  // we need to shift them leftwards.
  insertion = shift_chars(insertion, start_of_chars, end - start_of_chars);

  // Return the total length of decoded string.
  return insertion - begin;
}

/**
 * This is the C-to-JS interface, extracting the string param from the call,
 * decoding it, then returning the decoded value.
 */
void decode_uri_component(const v8::FunctionCallbackInfo<v8::Value> &args) {
  v8::Isolate *isolate = args.GetIsolate();

  if (args.Length() != 1) {
    isolate->ThrowException(
        v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Wrong number of arguments")));
    return;
  }

  if (!args[0]->IsString()) {
    isolate->ThrowException(
        v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Expected string argument")));
    return;
  }

  v8::String::Value encodedWrapper(args[0]->ToString());
  size_t length = encodedWrapper.length();
  uint16_t *encoded = *encodedWrapper;

  // Decode.
  length = safe_decode_utf8(encoded, length);

  // If we didn't decode anything, return the same JS string instance.
  if (length == 0) {
    args.GetReturnValue().Set(args[0]);
    return;
  }

  // Return it.
  args.GetReturnValue().Set(
      v8::String::NewFromTwoByte(isolate, encoded, v8::String::kNormalString, length));
}

/**
 * This sets up the exported module.
 * We only export the decoder function as "decode" property.
 */
void Init(v8::Local<v8::Object> exports, v8::Local<v8::Object> module) {
  NODE_SET_METHOD(module, "exports", decode_uri_component);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Init)

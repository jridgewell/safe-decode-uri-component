#include <node_api.h>
#include <stdlib.h>
#include <string.h>

/**
 * The equivalent of strchr for char16_t strings.
 * http://www.cplusplus.com/reference/cstring/strchr/
 */
char16_t *strchr16(const char16_t *str, const char16_t c) {
  while (*str != c) {
    if (!*str++) {
      return NULL;
    }
  }
  return (char16_t *)str;
}

/**
 * Decodes a single hex char into it's equivalent decimal value.
 * The value is then left shifted by `shift`.
 * If an invalid hex char is encountered, this returns `255`, which is guaranteed
 * to be rejected by the decoder FSM later on.
 */
uint8_t hex_char_to_int(const char16_t c, const uint8_t shift) {
  switch (c) {
  case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
    return (c - '0') << shift;
  case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    return (c - 'A' + 10) << shift;
  case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
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
char16_t *shift_chars(char16_t *dest, char16_t *src, size_t n) {
  memmove(dest, src, n * sizeof(char16_t));
  return dest + n;
}

/**
 * The below algorithm is based on Bjoern Hoehrmann's DFA Unicode Decoder.
 * Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
 * See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
 */
#define SAFE_DECODE_UTF8_ACCEPT 0
#define SAFE_DECODE_UTF8_REJECT 12
uint32_t c_utf8_decode(uint8_t *state, const uint32_t codep, const uint8_t byte) {
  static const uint8_t UTF8_DATA[] = {
      // The first part of the table maps bytes to character classes that
      // to reduce the size of the transition table and create bitmasks.
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 8, 8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 10, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 3, 3, 11, 6, 6, 6, 5, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8,

      // The second part is a transition table that maps a combination
      // of a state of the automaton and a character class to a state.
      0, 12, 24, 36, 60, 96, 84, 12, 12, 12, 48, 72, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
      12, 0, 12, 12, 12, 12, 12, 0, 12, 0, 12, 12, 12, 24, 12, 12, 12, 12, 12, 24, 12, 24, 12, 12,
      12, 12, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 12, 12, 24, 12,
      12, 12, 12, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, 12, 36, 12, 12, 12, 12, 12, 36, 12, 36,
      12, 12, 12, 36, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,

      // The third part maps the current character class to a mask that needs
      // to apply to it.
      0x7F, 0x3F, 0x1F, 0x0F, 0x0F, 0x07, 0x07, 0x3F, 0x00, 0x3F, 0x0F, 0x07,
  };
  uint8_t type = UTF8_DATA[byte];
  uint8_t mask = UTF8_DATA[364 + type];

  *state = UTF8_DATA[256 + *state + type];
  return (codep << 6) | (byte & mask);
}

/**
 * The main code, this decodes the percent-encoded characters in place, shifting
 * all characters leftwards as it happens.
 * This returns the final length of the decoded string. If nothing is decoded, it
 * returns `0`.
 */
size_t safe_decode_utf8(char16_t *encoded, const size_t length) {
  // The end of the buffer, used for simple out-of-bounds checking.
  const char16_t *end = encoded + length;

  // The current octet that we are decoding.
  char16_t *k = strchr16(encoded, '%');

  // The position of the first octet in this series.
  char16_t *start_of_octets = k;

  // The position of the first character after the last decoded octet.
  // Everything from this point on (up to the iteration index) will need to
  // be moved leftwards if a new valid octet series is encountered.
  char16_t *last = encoded;

  // The current "insertion" pointer, where we move our decoded and normal
  // characters.
  char16_t *index = encoded;

  // The current octet series' accumulated code point.
  uint32_t codepoint = 0;

  // The state of the decoding FSM.
  uint8_t state = SAFE_DECODE_UTF8_ACCEPT;

  // If k goes beyond end-2, there's no way we can encounter another valid
  // percent encoded octet.
  while (k && k < end - 2) {
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
      index = shift_chars(index, last, start_of_octets - last);

      // Push either a single character, or a surrogate character.
      if (codepoint <= 0xFFFF) {
        *index = codepoint;
        index++;
      } else {
        *index = (0xD7C0 + (codepoint >> 10));
        index++;
        *index = (0xDC00 + (codepoint & 0x3FF));
        index++;
      }

      // Now, the last unmoved character is the next char.
      // Find our next '%', and reset the accumulated code point.
      last = k + 3;
      k = start_of_octets = strchr16(last, '%');
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
      k = start_of_octets = strchr16(k + 1, '%');
      codepoint = 0;
      state = SAFE_DECODE_UTF8_ACCEPT;
      break;
    }
  }

  // Finally, if there are any normal characters after the last decoded octets,
  // we need to shift them leftwards.
  index = shift_chars(index, last, end - last);

  // Return the total length of decoded string.
  return index - encoded;
}

/**
 * This is the C-to-JS interface, extracting the string param from the call,
 * decoding it, then returning the decoded value.
 */
napi_value decode_uri_component(napi_env env, napi_callback_info info) {
  napi_status status;
  size_t argc = 1;
  napi_value argv[1];

  // Get the arguments.
  status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to get arguments");
    return NULL;
  }

  // Get the length of the encoded string.
  size_t length;
  status = napi_get_value_string_utf16(env, argv[0], NULL, 0, &length);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to get encoded string length");
    return NULL;
  }

  // Allocate a full buffer for the string.
  char16_t *encoded = malloc((length + 1) * sizeof(char16_t));
  if (encoded == NULL) {
    napi_throw_error(env, NULL, "Unable to allocate string space");
    return NULL;
  }
  encoded[length] = '\0';

  // Extract the string into the buffer.
  status = napi_get_value_string_utf16(env, argv[0], encoded, length + 1, NULL);
  if (status != napi_ok) {
    free(encoded);
    napi_throw_error(env, NULL, "Unable to get encoded string");
    return NULL;
  }

  // Decode.
  length = safe_decode_utf8(encoded, length);

  // If we didn't decode anything, return the same JS string instance.
  if (length == 0) {
    free(encoded);
    return argv[0];
  }

  // Create a new JS string to hold the decoded string.
  napi_value decoded;
  status = napi_create_string_utf16(env, encoded, length, &decoded);
  free(encoded);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to create decoded string");
    return NULL;
  }

  // Return it.
  return decoded;
}

/**
 * This sets up the exported module.
 * We only export the decoder function as "default" CJS export, without any
 * named properties.
 */
napi_value Init(napi_env env, napi_value exports) {
  napi_value decode;

  napi_status status = napi_create_function(env, NULL, 0, decode_uri_component, NULL, &decode);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to make decode function");
  }

  return decode;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)

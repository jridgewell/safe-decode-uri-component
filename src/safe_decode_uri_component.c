#include <node_api.h>
#include <stdlib.h>
#include <string.h>

char16_t *strchr16(const char16_t *str, const char16_t c) {
  while (*str != c) {
    if (!*str++) {
      return NULL;
    }
  }
  return (char16_t *)str;
}

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

size_t safe_decode_utf8(char16_t *encoded, const size_t length) {
  const char16_t *end = encoded + length;
  char16_t *k = strchr16(encoded, '%');
  char16_t *start_of_octets = k;
  char16_t *last = encoded;
  char16_t *index = encoded;
  uint32_t codepoint = 0;
  uint8_t state = SAFE_DECODE_UTF8_ACCEPT;

  while (k && k < end - 2) {
    const uint8_t high = hex_char_to_int(*(k + 1), 4);
    const uint8_t low = hex_char_to_int(*(k + 2), 0);
    codepoint = c_utf8_decode(&state, codepoint, high | low);

    switch (state) {
    case SAFE_DECODE_UTF8_ACCEPT:
      index = shift_chars(index, last, start_of_octets - last);

      if (codepoint <= 0xFFFF) {
        *index = codepoint;
        index++;
      } else {
        *index = (0xD7C0 + (codepoint >> 10));
        index++;
        *index = (0xDC00 + (codepoint & 0x3FF));
        index++;
      }

      last = k + 3;
      k = start_of_octets = strchr16(last, '%');
      codepoint = 0;
      break;

    default:
      k += 3;
      if (k < end && *k == '%') {
        break;
      }

    // Intentional fall-through
    case SAFE_DECODE_UTF8_REJECT:
      k = start_of_octets = strchr16(k + 1, '%');
      codepoint = 0;
      state = SAFE_DECODE_UTF8_ACCEPT;
      break;
    }
  }

  index = shift_chars(index, last, end - last);
  return index - encoded;
}

napi_value decode_uri_component(napi_env env, napi_callback_info info) {
  napi_status status;
  size_t argc = 1;
  napi_value argv[1];

  status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to get arguments");
    return NULL;
  }

  size_t length;
  status = napi_get_value_string_utf16(env, argv[0], NULL, 0, &length);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to get encoded string length");
    return NULL;
  }

  char16_t *encoded = malloc((length + 1) * sizeof(char16_t));
  if (encoded == NULL) {
    napi_throw_error(env, NULL, "Unable to allocate string space");
    return NULL;
  }
  encoded[length] = '\0';

  status = napi_get_value_string_utf16(env, argv[0], encoded, length + 1, NULL);
  if (status != napi_ok) {
    free(encoded);
    napi_throw_error(env, NULL, "Unable to get encoded string");
    return NULL;
  }

  length = safe_decode_utf8(encoded, length);

  if (length == 0) {
    free(encoded);
    return argv[0];
  }

  napi_value decoded;
  status = napi_create_string_utf16(env, encoded, length, &decoded);
  free(encoded);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to create decoded string");
    return NULL;
  }

  return decoded;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_value decode;

  napi_status status = napi_create_function(env, NULL, 0, decode_uri_component, NULL, &decode);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to make decode function");
  }

  return decode;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)

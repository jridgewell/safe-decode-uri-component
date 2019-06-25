{
  "targets": [
    {
      "target_name": "safe_decode_uri_component",
      "sources": [ "src/safe_decode_uri_component.c" ],
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ]
    }
  ]
}

const decodeURIComponent = require('bindings')('safe_decode_uri_component.node');
module.exports = function(string) {
  const k = string.indexOf('%');
  if (k === -1) {
    return string;
  }

  return decodeURIComponent(string, k);
};

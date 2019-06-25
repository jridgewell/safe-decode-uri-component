const benchmark = require('benchmark');
const decode = require('./');


const suite = new benchmark.Suite();

const short = encodeURIComponent('tést💩🇺🇸');
const medium = short.repeat(500);
const long = medium.repeat(500);
const shortWithout = 'abcd';
const mediumWithout = shortWithout.repeat(500);
const longWithout = mediumWithout.repeat(500);

// add tests
suite
.add('Short String (native)', function() {
  decodeURIComponent(short);
})
.add('Short String (safe)', function() {
  decode(short);
})

.add('Medium String (native)', function() {
  decodeURIComponent(medium);
})
.add('Medium String (safe)', function() {
  decode(medium);
})

.add('Long String (native)', function() {
  decodeURIComponent(long);
})
.add('Long String (safe)', function() {
  decode(long);
})

.add('Short String (without percents) (native)', function() {
  decodeURIComponent(shortWithout);
})
.add('Short String (without percents) (safe)', function() {
  decode(shortWithout);
})

.add('Medium String (without percents) (native)', function() {
  decodeURIComponent(mediumWithout);
})
.add('Medium String (without percents) (safe)', function() {
  decode(mediumWithout);
})

.add('Long String (without percents) (native)', function() {
  decodeURIComponent(longWithout);
})
.add('Long String (without percents) (safe)', function() {
  decode(longWithout);
})

// add listeners
.on('cycle', function(event) {
  console.log(String(event.target));
})
.on('complete', function() {
  console.log('Fastest is ' + this.filter('fastest').map('name'));
})
// run async
.run({ 'async': true });

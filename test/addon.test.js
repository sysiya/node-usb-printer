const addon = require('../index');

test('hello, world', () => {
  const result = addon.hello('World!');
  expect(result).toBe('Hello, World!');
});

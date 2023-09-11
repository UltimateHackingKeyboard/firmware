const path = require('path');
const fs = require('fs');

module.exports = function readPackageJson() {
  return JSON.parse(fs.readFileSync(path.join(__dirname, 'package.json'), 'utf8'));
}

import { readFileSync } from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

export function readPackageJson() {
  return JSON.parse(readFileSync(path.join(__dirname, 'package.json'), 'utf8'));
}

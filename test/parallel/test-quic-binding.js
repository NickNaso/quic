// Flags: --expose-internals
'use strict';

const common = require('../common');
if (!common.hasCrypto)
  common.skip('missing crypto');
const assert = require('assert');
const { internalBinding } = require('internal/test/binding');

const quic = internalBinding('quic');
assert(quic);

// Version numbers used to identify IETF drafts are created by adding the draft
// number to 0xff0000, in this case 11 (17).
assert.strictEqual(quic.protocolVersion().toString(16), 'ff000011');
assert.strictEqual(quic.alpnVersion(), '\u0005hq-17');

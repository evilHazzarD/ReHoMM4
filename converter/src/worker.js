import {prepareConversion} from './session.js';

// This module is started as a module Worker by the Pages UI.  File objects are
// supplied by the browser's picker; the worker deliberately has no fetch/XHR
// path, so installers and generated builds cannot leave the user's machine.
self.addEventListener('message', async event => {
  const {id, type, payload} = event.data || {};
  if (type !== 'prepare') return;
  try {
    const plan = await prepareConversion(payload || {});
    self.postMessage({id, ok: true, plan});
  } catch (error) {
    self.postMessage({id, ok: false, error: String(error?.message || error)});
  }
});

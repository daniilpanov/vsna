const $ = (id) => document.getElementById(id);

async function api(method, path, body) {
  const opts = { method, headers: { 'Content-Type': 'application/json' } };
  if (body !== undefined) opts.body = JSON.stringify(body);
  const res = await fetch(path, opts);
  const data = await res.json().catch(() => ({}));
  if (!res.ok) throw new Error(data.error || `HTTP ${res.status}`);
  return data;
}

async function refreshDescribe() {
  const d = await api('GET', '/api/describe');
  $('describe').textContent = d.addr + '\n' + d.path;
}

async function refreshPeers() {
  const { known, connected } = await api('GET', '/api/peers');
  const list = $('peer-list');
  list.textContent = '';
  for (const p of known) {
    const li = document.createElement('li');
    li.textContent = p + (connected.includes(p) ? '  [connected]' : '');
    list.appendChild(li);
  }
}

$('add-peer').addEventListener('submit', async (e) => {
  e.preventDefault();
  await api('POST', '/api/add', { addr: $('peer-addr').value });
  $('peer-addr').value = '';
  refreshPeers();
});

$('connect-peer').addEventListener('submit', async (e) => {
  e.preventDefault();
  const [host, port] = $('connect-addr').value.split(':');
  await api('POST', '/api/connect', { host, port });
  $('connect-addr').value = '';
});

$('connect-all').addEventListener('click', async () => {
  await api('POST', '/api/connect_all');
  refreshPeers();
});

$('send-form').addEventListener('submit', async (e) => {
  e.preventDefault();
  const peer = $('send-peer').value;
  const payload = { path: $('send-path').value };
  if (peer) payload.peer = peer;
  await api('POST', '/api/send', payload);
  $('send-status').textContent = 'Status: transfer started';
});

refreshDescribe();
refreshPeers();
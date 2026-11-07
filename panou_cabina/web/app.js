const $ = id => document.getElementById(id);
const text = (id, value) => { $(id).textContent = value; };
const resetReasonText = reason => ({
  1: 'POR / Brownout',
  2: 'Reset watchdog',
  3: 'Repornire software / pin RUN',
  4: 'Reset prin 4 apasari'
}[reason] || 'Cauza necunoscuta (' + reason + ')');

const themeToggle = $('theme-toggle');
const savedTheme = localStorage.getItem('webTheme');
const initialTheme = savedTheme === 'light' ? 'light' : 'dark';
document.documentElement.dataset.theme = initialTheme;
themeToggle.checked = initialTheme === 'dark';
themeToggle.addEventListener('change', () => {
  const theme = themeToggle.checked ? 'dark' : 'light';
  document.documentElement.dataset.theme = theme;
  localStorage.setItem('webTheme', theme);
});

let webAuthorization = '';
let webAuthenticated = false;
let networkFormDirty = false;

async function refresh() {
  try {
    const r = await fetch('/api/state', { cache: 'no-store' });
    const s = await r.json();

    text('floor', s.lift.floor);
    text('destination', s.lift.destination);
    text('direction', s.lift.direction);
    text('service', s.lift.service);
    text('occupancy', s.lift.occupied ? 'Ocupat' : 'Liber');

    text('uptime', Math.floor(s.system.uptime / 3600) + ' h ' + Math.floor(s.system.uptime % 3600 / 60) + ' min');
    text('reset', resetReasonText(s.system.resetReason));

    text('frames', s.diagnostics.frames);
    text('counting', s.diagnostics.counting ? 'Activ' : 'Oprit');
    text('valid', s.diagnostics.valid);
    text('crc', s.diagnostics.crc);
    text('timeouts', s.diagnostics.timeouts);
    text('errors', s.diagnostics.format + s.diagnostics.data);

    text('network', s.network.ip + ' / ' + s.network.mac);
    text('network-detail', (s.network.dhcp ? 'DHCP' : 'Static') + '; IP activ ' + s.network.ip);

    if (!networkFormDirty) {
      const n = $('network-settings').elements;
      n.dhcp.checked = s.network.dhcp;
      n.ip.value = s.network.staticIp;
      n.subnet.value = s.network.subnet;
      n.gateway.value = s.network.gateway;
      n.dns.value = s.network.dns;
    }
    $('settings').elements.lift.value = s.config.lift;
    $('settings').elements.gain.value = s.config.gain;

    text('auth-note', !s.config.auth
      ? 'Comenzile sunt dezactivate: parola WEB_UI_PASSWORD nu este configurata.'
      : webAuthenticated ? 'Autentificare reusita' : 'Introdu parola pentru a activa comenzile.');
    document.querySelectorAll('[data-command],#save-lift,#save-gain,#save-network')
      .forEach(c => { c.disabled = !s.config.auth || !webAuthenticated; });
    $('web-login-button').disabled = !s.config.auth;
    $('web-logout-button').disabled = !webAuthenticated;
  } catch (e) {
    text('network', 'Conexiune indisponibila');
  }
}

async function authenticate() {
  webAuthorization = 'Basic ' + btoa('service:' + $('web-password').value);
  $('web-password').value = '';
  try {
    const r = await fetch('/api/auth', {
      method: 'POST',
      headers: { 'Content-Type': 'text/plain', 'Authorization': webAuthorization },
      body: 'AUTH'
    });
    webAuthenticated = r.ok;
    const body = await r.text();
    text('result', r.ok ? 'Autentificare reusita' : 'Eroare ' + r.status + ': ' + body);
    if (!r.ok) webAuthorization = '';
    await refresh();
  } catch (e) {
    webAuthorization = '';
    webAuthenticated = false;
    text('result', 'Eroare de conexiune');
  }
}

async function command(path, value, onSuccess) {
  const headers = { 'Content-Type': 'text/plain' };
  if (webAuthorization) headers.Authorization = webAuthorization;
  const r = await fetch(path, { method: 'POST', headers, body: value });
  const body = await r.text();
  if (r.status === 401) {
    webAuthorization = '';
    webAuthenticated = false;
    await refresh();
  }
  text('result', r.ok ? body : 'Eroare ' + r.status + ': ' + body);
  if (r.ok && onSuccess) onSuccess();
  setTimeout(refresh, 250);
}

$('web-login-button').onclick = authenticate;
$('network-settings').addEventListener('input', () => { networkFormDirty = true; });
$('network-settings').addEventListener('change', () => { networkFormDirty = true; });
$('web-logout-button').onclick = () => {
  webAuthorization = '';
  webAuthenticated = false;
  text('result', 'Deconectat');
  refresh();
};
document.querySelectorAll('[data-command]')
  .forEach(b => { b.onclick = () => command('/api/service', b.dataset.command); });
$('save-lift').onclick = () => command('/api/service', 'mb_11 ' + $('settings').elements.lift.value);
$('save-gain').onclick = () => command('/api/service', 'mb_12 ' + $('settings').elements.gain.value);
$('save-network').onclick = () => {
  const f = $('network-settings').elements;
  const body = f.dhcp.checked
    ? 'DHCP'
    : 'STATIC ' + f.ip.value + ' ' + f.subnet.value + ' ' + f.gateway.value + ' ' + f.dns.value;
  command('/api/network', body, () => {
    networkFormDirty = false;
    refresh();
  });
};

refresh();
setInterval(refresh, 2000);

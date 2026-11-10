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
let serviceModeActive = false;

const otaPhaseNames = ['Pregatit', 'Upload in curs...', 'Incarcat pe SD',
  'Verificare in curs...', 'Verificat', 'Instalare ceruta...',
  'Oprire servicii...', 'Instalare in curs...', 'Eroare', 'Finalizat'];
let lastOtaState = null;

function updateOtaUi(s) {
  const section = $('ota-section');
  section.hidden = !webAuthenticated;
  if (!webAuthenticated) return;
  if (s.update) lastOtaState = s.update;
  const u = lastOtaState;
  if (!u) return;
  $('ota-file').disabled = !serviceModeActive;
  const hasFile = $('ota-file').files.length > 0;
  $('ota-upload').disabled = !serviceModeActive || !hasFile ||
    !(u.phase === 0 || u.phase === 2 || u.phase === 4 || u.phase === 8 || u.phase === 9);
  $('ota-verify').disabled = !serviceModeActive || u.phase !== 2;
  $('ota-apply').disabled = !serviceModeActive || u.phase !== 4 || (u.compatKnown && !u.compat);
  let status = serviceModeActive
    ? 'Stare: ' + (otaPhaseNames[u.phase] || ('Necunoscuta (' + u.phase + ')'))
    : 'Disponibil numai in Service Mode (apasa Intrare service).';
  if (u.file) status += ' | ' + u.file;
  if (u.compatKnown) status += u.compat ? ' | compatibil' : ' | INCOMPATIBIL cu acest panou';
  if (serviceModeActive && u.bootOk) status += ' | boot confirmat';
  if (serviceModeActive && u.error) status += ' | ' + u.error;
  $('ota-status').textContent = status;
  if (u.total > 0 && (u.phase === 1 || u.phase === 3)) {
    $('ota-progress').style.width = Math.floor(u.bytes * 100 / u.total) + '%';
  } else if (u.phase === 0) {
    $('ota-progress').style.width = '0';
  }
}

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
    serviceModeActive = Boolean(s.config.serviceMode);
    updateOtaUi(s);

    text('auth-note', !s.config.auth
      ? 'Comenzile sunt dezactivate: parola WEB_UI_PASSWORD nu este configurata.'
      : webAuthenticated ? 'Autentificare reusita' : 'Introdu parola pentru a activa comenzile.');
    document.querySelectorAll('[data-command],#save-lift,#save-gain,#save-network')
      .forEach(c => { c.disabled = !s.config.auth || !webAuthenticated; });
    $('web-login-button').disabled = !s.config.auth;
    $('web-logout-button').disabled = !webAuthenticated;
  } catch (e) {
    serviceModeActive = false;
    $('ota-section').hidden = true;
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

$('ota-file').onchange = () => updateOtaUi({});
$('ota-upload').onclick = () => {
  const file = $('ota-file').files[0];
  if (!file || !webAuthenticated || !serviceModeActive) return;
  if (!file.name.toLowerCase().endsWith('.uf2')) {
    $('ota-status').textContent = 'Selectati un fisier .uf2';
    return;
  }
  if (!window.confirm('Se incarca "' + file.name + '" pe cardul SD al panoului. Continuati?')) return;
  const xhr = new XMLHttpRequest();
  xhr.open('POST', '/api/update/upload');
  xhr.setRequestHeader('Authorization', webAuthorization);
  xhr.setRequestHeader('Content-Type', 'application/octet-stream');
  xhr.setRequestHeader('X-Filename', file.name);
  xhr.upload.onprogress = e => {
    if (e.lengthComputable) {
      $('ota-progress').style.width = Math.floor(e.loaded * 100 / e.total) + '%';
    }
  };
  xhr.onload = () => { $('ota-status').textContent = xhr.responseText; setTimeout(refresh, 300); };
  xhr.onerror = () => { $('ota-status').textContent = 'Eroare de conexiune la upload'; refresh(); };
  $('ota-status').textContent = 'Upload in curs...';
  xhr.send(file);
};
$('ota-verify').onclick = () => command('/api/update/verify', 'VERIFY');
$('ota-apply').onclick = () => {
  if (!window.confirm('Firmware-ul verificat va fi instalat si panoul va reporni. Nu intrerupeti alimentarea. Continuati?')) return;
  command('/api/update/apply', 'APPLY');
};
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

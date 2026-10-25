#pragma once

// Static, self-contained phone UI. No external CSS/JS dependencies.
static const char PUMPKIN_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Haunted Pumpkin</title><style>
*{box-sizing:border-box}body{background:#0c0a14;color:#eee;font:16px system-ui,sans-serif;margin:0 auto;max-width:720px;padding:22px}
h1{color:#ffa329;margin-bottom:5px}p{color:#aea9bd}section{background:#1a1724;padding:19px;border-radius:16px;margin:14px 0}
label{display:block;color:#eee;margin:10px 0 5px}select,input,button{width:100%;border-radius:9px;padding:12px;border:1px solid #454054;background:#252132;color:white;font:inherit}
button{background:#dd6e10;border:0;font-weight:700;margin-top:10px}button.secondary{background:#393048}small{color:#aaa}#status{color:#88ddaa}input[type=range]{padding:0;accent-color:#ff9820}
</style></head><body><h1>🎃 Haunted Pumpkin</h1><p>Twenty spooky effects, one little monster.</p>
<section><label for="effect">Lighting effect</label><select id="effect"></select><button onclick="next()">Next Effect</button></section>
<section><label for="power">Lights</label><button id="power" onclick="toggle()">Toggle</button><label for="brightness">Brightness: <span id="level">60%</span></label><input id="brightness" type="range" min="1" max="100" value="60" onchange="set({brightness:this.value})" oninput="document.getElementById('level').textContent=this.value+'%'" /></section>
<p id="status">Connecting...</p><p><small>Use Google Home for voice on/off and brightness after linking Sinric Pro. The full effect mode list also works via Google Home after the Sinric Pro Mode capability is configured.</small></p>
<script>
let state=null;
async function refresh(){try{const response=await fetch('/api/state',{cache:'no-store'});if(!response.ok)throw Error(response.status);let d=await response.json();state=d;
const sel=document.getElementById('effect');if(sel.options.length!==d.effects.length){sel.replaceChildren();d.effects.forEach((n,i)=>sel.add(new Option((i+1)+'. '+n,i)))}sel.value=d.pendingEffect;
document.getElementById('power').textContent=d.on?'Turn LEDs off':'Turn LEDs on';
document.getElementById('brightness').value=d.brightness;document.getElementById('level').textContent=d.brightness+'%';
document.getElementById('status').textContent='Connected · '+(d.on?'LEDs on':'LEDs off')+(d.wifi?' · Wi-Fi online':' · setup Wi-Fi');
}catch(e){document.getElementById('status').textContent='Connection lost: '+e.message}}
async function set(p){await fetch('/api/set',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(p)});refresh()}
function toggle(){set({power:state&&state.on?'off':'on'})}
function next(){set({effect:(Number(document.getElementById('effect').value)+1)%20})}
document.getElementById('effect').addEventListener('change',function(){set({effect:this.value})});
refresh();setInterval(refresh,3000);
</script></body></html>)HTML";

static const char PUMPKIN_SETUP_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Pumpkin Setup</title><style>
body{font:16px system-ui;background:#16121f;color:white;max-width:600px;margin:30px auto;padding:20px}h1{color:orange}input,button{display:block;width:100%;box-sizing:border-box;margin:8px 0 16px;padding:13px;border-radius:9px;border:1px solid #777}button{background:#ef8021;color:#16121f;font-weight:700}small{color:#bbb}
</style></head><body><h1>🎃 Pumpkin Setup</h1><p>Paste your Wi-Fi password once. No typing it on the ESP32.</p>
<form method="post" action="/save"><label>Wi-Fi network name (2.4GHz)</label><input maxlength="32" name="ssid" autocomplete="off" placeholder="Blank keeps previous SSID">
<label>Wi-Fi password</label><input type="password" name="password" maxlength="64" autocomplete="new-password" placeholder="Blank keeps previous password">
<h2>Google Home (optional)</h2><p><small>Create a Sinric Pro custom device template (Light type) with Power, Brightness and Mode. Set Mode InstanceId to pumpkin_effect; add all 20 exact effect names. Then link Sinric Pro in Google Home. Paste the three values below; you can leave them empty and add them later.</small></p>
<label>Sinric Pro App Key</label><input name="sinric_key" maxlength="128">
<label>Sinric Pro App Secret</label><input name="sinric_secret" maxlength="160">
<label>Sinric Pro Custom Device ID</label><input name="sinric_device" maxlength="40">
<h2>Other settings</h2><p><small>All fields may be left blank when updating an existing configuration. You only need to flash once.</small></p>
<label>Number of LEDs (1-32)</label><input name="led_count" type="number" min="1" max="32" placeholder="8">
<label>New OTA update password (8+ characters, optional)</label><input name="ota_password" type="password" minlength="8" maxlength="64" placeholder="Leave blank to keep existing">
<button type="submit">Save &amp; Restart</button></form><p><small>Keep this setup AP private. Long-press BOOT while running to enter setup mode again.</small></p></body></html>)HTML";
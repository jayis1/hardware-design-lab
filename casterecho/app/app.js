// CasterEcho local-first Web Bluetooth application
// Author: jayis1
// Copyright (c) 2026 jayis1
// SPDX-License-Identifier: MIT
'use strict';
const P = window.CasterEchoProtocol;
const SERVICE = 'f4f5ce00-7d11-4e65-a934-5d00c4579a10';
const RX = 'f4f5ce01-7d11-4e65-a934-5d00c4579a10';
const TX = 'f4f5ce02-7d11-4e65-a934-5d00c4579a10';
let tx; let sequence = 1;
const demo = Array.from({ length: 24 }, (_, i) => ({ sequence: i + 1, health: Math.round(96 - i * .18 - (i > 17 ? (i - 17) * .7 : 0)), rms: .055 + i * .0015, drag: .09 + i * .002, temperature: 29 + i * .12, speed: 1.15, flags: i > 20 ? 2 : 0 }));
let records = JSON.parse(localStorage.getItem('casterecho-records') || 'null') || demo;
const $ = id => document.getElementById(id);

function flagsText(flags) {
  const names = [[1,'flat spot'],[2,'bearing roughness'],[4,'drag'],[8,'swivel chatter'],[16,'overheat'],[32,'sensor fault'],[64,'low energy'],[128,'learning']];
  const found = names.filter(([mask]) => flags & mask).map(([,name]) => name);
  return found.length ? found.join(', ') : 'normal signature';
}
function serviceAction(record) {
  if (record.flags & 16) return 'Stop and inspect';
  if (record.flags & 3) return 'Schedule wheel service';
  if (record.flags & 12) return 'Inspect swivel and debris';
  return 'No action';
}
function render() {
  const latest = records.at(-1) || demo.at(-1);
  $('health').textContent = latest.health; $('rms').textContent = `${latest.rms.toFixed(3)} g`;
  $('drag').textContent = latest.drag.toFixed(2); $('temp').textContent = `${latest.temperature.toFixed(1)} °C`; $('speed').textContent = `${latest.speed.toFixed(1)} m/s`;
  $('wheel-fill').parentElement.style.background = `conic-gradient(var(--mint) 0 ${latest.health}%, #203731 ${latest.health}%)`;
  $('findings').innerHTML = `<span class="finding ${latest.flags ? 'warn' : ''}">${flagsText(latest.flags)}</span><span class="finding">Trend: ${latest.health < 85 ? 'degrading' : 'stable'}</span>`;
  $('history').innerHTML = records.slice(-8).reverse().map(r => `<tr><td>#${r.sequence}</td><td class="score">${r.health}/100</td><td>${flagsText(r.flags)}</td><td>${serviceAction(r)}</td></tr>`).join('');
  draw(records.slice(-Number($('range').value)));
}
function draw(data) {
  const canvas=$('chart'),ctx=canvas.getContext('2d'),w=canvas.width,h=canvas.height;ctx.clearRect(0,0,w,h);
  ctx.strokeStyle='#24413a';ctx.lineWidth=1;for(let y=30;y<h;y+=45){ctx.beginPath();ctx.moveTo(24,y);ctx.lineTo(w-24,y);ctx.stroke();}
  const line=(field,color,min,max)=>{ctx.strokeStyle=color;ctx.lineWidth=3;ctx.beginPath();data.forEach((r,i)=>{const x=24+i*(w-48)/Math.max(data.length-1,1),y=h-24-(r[field]-min)/(max-min)*(h-48);i?ctx.lineTo(x,y):ctx.moveTo(x,y);});ctx.stroke();};
  line('rms','#61e7b6',0,.18);line('drag','#ffc766',0,.5);
}
async function send(type,payload=new Uint8Array()){if(!tx)throw new Error('not connected');await tx.writeValueWithoutResponse(P.encodePacket(type,sequence++,payload));}
async function connect(){
  if(!navigator.bluetooth){$('status').textContent='Web Bluetooth unavailable';return;}
  try { const device=await navigator.bluetooth.requestDevice({filters:[{services:[SERVICE]}]});const server=await device.gatt.connect();const service=await server.getPrimaryService(SERVICE);tx=await service.getCharacteristic(TX);const rx=await service.getCharacteristic(RX);await rx.startNotifications();rx.addEventListener('characteristicvaluechanged',event=>{try{const packet=P.decodePacket(event.target.value.buffer);if(packet.type===P.MESSAGE.PASS||packet.type===P.MESSAGE.HISTORY){const pass=P.parsePass(packet.payload);records.push(pass);records=records.slice(-192);localStorage.setItem('casterecho-records',JSON.stringify(records));render();}}catch(error){console.warn(error);}});$('status').textContent=`Connected: ${device.name||'CasterEcho'}`;await send(P.COMMAND.LATEST); }
  catch(error){$('status').textContent=error.name==='NotFoundError'?'Connection cancelled':'Connection failed';}
}
function exportCsv(){const header='sequence,health,rms_g,drag,temp_c,speed_mps,flags\n';const rows=records.map(r=>[r.sequence,r.health,r.rms,r.drag,r.temperature,r.speed,r.flags].join(',')).join('\n');const a=document.createElement('a');a.href=URL.createObjectURL(new Blob([header+rows],{type:'text/csv'}));a.download='casterecho-history.csv';a.click();URL.revokeObjectURL(a.href);}
$('connect').addEventListener('click',connect);$('range').addEventListener('change',render);$('export').addEventListener('click',exportCsv);
$('save-wheel').addEventListener('click',async()=>{const mm=Number($('circumference').value);if(mm<150||mm>1000)return;const bytes=new Uint8Array(4);new DataView(bytes.buffer).setFloat32(0,mm,true);try{await send(P.COMMAND.SET_WHEEL,bytes);$('status').textContent='Wheel circumference saved';}catch{$('status').textContent='Connect to save on sensor';}});
$('calibrate').addEventListener('click',async()=>{if(!confirm('Use the latest pass as a known-healthy baseline?'))return;try{await send(P.COMMAND.CALIBRATE);$('status').textContent='Baseline accepted';}catch{$('status').textContent='Connect before calibration';}});
render();

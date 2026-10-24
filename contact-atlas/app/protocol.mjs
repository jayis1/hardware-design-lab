// Author: jayis1. SPDX-License-Identifier: MIT
export function crc(text) {
 let c = 0xffff;
 for (const ch of text) {
  c ^= ch.charCodeAt(0) << 8;
  for (let i=0;i<8;i++) c = ((c << 1) ^ ((c & 0x8000) ? 0x1021 : 0)) & 0xffff;
 }
 return c;
}
export function parse(line) {
 if (line.length > 255 || !/^CA1,[SE],[0-9,]+\*[0-9A-F]{4}$/.test(line)) throw Error('Invalid frame');
 const [body, checksum] = line.split('*');
 if (crc(body) !== Number.parseInt(checksum,16)) throw Error('CRC mismatch');
 const [version, type, ...fields] = body.split(',');
 const values = fields.map(s => {
  if (!/^(0|[1-9][0-9]*)$/.test(s)) throw Error('Invalid integer');
  const n = Number(s);
  if (!Number.isSafeInteger(n) || n > 0xffffffff) throw Error('Integer overflow');
  return n;
 });
 const names = type === 'S' ? ['seq','time','known','travelling','raw','stable','transitions','gaps','invalid','dropped','loss','age'] : ['seq','time','number','from','to','duration','zero','multi','changes'];
 if (values.length !== names.length) throw Error('Field count');
 const row = {version,type};
 names.forEach((name,i)=>row[name]=values[i]);
 if (type === 'S') {
  if (row.known > 1 || row.travelling > 1 || row.raw > 15 || row.stable > 15) throw Error('Status range');
 } else if (row.from > 15 || row.to > 15 || row.zero + row.multi > row.duration || row.changes < 1 || row.number < 1) throw Error('Event range');
 return row;
}
export class Framer {
 constructor() { this.buffer=''; this.discard=false; this.errors=0; }
 feed(chunk) {
  const rows=[];
  for (const c of chunk) {
   if (c === '\n') {
    if (!this.discard) {
     try { rows.push(parse(this.buffer)); } catch { this.errors++; }
    }
    this.buffer=''; this.discard=false;
   } else if (!this.discard) {
    if (this.buffer.length === 255) { this.buffer=''; this.discard=true; this.errors++; }
    else {
     this.buffer += c;
     // The complete version marker cannot occur inside fields or a CRC.
     if (this.buffer.length>4 && this.buffer.endsWith('CA1,')) {
      this.errors++; this.buffer='CA1,';
     }
    }
   }
  }
  return rows;
 }
}
export class Session {
 constructor() { this.status=null; this.events=[]; this.epoch=0; this.last=null; this.missing=0; this.received=-Infinity; this.rejected=0; this.evicted=0; this.discontinuity=false; }
 accept(row, now) {
  // No boot ID: preserve evidence and require explicit reconnect on regression.
  if (this.discontinuity) return false;
  if (row.type === 'S' && this.status && ((row.time-this.status.time) >>> 0) >= 0x80000000) {
   this.discontinuity=true; this.rejected++; return false;
  }
  if (row.type === 'E' && !this.status) return false;
  if (this.last !== null) {
   const delta = (row.seq-this.last) >>> 0;
   if (!delta || delta >= 0x80000000) {
    if (delta) this.discontinuity=true;
    this.rejected++; return false;
   }
   this.missing += delta-1;
  }
  this.last=row.seq;
  if (row.type === 'S') { this.status=row; this.received=now; }
  else {
   this.events.push({...row,epoch:this.epoch});
   if (this.events.length>1000) { this.events.shift(); this.evicted++; }
  }
  return true;
 }
 live(now, connected=true) { return connected && !this.discontinuity && now-this.received < 1000; }
}

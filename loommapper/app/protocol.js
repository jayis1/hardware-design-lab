/* LoomMapper protocol codec. Author: jayis1. Copyright (c) 2026 jayis1. */
'use strict';
const MAGIC=[0x4c,0x4d], VERSION=1;
function crc16(bytes){let crc=0xffff;for(const byte of bytes){crc^=byte<<8;for(let i=0;i<8;i++)crc=(crc&0x8000)?((crc<<1)^0x1021)&0xffff:(crc<<1)&0xffff;}return crc;}
function command(type,payload=[]){const out=new Uint8Array(9+payload.length);out.set(MAGIC);out[2]=VERSION;out[3]=type;out[4]=payload.length&255;out[5]=payload.length>>8;out.set(payload,6);const crc=crc16(out.slice(0,6+payload.length));out[6+payload.length]=crc&255;out[7+payload.length]=crc>>8;out[8+payload.length]=10;return out;}
function validate(data){const b=Uint8Array.from(data);if(b.length<9)throw new Error('short frame');if(b[0]!==MAGIC[0]||b[1]!==MAGIC[1])throw new Error('bad magic');if(b[2]!==VERSION)throw new Error('unsupported protocol');const size=b[4]|b[5]<<8;if(b.length!==size+9||b.at(-1)!==10)throw new Error('bad length');const expected=b[6+size]|b[7+size]<<8;if(crc16(b.slice(0,6+size))!==expected)throw new Error('bad crc');return {type:b[3],payload:b.slice(6,6+size)};}
function kindName(kind){return ['Unknown','Good','Open','Short','Intermittent','Crosstalk'][kind]||'Invalid';}
function synthetic(seed=0,flex=false){const map=[3,6,1,7,0,2,5,4];const edges=[];for(let source=0;source<8;source++){for(let sink=0;sink<8;sink++){let kind=map[source]===sink?1:2, confidence=kind===1?958:925, variation=20+(source*11+sink*7+seed)%38;if(source===2&&sink===4){kind=5;confidence=742;}if(flex&&source===4&&sink===0){kind=4;confidence=887;variation=710;}edges.push({source,sink,kind,confidence,variation});}}return {battery:3920,sequence:120+seed,crc:(0xa4139f20+seed)>>>0,edges,flex};}
if(typeof module!=='undefined')module.exports={crc16,command,validate,kindName,synthetic};

// Auto-generated from index.html - the game page served by the ESP32
#pragma once
#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no,viewport-fit=cover">
<meta name="theme-color" content="#000000">
<title>Space Shooter</title>
<style>
html,body{margin:0;height:100%;background:#000;overflow:hidden;touch-action:none;
  -webkit-user-select:none;user-select:none;-webkit-touch-callout:none;-webkit-tap-highlight-color:transparent}
body{position:fixed;inset:0;display:flex;align-items:center;justify-content:center}
canvas{display:block;background:#000;touch-action:none}
#base,#knob{position:fixed;left:0;top:0;border-radius:50%;pointer-events:none;display:none}
#base{width:90px;height:90px;margin:-45px 0 0 -45px;border:2px solid rgba(255,255,255,.35);background:rgba(255,255,255,.06)}
#knob{width:38px;height:38px;margin:-19px 0 0 -19px;background:rgba(0,255,255,.45);border:2px solid rgba(255,255,255,.6)}
#mute{position:fixed;right:10px;bottom:10px;width:40px;height:40px;border-radius:50%;
  border:1px solid rgba(255,255,255,.3);background:rgba(0,0,0,.5);color:#fff;font-size:18px;padding:0}
</style>
</head>
<body>
<canvas id="c"></canvas>
<div id="base"></div><div id="knob"></div>
<button id="mute" aria-label="sound">&#128266;</button>
<script>
(()=>{
'use strict';

// ===================== Settings =====================
const W=128, H=160, FR=1000/30;          // logical screen, 30 "frames" per second reference
const IS=10;                             // item size
const PS=3.5;                            // ship speed (px per reference frame)
const FIRE=170;                          // ms between shots (rapid = half)
const START_LIVES=3, MAX_LIVES=5, MAX_WEAPON=4, RAPID_MS=8000;
const DROP_W=10, DROP_L=5, DROP_R=6;     // drop chances (%)
const DROP_BONUS=[0,10,25];              // extra drop chance: grunt, shooter, tank
const STICK_R=40;                        // joystick radius (css px)

// ----- difficulty -----
const SCORE_PER_LEVEL=120;               // score needed per level (lower = harder)
const BOSS_EVERY=5;                      // boss appears on every 5th level
const SIZES=[[15,12],[18,15],[26,20]];   // enemy sizes: grunt, shooter, tank
const SCORE_BY=[10,25,60];               // score per enemy type

const C={cyan:'#00ffff',white:'#ffffff',red:'#ff2020',green:'#20ff20',blue:'#3050ff',
  magenta:'#ff30ff',yellow:'#ffff00',orange:'#ffa500',gray:'#808080',dgray:'#404040',black:'#000000'};

// ===================== Canvas =====================
const cv=document.getElementById('c'), ctx=cv.getContext('2d');
let S=1;
function resize(){
  const vw=innerWidth, vh=innerHeight;
  const cw=Math.min(vw, vh*W/H), ch=cw*H/W;
  cv.style.width=cw+'px'; cv.style.height=ch+'px';
  const dpr=Math.min(window.devicePixelRatio||1,3);
  cv.width=Math.round(cw*dpr); cv.height=Math.round(ch*dpr);
  S=cv.width/W;
}
addEventListener('resize',resize); resize();

// ===================== Helpers =====================
const clamp=(v,a,b)=>v<a?a:(v>b?b:v);
const rnd=(a,b)=>Math.random()*(b-a)+a;
const rndi=(a,b)=>Math.floor(rnd(a,b));
const hit=(ax,ay,aw,ah,bx,by,bw,bh)=>ax<bx+bw&&ax+aw>bx&&ay<by+bh&&ay+ah>by;
function rect(x,y,w,h,c){ctx.fillStyle=c;ctx.fillRect(x,y,w,h);}
function tri(x1,y1,x2,y2,x3,y3,c){ctx.fillStyle=c;ctx.beginPath();ctx.moveTo(x1,y1);ctx.lineTo(x2,y2);ctx.lineTo(x3,y3);ctx.closePath();ctx.fill();}
function disc(x,y,r,c){ctx.fillStyle=c;ctx.beginPath();ctx.arc(x,y,Math.max(0.1,r),0,6.2832);ctx.fill();}
function ring(x,y,r,c){ctx.strokeStyle=c;ctx.lineWidth=1;ctx.beginPath();ctx.arc(x,y,Math.max(0.1,r),0,6.2832);ctx.stroke();}
function text(s,x,y,c,size,align){
  ctx.font='bold '+(size||8)+'px monospace';
  ctx.textBaseline='top'; ctx.textAlign=align||'left'; ctx.fillStyle=c;
  ctx.fillText(s,x,y);
}

// ===================== Sound =====================
let ac=null, muted=false;
try{muted=localStorage.getItem('ss_mute')==='1';}catch(e){}
const muteBtn=document.getElementById('mute');
function showMute(){muteBtn.innerHTML=muted?'&#128263;':'&#128266;';}
showMute();
function audioInit(){
  if(!ac){try{ac=new (window.AudioContext||window.webkitAudioContext)();}catch(e){}}
  if(ac&&ac.state==='suspended')ac.resume();
}
function beep(f,d,type,vol,slide){
  if(!ac||muted)return;
  const t=ac.currentTime, o=ac.createOscillator(), g=ac.createGain();
  o.type=type||'square';
  o.frequency.setValueAtTime(f,t);
  if(slide)o.frequency.exponentialRampToValueAtTime(Math.max(30,f+slide),t+d);
  g.gain.setValueAtTime(vol||0.05,t);
  g.gain.exponentialRampToValueAtTime(0.0001,t+d);
  o.connect(g);g.connect(ac.destination);o.start(t);o.stop(t+d);
}
const sndBoom=()=>beep(180,0.15,'sawtooth',0.05,-120);
const sndBig=()=>beep(150,0.6,'sawtooth',0.08,-110);
const sndHit=()=>beep(130,0.3,'sawtooth',0.07,-90);
const sndPick=()=>beep(600,0.12,'square',0.04,500);
const sndOver=()=>beep(220,0.7,'sawtooth',0.06,-170);
const sndWarn=()=>beep(300,0.35,'square',0.05,200);
muteBtn.addEventListener('pointerdown',e=>{
  e.stopPropagation();e.preventDefault();
  muted=!muted;try{localStorage.setItem('ss_mute',muted?'1':'0');}catch(err){}
  showMute();audioInit();
});

// ===================== State =====================
let state='title';
let px=W/2, py=H-30, score=0, best=0, lives=START_LIVES, level=1, weaponLevel=1;
let invincible=0, rapidUntil=0, lastShot=0, lastSpawn=0, overTime=0, fc=0;
let nextBoss=BOSS_EVERY, spawnCount=0;
let popupMsg='', popupT=0;
try{best=parseInt(localStorage.getItem('ss_best')||'0',10)||0;}catch(e){}

let pb=[], eb=[], en=[], booms=[], items=[];
const stars=[];
for(let i=0;i<32;i++)stars.push({x:rnd(0,W),y:rnd(0,H),s:rndi(1,4)});

function showPopup(m){popupMsg=m;popupT=45;}

function resetGame(){
  px=W/2; py=H-30; score=0; lives=START_LIVES; level=1; weaponLevel=1;
  invincible=0; rapidUntil=0; popupT=0; nextBoss=BOSS_EVERY; spawnCount=0;
  const n=performance.now(); lastShot=n; lastSpawn=n;
  pb=[];eb=[];en=[];booms=[];items=[];
}
function startGame(){audioInit();resetGame();state='play';}

// ===================== Input =====================
const keys={left:0,right:0,up:0,down:0};
let stickId=null, ox=0, oy=0, sjx=0, sjy=0;
const baseEl=document.getElementById('base'), knobEl=document.getElementById('knob');

function canRestart(){return state==='title'||(state==='over'&&performance.now()-overTime>1000);}

addEventListener('pointerdown',e=>{
  if(e.target===muteBtn)return;
  e.preventDefault();
  audioInit();
  if(canRestart())startGame();
  if(stickId!==null)return;
  stickId=e.pointerId; ox=e.clientX; oy=e.clientY; sjx=sjy=0;
  baseEl.style.display='block'; knobEl.style.display='block';
  baseEl.style.transform='translate('+ox+'px,'+oy+'px)';
  knobEl.style.transform='translate('+ox+'px,'+oy+'px)';
},{passive:false});

addEventListener('pointermove',e=>{
  if(e.pointerId!==stickId)return;
  e.preventDefault();
  let dx=e.clientX-ox, dy=e.clientY-oy;
  const d=Math.hypot(dx,dy);
  if(d>STICK_R){                       // floating stick: base follows the finger
    ox+=dx/d*(d-STICK_R); oy+=dy/d*(d-STICK_R);
    dx=e.clientX-ox; dy=e.clientY-oy;
    baseEl.style.transform='translate('+ox+'px,'+oy+'px)';
  }
  const m=Math.hypot(dx,dy)/STICK_R;
  sjx=clamp(dx/STICK_R,-1,1); sjy=clamp(dy/STICK_R,-1,1);
  if(m<0.15){sjx=0;sjy=0;}
  knobEl.style.transform='translate('+(ox+clamp(dx,-STICK_R,STICK_R))+'px,'+(oy+clamp(dy,-STICK_R,STICK_R))+'px)';
},{passive:false});

function endStick(e){
  if(e.pointerId!==stickId)return;
  stickId=null; sjx=sjy=0;
  baseEl.style.display='none'; knobEl.style.display='none';
}
addEventListener('pointerup',endStick);
addEventListener('pointercancel',endStick);
addEventListener('contextmenu',e=>e.preventDefault());

addEventListener('keydown',e=>{
  const k=e.key.toLowerCase();
  if(k==='arrowleft'||k==='a')keys.left=1;
  else if(k==='arrowright'||k==='d')keys.right=1;
  else if(k==='arrowup'||k==='w')keys.up=1;
  else if(k==='arrowdown'||k==='s')keys.down=1;
  else if(k===' '||k==='enter'){}
  else return;
  e.preventDefault();
  if(canRestart())startGame();
});
addEventListener('keyup',e=>{
  const k=e.key.toLowerCase();
  if(k==='arrowleft'||k==='a')keys.left=0;
  else if(k==='arrowright'||k==='d')keys.right=0;
  else if(k==='arrowup'||k==='w')keys.up=0;
  else if(k==='arrowdown'||k==='s')keys.down=0;
});

// ===================== Game logic =====================
function spawnBoom(x,y){if(booms.length<16)booms.push({x:x,y:y,t:0});}

function addPlayerBullet(x,y,dx){pb.push({x:x,y:y,dx:dx});}
function firePlayer(){
  const x=Math.round(px), y=Math.round(py);
  switch(weaponLevel){
    case 1: addPlayerBullet(x-1,y,0); break;
    case 2: addPlayerBullet(x-6,y+4,0); addPlayerBullet(x+5,y+4,0); break;
    case 3: addPlayerBullet(x-1,y,0); addPlayerBullet(x-5,y+3,-1); addPlayerBullet(x+3,y+3,1); break;
    default:
      addPlayerBullet(x-6,y+4,0); addPlayerBullet(x+5,y+4,0);
      addPlayerBullet(x-7,y+5,-1); addPlayerBullet(x+6,y+5,1);
  }
}

// ----- enemy bullets -----
function ebullet(x,y,dx,vy,col){if(eb.length<45)eb.push({x:x,y:y,dx:dx,vy:vy,col:col||C.green});}
function enemySpeed(){return 2.6+Math.min(2,level*0.12);}

function enemyFire(e){
  const cx=e.x+e.w/2, cy=e.y+e.h, sp=enemySpeed();
  if(e.type===1){
    ebullet(cx,cy,0,sp);
  }else if(e.type===2){                   // tank: 3-way spread
    ebullet(cx,cy,-0.9,sp*0.95);
    ebullet(cx,cy,0,sp*0.95);
    ebullet(cx,cy,0.9,sp*0.95);
  }
}

function bossFire(e){
  const cx=e.x+e.w/2, cy=e.y+e.h-4, sp=2.8+Math.min(1.2,level*0.05);
  if(e.mode===0){                         // fan of 5
    for(let i=-2;i<=2;i++){const a=i*0.28;ebullet(cx,cy,Math.sin(a)*sp,Math.cos(a)*sp,C.orange);}
  }else{                                  // aimed triple
    const base=Math.atan2(px-cx,py-cy);
    for(let i=-1;i<=1;i++){const a=base+i*0.18;ebullet(cx,cy,Math.sin(a)*sp*1.1,Math.cos(a)*sp*1.1,C.orange);}
  }
  if(e.hp<e.maxhp/2){                     // enraged: extra side shots
    ebullet(e.x+4,e.y+e.h,0,sp,C.orange);
    ebullet(e.x+e.w-4,e.y+e.h,0,sp,C.orange);
  }
}

// ----- enemies -----
function addEnemy(type,baseX,y0,amp,vy,phase){
  const hp=type===0?1+Math.floor(level/6):(type===1?3+Math.floor(level/8):6+Math.floor(level/3));
  en.push({type:type,x:baseX,y:y0,baseX:baseX,vy:vy,phase:phase,amp:amp,
    w:SIZES[type][0],h:SIZES[type][1],hp:hp,maxhp:hp,flash:0});
}
function maxEnemies(){return Math.min(6+level,12);}

function spawnEnemy(gruntOnly){
  if(en.length>=maxEnemies())return;
  let type=0;
  if(!gruntOnly){
    const r=Math.random();
    const ps=level>=2?Math.min(0.5,0.15+level*0.05):0;
    const pt=level>=3?Math.min(0.25,0.05+level*0.02):0;
    if(r<pt)type=2;else if(r<pt+ps)type=1;
  }
  const w=SIZES[type][0], h=SIZES[type][1];
  const vy=type===0?Math.min(0.9+level*0.15+rnd(0,0.4),3.0)
          :type===1?Math.min(0.7+level*0.1+rnd(0,0.3),2.0)
          :Math.min(0.5+level*0.05,1.1);
  addEnemy(type,rnd(14,W-w-14),-h,type===2?rndi(0,6):rndi(0,13),vy,rnd(0,6.28));
}

function spawnWave(){                     // V-formation of 3 grunts
  if(en.length+3>maxEnemies()+2)return;
  const cx=rnd(32,81), vy=Math.min(1.3+level*0.1,2.6);
  const offs=[[-30,-14],[0,0],[30,-14]];
  for(const o of offs)addEnemy(0,cx+o[0],-12+o[1],0,vy,0);
}

function spawnBoss(lv){
  const hp=60+25*(Math.floor(lv/BOSS_EVERY)-1);
  en.push({type:3,x:(W-48)/2,y:-32,baseX:(W-48)/2,vy:0,phase:0,amp:0,w:48,h:30,
    hp:hp,maxhp:hp,flash:0,t:0,mode:0,nextFire:performance.now()+1500});
  showPopup('WARNING! BOSS');
  sndWarn();
}

// ----- items -----
function spawnItem(x,y,type){
  if(items.length>=4)return;
  items.push({x:clamp(x,0,W-IS),y:y,type:type});     // 0=weapon 1=life 2=rapid
}
function maybeDrop(e){
  const b=DROP_BONUS[e.type]||0, r=rndi(0,100);
  const t1=DROP_W+b, t2=t1+DROP_L, t3=t2+DROP_R;
  const cx=e.x+e.w/2-IS/2, cy=e.y;
  if(r<t1)spawnItem(cx,cy,0);
  else if(r<t2){if(lives<MAX_LIVES)spawnItem(cx,cy,1);}
  else if(r<t3)spawnItem(cx,cy,2);
}
function collectItem(t){
  if(t===0){
    if(weaponLevel<MAX_WEAPON){weaponLevel++;showPopup('WEAPON UP!');}
    else{score+=50;showPopup('MAX +50');}
  }else if(t===1){
    if(lives<MAX_LIVES){lives++;showPopup('+1 LIFE');}
    else{score+=50;showPopup('MAX +50');}
  }else{
    rapidUntil=performance.now()+RAPID_MS;showPopup('RAPID FIRE!');
  }
  sndPick();
}

function killEnemy(e){
  e.hp=0;
  spawnBoom(e.x+e.w/2,e.y+e.h/2);
  if(e.type===3){
    for(let i=0;i<8;i++)spawnBoom(e.x+rnd(0,e.w),e.y+rnd(0,e.h));
    score+=200+level*5;
    showPopup('STAGE CLEAR!');
    eb=[];                                // clear enemy bullets
    spawnItem(e.x+e.w/2-15,e.y+e.h,0);
    spawnItem(e.x+e.w/2+5,e.y+e.h,lives<MAX_LIVES?1:2);
    sndBig();
  }else{
    score+=SCORE_BY[e.type];
    maybeDrop(e);
    sndBoom();
  }
}

function hitPlayer(){
  lives--;
  if(weaponLevel>1)weaponLevel--;
  spawnBoom(px,py+6);
  invincible=60;
  try{if(navigator.vibrate)navigator.vibrate(70);}catch(e){}
  if(lives<=0){
    spawnBoom(px-6,py+2);spawnBoom(px+6,py+10);
    if(score>best){best=score;try{localStorage.setItem('ss_best',String(best));}catch(e){}}
    state='over';overTime=performance.now();
    sndOver();
  }else sndHit();
}

function updateStars(k){
  for(const s of stars){s.y+=s.s*k;if(s.y>=H){s.y=0;s.x=rnd(0,W);}}
}
function updateBooms(k){
  for(const b of booms)b.t+=k;
  booms=booms.filter(b=>b.t<=9);
}

function updatePlay(k,now){
  // movement
  const jx=clamp(sjx+keys.right-keys.left,-1,1);
  const jy=clamp(sjy+keys.down-keys.up,-1,1);
  px=clamp(px+jx*PS*k,9,W-9);
  py=clamp(py+jy*PS*k,70,H-18);

  // auto fire
  const fd=now<rapidUntil?FIRE/2:FIRE;
  if(now-lastShot>=fd){lastShot=now;firePlayer();}

  // level, boss & spawning
  level=1+Math.floor(score/SCORE_PER_LEVEL);
  const boss=en.find(e=>e.type===3);
  if(level>=nextBoss&&!boss){spawnBoss(nextBoss);nextBoss+=BOSS_EVERY;}
  let interval=Math.max(320,1000-(level-1)*90);
  if(boss)interval*=2.5;
  if(now-lastSpawn>=interval){
    lastSpawn=now;spawnCount++;
    if(!boss&&level>=3&&spawnCount%6===0)spawnWave();
    else spawnEnemy(!!boss);
  }

  updateStars(k);

  // player bullets
  for(const b of pb){b.y-=6*k;b.x+=b.dx*k;}
  pb=pb.filter(b=>b.y>-6&&b.x>-3&&b.x<W+1);

  // enemies
  for(const e of en){
    if(e.flash>0)e.flash-=k;
    if(e.type===3){                        // boss movement & fire
      if(e.y<22)e.y+=1*k;
      else{
        e.t+=k;
        e.x=(W-e.w)/2+Math.sin(e.t*0.035)*((W-e.w)/2-3);
        if(now>=e.nextFire){
          bossFire(e);
          e.nextFire=now+(e.hp<e.maxhp/2?700:1000);
          e.mode^=1;
        }
      }
      continue;
    }
    e.y+=e.vy*k; e.phase+=0.07*k;
    e.x=e.baseX+Math.sin(e.phase)*e.amp;
    if((e.type===1||e.type===2)&&e.y>0&&e.y<H-70){
      const p=(e.type===1?0.012:0.01)*(1+level*0.1)*k;
      if(Math.random()<p)enemyFire(e);
    }
  }
  en=en.filter(e=>e.hp>0&&(e.type===3||e.y<=H));

  // bullet vs enemy
  for(const b of pb){
    for(const e of en){
      if(e.hp<=0)continue;
      if(hit(b.x,b.y,2,5,e.x,e.y,e.w,e.h)){
        b.y=-100;                          // consume bullet
        e.hp--;e.flash=3;
        if(e.hp<=0)killEnemy(e);
        break;
      }
    }
  }
  en=en.filter(e=>e.hp>0);
  pb=pb.filter(b=>b.y>-6);

  // enemy bullets
  for(const b of eb){
    b.x+=b.dx*k; b.y+=b.vy*k;
    if(invincible<=0&&hit(b.x-1,b.y,3,6,px-5,py+2,11,11)){
      b.y=H+100;
      hitPlayer();
      if(state!=='play')return;
    }
  }
  eb=eb.filter(b=>b.y<=H&&b.x>-4&&b.x<W+4);

  // enemy vs player (ramming): enemy takes 3 damage, player loses a life
  if(invincible<=0){
    for(const e of en){
      if(hit(px-5,py+2,11,11,e.x,e.y,e.w,e.h)){
        if(e.type!==3){e.hp-=3;e.flash=3;if(e.hp<=0)killEnemy(e);}
        hitPlayer();
        if(state!=='play')return;
        break;
      }
    }
    en=en.filter(e=>e.hp>0);
  }

  // items
  for(const it of items){
    it.y+=1.3*k;
    if(hit(px-6,py,13,14,it.x,it.y,IS,IS)){it.y=H+100;collectItem(it.type);}
  }
  items=items.filter(it=>it.y<=H);

  if(invincible>0)invincible-=k;
  if(popupT>0)popupT-=k;
  updateBooms(k);
}

// ===================== Drawing =====================
function drawStars(){
  for(const s of stars)rect(Math.floor(s.x),Math.floor(s.y),1,1,s.s===1?C.dgray:(s.s===2?C.gray:C.white));
}
function drawBooms(){
  for(const b of booms){
    const t=b.t;
    ring(b.x,b.y,t,t<5?C.yellow:C.orange);
    if(t>1)ring(b.x,b.y,t-2,C.red);
    if(t<6)disc(b.x,b.y,4-t/2,C.white);
  }
}
function drawPlayer(x,y){
  tri(x,y,x-6,y+12,x+6,y+12,C.cyan);
  rect(x-1,y+4,3,6,C.white);
  rect(x-8,y+8,3,5,C.blue);
  rect(x+6,y+8,3,5,C.blue);
  if(fc&2)tri(x-2,y+13,x+2,y+13,x,y+17,C.orange);
  else tri(x-1,y+13,x+1,y+13,x,y+15,C.yellow);
}

function drawAlien(x,y,w,h,type,f){          // grunt / shooter (scaled sprite)
  const c=f?C.white:(type?C.magenta:C.red);
  ctx.save();ctx.translate(x,y);ctx.scale(w/12,h/10);
  rect(2,0,8,6,c);rect(0,2,12,4,c);
  rect(2,6,2,4,c);rect(8,6,2,4,c);
  rect(3,2,2,2,f?C.black:C.white);rect(7,2,2,2,f?C.black:C.white);
  if(type===1)rect(5,6,2,3,C.yellow);       // cannon
  ctx.restore();
}
function drawTank(x,y,f){
  const a=f?C.white:'#e06000', d=f?C.white:'#803000';
  ctx.save();ctx.translate(x,y);
  rect(3,0,20,13,a);rect(0,4,26,9,a);rect(0,9,26,2,d);
  rect(2,13,6,7,d);rect(18,13,6,7,d);rect(11,13,4,7,a);
  rect(6,4,5,4,C.white);rect(15,4,5,4,C.white);
  rect(7,5,2,2,C.red);rect(16,5,2,2,C.red);
  ctx.restore();
}
function drawBoss(x,y,f){
  const a=f?C.white:'#7a2dbd', b=f?C.white:'#c060ff';
  ctx.save();ctx.translate(x,y);
  rect(8,0,32,22,a);rect(0,8,48,10,a);
  rect(4,18,6,12,b);rect(38,18,6,12,b);rect(20,22,8,8,b);
  rect(13,5,7,7,C.red);rect(28,5,7,7,C.red);
  rect(15,7,3,3,C.white);rect(30,7,3,3,C.white);
  disc(24,16,3,C.yellow);
  ctx.restore();
}
function drawEnemy(e){
  const x=Math.round(e.x), y=Math.round(e.y), f=e.flash>0;
  if(e.type===3)drawBoss(x,y,f);
  else if(e.type===2)drawTank(x,y,f);
  else drawAlien(x,y,e.w,e.h,e.type,f);
  if(e.type>=2||e.hp<e.maxhp){                // health bar
    rect(x,y+e.h+1,e.w,2,C.dgray);
    rect(x,y+e.h+1,e.w*Math.max(0,e.hp)/e.maxhp,2,e.type===3?C.red:C.green);
  }
}
function drawItem(x,y,type){
  if(type===1){
    disc(x+3,y+3,3,C.red);disc(x+6.5,y+3,3,C.red);
    tri(x,y+4,x+9.5,y+4,x+4.75,y+9.5,C.red);
  }else{
    rect(x,y,IS,IS,type===0?C.orange:C.cyan);
    ctx.strokeStyle=C.white;ctx.lineWidth=1;ctx.strokeRect(x+0.5,y+0.5,IS-1,IS-1);
    text(type===0?'W':'R',x+IS/2,y+1.5,C.black,8,'center');
  }
}
function drawHUD(now){
  text(String(score),2,2,C.white);
  const bossAlive=en.some(e=>e.type===3);
  text('L'+level,54,2,bossAlive?C.red:C.yellow);
  for(let i=0;i<lives;i++){const x=W-7-i*9;tri(x,2,x-3,9,x+3,9,C.cyan);}
  text('W',2,12,C.orange);
  for(let i=0;i<weaponLevel;i++)rect(10+i*5,13,4,6,C.orange);
  if(now<rapidUntil){
    text('R',36,12,C.cyan);
    rect(44,13,Math.max(0,(rapidUntil-now)*20/RAPID_MS),6,C.cyan);
  }
}
function drawPlay(now){
  rect(0,0,W,H,C.black);
  drawStars();
  for(const it of items)drawItem(Math.round(it.x),Math.round(it.y),it.type);
  for(const e of en)drawEnemy(e);
  for(const b of pb)rect(Math.round(b.x),Math.round(b.y),2,5,C.yellow);
  for(const b of eb)rect(Math.round(b.x)-1,Math.round(b.y),3,6,b.col);
  if(invincible<=0||Math.floor(invincible/4)%2===0)drawPlayer(Math.round(px),Math.round(py));
  drawBooms();
  drawHUD(now);
  if(popupT>0&&(popupT>12||Math.floor(popupT/3)%2===0))text(popupMsg,W/2,62,C.yellow,8,'center');
}
function drawTitle(){
  rect(0,0,W,H,C.black);drawStars();
  text('SPACE',W/2,20,C.cyan,16,'center');
  text('SHOOTER',W/2,40,C.yellow,16,'center');
  drawPlayer(W/2,78);
  drawItem(34,106,0);drawItem(59,106,1);drawItem(84,106,2);
  if(Math.floor(fc/15)%2===0)text('TAP TO START',W/2,128,C.white,8,'center');
  text('DRAG = MOVE   AUTO FIRE',W/2,141,C.gray,6,'center');
  if(best>0)text('BEST: '+best,W/2,150,C.gray,7,'center');
}
function drawOver(){
  rect(0,0,W,H,C.black);drawStars();drawBooms();
  text('GAME OVER',W/2,38,C.red,16,'center');
  text('SCORE: '+score,W/2,76,C.white,8,'center');
  text('LEVEL: '+level,W/2,88,C.yellow,8,'center');
  text('BEST: '+best,W/2,100,C.gray,8,'center');
  if(Math.floor(fc/15)%2===0&&performance.now()-overTime>1000)text('TAP TO RESTART',W/2,130,C.white,8,'center');
}

// ===================== Main loop =====================
let last=performance.now();
function frame(t){
  requestAnimationFrame(frame);
  let dt=t-last;last=t;
  if(dt>50)dt=50;
  const k=dt/FR;
  fc++;
  ctx.setTransform(S,0,0,S,0,0);
  if(state==='play'){updatePlay(k,t);}
  else{updateStars(k);updateBooms(k);}
  if(state==='play')drawPlay(t);
  else if(state==='title')drawTitle();
  else drawOver();
}
requestAnimationFrame(frame);
})();
</script>
</body>
</html>
)rawliteral";

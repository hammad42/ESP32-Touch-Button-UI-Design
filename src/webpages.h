
#include <Arduino.h>

#ifndef WEBPAGES_H
#define WEBPAGES_H

static const char index_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML><html><head><title>ESP REMOTE</title><meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body{font-family:Arial;text-align:center;background:#111;color:#0f0;} 
    .btn{background:#222;color:#0f0;border:1px solid #0f0;padding:15px;width:30%;margin:5px;font-weight:bold;border-radius:10px;}
    .sub-btn{background:#040;color:#fff;border:1px solid #0f0;padding:15px;width:65%;margin:5px;font-weight:bold;border-radius:10px;}
    .btn:active, .sub-btn:active{background:#0f0;color:#000;}
    #txt{padding:12px;width:80%;margin-top:10px;background:#000;color:#0f0;border:1px solid #0f0;font-size:18px;}
    #oledMirror{border:2px solid #555; background:#000; width:90%; max-width:300px; image-rendering: pixelated; margin-bottom:10px;}
  </style>
  </head><body>
    <h2>VIRTUAL OLED</h2>
    <canvas id="oledMirror" width="128" height="64"></canvas><br>
    <button class="btn" onclick="s('up')">UP</button><br>
    <button class="btn" onclick="s('sel')">SELECT</button><br>
    <button class="btn" onclick="s('down')">DOWN</button><br>
    <button class="sub-btn" onclick="f('/submit')">SUBMIT / SAVE</button>
    <hr>
    <input type="text" id="txt" placeholder="Inject Text..."><br>
    <button class="sub-btn" onclick="st()">SEND TO ESP</button>
    <script>
  const canvas = document.getElementById('oledMirror');
  const ctx = canvas.getContext('2d');

  // --- THE ESSENTIAL ENGINES ---
  function s(c){ fetch('/ctrl?c=' + c); } 
  function f(u){ fetch(u); }
  function st(){ 
    const val = document.getElementById('txt').value;
    fetch('/text?v=' + encodeURIComponent(val)); 
    document.getElementById('txt').value = ''; 
  }

  // --- THE MIRROR ENGINE (Fixed for SSD1306 Vertical Mapping) ---
  function updateMirror() {
    fetch('/screen').then(r => r.text()).then(hex => {
      ctx.fillStyle = "black"; 
      ctx.fillRect(0, 0, 128, 64);
      ctx.fillStyle = "#0af"; // Matches Blue OLED color
      
      for (let page = 0; page < 8; page++) {
        for (let x = 0; x < 128; x++) {
          let byteIdx = (page * 128) + x;
          let byte = parseInt(hex.substr(byteIdx * 2, 2), 16);
          for (let bit = 0; bit < 8; bit++) {
            if ((byte >> bit) & 0x01) {
              ctx.fillRect(x, (page * 8) + bit, 1, 1);
            }
          }
        }
      }
    }).catch(err => console.log("Mirror paused..."));
  }

  // Poll the screen every 400ms
  setInterval(updateMirror, 400);
</script>
  </body></html>)rawliteral";

#endif
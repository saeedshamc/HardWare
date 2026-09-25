#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>

// تنظیمات WiFi
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ایجاد وب سرور روی پورت 80
WebServer server(80);

// متغیرهای بازی ماشین
struct Car {
  float x = 300;           // موقعیت افقی ماشین
  float y = 600;           // موقعیت عمودی ماشین
  float speed = 0;         // سرعت ماشین
  int lane = 1;            // لاین فعلی (0=چپ، 1=وسط، 2=راست)
  float maxSpeed = 20;     // حداکثر سرعت
  float acceleration = 0.8; // شتاب
  float deceleration = 0.5; // ترمز
};

struct ScoreBox {
  float x;
  float y;
  bool collected = false;
  int value = 10;
  int size = 15;
  String color = "#f1c40f";
  String symbol = "+";
};

struct EnemyCar {
  float x;
  float y;
  float speed;
  int lane;
  int width = 30;
  int height = 50;
};

Car playerCar;
ScoreBox scoreBoxes[100];
EnemyCar enemyCars[3];
int score = 0;
bool gameRunning = false;
float roadOffset = 0;

void setup() {
  Serial.begin(115200);
  
  // اتصال به WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("در حال اتصال به WiFi...");
  }
  
  Serial.println("متصل شد!");
  Serial.print("آدرس IP: ");
  Serial.println(WiFi.localIP());
  
  // راه‌اندازی SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("خطا در راه‌اندازی SPIFFS");
    return;
  }
  
  // مقداردهی اولیه باکس‌های امتیاز و ماشین‌های دشمن
  initializeScoreBoxes();
  initializeEnemyCars();
  
  // تعریف مسیرهای API
  setupRoutes();
  
  // شروع سرور
  server.begin();
  Serial.println("سرور شروع شد");
}

void initializeScoreBoxes() {
  for (int i = 0; i < 100; i++) {
    int boxType = random(4);
    switch(boxType) {
      case 0: // طلایی
        scoreBoxes[i].value = 10;
        scoreBoxes[i].size = 15;
        scoreBoxes[i].color = "#f1c40f";
        scoreBoxes[i].symbol = "+";
        break;
      case 1: // قرمز
        scoreBoxes[i].value = 20;
        scoreBoxes[i].size = 18;
        scoreBoxes[i].color = "#e74c3c";
        scoreBoxes[i].symbol = "★";
        break;
      case 2: // بنفش
        scoreBoxes[i].value = 15;
        scoreBoxes[i].size = 16;
        scoreBoxes[i].color = "#9b59b6";
        scoreBoxes[i].symbol = "♦";
        break;
      case 3: // سبز
        scoreBoxes[i].value = 25;
        scoreBoxes[i].size = 20;
        scoreBoxes[i].color = "#2ecc71";
        scoreBoxes[i].symbol = "●";
        break;
    }
    
    scoreBoxes[i].x = random(100, 500);
    scoreBoxes[i].y = -200 - (i * 100);
    scoreBoxes[i].collected = false;
  }
}

void initializeEnemyCars() {
  for (int i = 0; i < 3; i++) {
    enemyCars[i].lane = random(3);
    enemyCars[i].x = 150 + (enemyCars[i].lane * 150);
    enemyCars[i].y = -200 - (i * 400);
    enemyCars[i].speed = 3 + random(5);
  }
}

void setupRoutes() {
  // صفحه اصلی بازی
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Content-Type", "text/html; charset=utf-8");
    server.send(200, "text/html", getCarGameHTML());
  });
  
  // API برای دریافت وضعیت بازی
  server.on("/api/game-state", HTTP_GET, []() {
    String json = "{";
    json += "\"carX\":" + String(playerCar.x) + ",";
    json += "\"carY\":" + String(playerCar.y) + ",";
    json += "\"carSpeed\":" + String(playerCar.speed) + ",";
    json += "\"carLane\":" + String(playerCar.lane) + ",";
    json += "\"score\":" + String(score) + ",";
    json += "\"gameRunning\":" + String(gameRunning ? "true" : "false") + ",";
    json += "\"roadOffset\":" + String(roadOffset);
    json += "}";
    
    server.sendHeader("Content-Type", "application/json");
    server.send(200, "application/json", json);
  });
  
  // API برای کنترل ماشین
  server.on("/api/car-control", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      String body = server.arg("plain");
      
      if (body.indexOf("\"action\":\"gas\"") != -1) {
        // گاز
        playerCar.speed = min(playerCar.maxSpeed, playerCar.speed + playerCar.acceleration);
      } else if (body.indexOf("\"action\":\"brake\"") != -1) {
        // ترمز
        playerCar.speed = max(0.0, playerCar.speed - playerCar.deceleration);
      } else if (body.indexOf("\"action\":\"left\"") != -1) {
        // تغییر لاین به چپ
        if (playerCar.lane > 0) {
          playerCar.lane--;
          playerCar.x = 150 + (playerCar.lane * 150);
        }
      } else if (body.indexOf("\"action\":\"right\"") != -1) {
        // تغییر لاین به راست
        if (playerCar.lane < 2) {
          playerCar.lane++;
          playerCar.x = 150 + (playerCar.lane * 150);
        }
      }
    }
    
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });
  
  // API برای شروع/توقف بازی
  server.on("/api/toggle-game", HTTP_POST, []() {
    gameRunning = !gameRunning;
    if (gameRunning) {
      score = 0;
      playerCar.speed = 0;
      playerCar.lane = 1;
      playerCar.x = 300;
      playerCar.y = 600;
      initializeScoreBoxes();
      initializeEnemyCars();
    }
    
    String json = "{\"gameRunning\":" + String(gameRunning ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  });
  
  // API برای به‌روزرسانی بازی
  server.on("/api/update-game", HTTP_POST, []() {
    if (gameRunning) {
      updateGame();
    }
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });
}

void updateGame() {
  // حرکت جاده
  roadOffset += playerCar.speed;
  if (roadOffset > 50) {
    roadOffset = 0;
  }
  
  // حرکت باکس‌های امتیاز
  for (int i = 0; i < 100; i++) {
    if (!scoreBoxes[i].collected) {
      scoreBoxes[i].y += playerCar.speed;
      
      // اگر باکس از صفحه خارج شد، دوباره در بالا ظاهر شود
      if (scoreBoxes[i].y > 800) {
        scoreBoxes[i].y = -200 - random(300);
        scoreBoxes[i].x = random(100, 500);
        scoreBoxes[i].collected = false;
        
        // ایجاد باکس جدید با ویژگی‌های تصادفی
        int boxType = random(4);
        switch(boxType) {
          case 0: // طلایی
            scoreBoxes[i].value = 10;
            scoreBoxes[i].size = 15;
            scoreBoxes[i].color = "#f1c40f";
            scoreBoxes[i].symbol = "+";
            break;
          case 1: // قرمز
            scoreBoxes[i].value = 20;
            scoreBoxes[i].size = 18;
            scoreBoxes[i].color = "#e74c3c";
            scoreBoxes[i].symbol = "★";
            break;
          case 2: // بنفش
            scoreBoxes[i].value = 15;
            scoreBoxes[i].size = 16;
            scoreBoxes[i].color = "#9b59b6";
            scoreBoxes[i].symbol = "♦";
            break;
          case 3: // سبز
            scoreBoxes[i].value = 25;
            scoreBoxes[i].size = 20;
            scoreBoxes[i].color = "#2ecc71";
            scoreBoxes[i].symbol = "●";
            break;
        }
      }
    }
  }
  
  // حرکت ماشین‌های دشمن
  for (int i = 0; i < 3; i++) {
    enemyCars[i].y += enemyCars[i].speed;
    
    // اگر ماشین دشمن از صفحه خارج شد، دوباره در بالا ظاهر شود
    if (enemyCars[i].y > 800) {
      enemyCars[i].y = -200;
      enemyCars[i].lane = random(3);
      enemyCars[i].x = 150 + (enemyCars[i].lane * 150);
      enemyCars[i].speed = 3 + random(5);
    }
  }
  
  // بررسی برخورد با باکس‌های امتیاز
  for (int i = 0; i < 100; i++) {
    if (!scoreBoxes[i].collected) {
      float distance = sqrt(pow(playerCar.x - scoreBoxes[i].x, 2) + pow(playerCar.y - scoreBoxes[i].y, 2));
      if (distance < 25) {
        score += scoreBoxes[i].value;
        scoreBoxes[i].collected = true;
      }
    }
  }
  
  // بررسی برخورد با ماشین‌های دشمن
  for (int i = 0; i < 3; i++) {
    float distance = sqrt(pow(playerCar.x - enemyCars[i].x, 2) + pow(playerCar.y - enemyCars[i].y, 2));
    if (distance < 30) {
      gameRunning = false; // پایان بازی
    }
  }
}

void loop() {
  server.handleClient();
  delay(2);
}

String getCarGameHTML() {
  return R"rawliteral(
<!DOCTYPE html>
<html lang="fa" dir="rtl">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>🏎️ بازی ماشین ESP32</title>
    <style>
        body {
            font-family: 'Tahoma', sans-serif;
            background: linear-gradient(135deg, #2c3e50 0%, #34495e 100%);
            margin: 0;
            padding: 10px;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            overflow-x: hidden;
        }
        
        .game-container {
            background: rgba(255, 255, 255, 0.95);
            border-radius: 20px;
            padding: 15px;
            box-shadow: 0 20px 40px rgba(0,0,0,0.3);
            text-align: center;
            max-width: 100%;
            width: 100%;
            box-sizing: border-box;
        }
        
        h1 {
            color: #2c3e50;
            margin-bottom: 15px;
            font-size: 1.8em;
        }
        
        .game-info {
            display: flex;
            justify-content: space-between;
            margin-bottom: 15px;
            font-size: 1.1em;
            font-weight: bold;
            flex-wrap: wrap;
            gap: 10px;
        }
        
        .score {
            color: #27ae60;
        }
        
        .speed {
            color: #e74c3c;
        }
        
        #gameCanvas {
            border: 3px solid #2c3e50;
            border-radius: 10px;
            background: #34495e;
            margin: 15px 0;
            display: block;
            max-width: 100%;
            width: 100%;
            height: auto;
            box-sizing: border-box;
        }
        
        .controls {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 8px;
            margin: 15px 0;
        }
        
        .control-row {
            display: flex;
            gap: 8px;
            justify-content: center;
        }
        
        button {
            background: #3498db;
            color: white;
            border: none;
            padding: 12px 16px;
            border-radius: 25px;
            cursor: pointer;
            font-size: 14px;
            font-weight: bold;
            transition: all 0.3s;
            min-width: 80px;
            min-height: 44px;
            touch-action: manipulation;
        }
        
        button:hover {
            background: #2980b9;
            transform: translateY(-2px);
        }
        
        button:active {
            transform: translateY(0);
        }
        
        .gas-btn {
            background: #27ae60;
        }
        
        .gas-btn:hover {
            background: #229954;
        }
        
        .brake-btn {
            background: #e74c3c;
        }
        
        .brake-btn:hover {
            background: #c0392b;
        }
        
        .lane-btn {
            background: #f39c12;
        }
        
        .lane-btn:hover {
            background: #e67e22;
        }
        
        .game-btn {
            background: #9b59b6;
            grid-column: 1 / -1;
            margin-top: 10px;
        }
        
        .game-btn:hover {
            background: #8e44ad;
        }
        
        .status {
            font-size: 1.1em;
            margin: 10px 0;
            padding: 10px;
            border-radius: 10px;
            font-weight: bold;
        }
        
        .status.running {
            background: #d4edda;
            color: #155724;
        }
        
        .status.stopped {
            background: #f8d7da;
            color: #721c24;
        }
        
        .instructions {
            font-size: 0.9em;
            color: #666;
            margin-top: 15px;
            line-height: 1.4;
        }
        
        /* موبایل */
        @media (max-width: 768px) {
            body {
                padding: 5px;
            }
            
            .game-container {
                padding: 10px;
                border-radius: 15px;
            }
            
            h1 {
                font-size: 1.5em;
                margin-bottom: 10px;
            }
            
            .game-info {
                font-size: 1em;
                margin-bottom: 10px;
            }
            
            #gameCanvas {
                margin: 10px 0;
                border-width: 2px;
            }
            
            .controls {
                gap: 6px;
                margin: 10px 0;
            }
            
            .control-row {
                gap: 6px;
            }
            
            button {
                padding: 10px 12px;
                font-size: 13px;
                min-width: 70px;
                min-height: 40px;
            }
            
            .instructions {
                font-size: 0.9em;
                margin: 10px 0;
            }
        }
        
        /* موبایل کوچک */
        @media (max-width: 480px) {
            .game-container {
                padding: 8px;
            }
            
            h1 {
                font-size: 1.3em;
            }
            
            .game-info {
                font-size: 0.9em;
                flex-direction: column;
                gap: 5px;
            }
            
            button {
                padding: 8px 10px;
                font-size: 12px;
                min-width: 60px;
                min-height: 36px;
            }
            
            .instructions {
                font-size: 0.8em;
            }
        }
    </style>
</head>
<body>
    <div class="game-container">
        <h1>🏎️ بازی ماشین ESP32</h1>
        
        <div class="game-info">
            <div class="score">امتیاز: <span id="score">0</span></div>
            <div class="speed">سرعت: <span id="speed">0</span></div>
        </div>
        
        <div class="status stopped" id="status">بازی متوقف است</div>
        
        <canvas id="gameCanvas" width="600" height="800"></canvas>
        
        <div class="controls">
            <div class="control-row">
                <button class="gas-btn" onclick="carControl('gas')">🚗 گاز</button>
                <button class="brake-btn" onclick="carControl('brake')">🛑 ترمز</button>
            </div>
            
            <div class="control-row">
                <button class="lane-btn" onclick="carControl('left')">⬅️ چپ</button>
                <button class="lane-btn" onclick="carControl('right')">➡️ راست</button>
            </div>
            
            <button class="game-btn" onclick="toggleGame()" id="gameToggle">شروع بازی</button>
        </div>
        
        <div class="instructions">
            <strong>راهنمای بازی:</strong><br>
            🚗 گاز: افزایش سرعت (حداکثر 20) | 🛑 ترمز: کاهش سرعت<br>
            ⬅️➡️ تغییر لاین | 🎯 جمع‌آوری 100 باکس امتیاز:<br>
            🟨 طلایی (+): 10 امتیاز | 🔴 قرمز (★): 20 امتیاز<br>
            🟣 بنفش (♦): 15 امتیاز | 🟢 سبز (●): 25 امتیاز<br>
            🚗 از ماشین‌های دشمن دوری کنید!<br>
            ⌨️ کنترل: WASD یا کلیدهای جهت‌دار | 📱 موبایل: کشیدن انگشت روی صفحه
        </div>
    </div>

    <script>
        const canvas = document.getElementById('gameCanvas');
        const ctx = canvas.getContext('2d');
        
        let gameState = {
            carX: 300,
            carY: 600,
            carSpeed: 0,
            carLane: 1,
            score: 0,
            gameRunning: false,
            roadOffset: 0
        };
        
        let scoreBoxes = [];
        let enemyCars = [];
        let gameLoopId = null;
        
        // تنظیمات ماشین
        const carSettings = {
            maxSpeed: 20,
            acceleration: 0.8,
            deceleration: 0.5,
            lanes: [150, 300, 450] // موقعیت مرکز هر لاین برای زمین 600 پیکسلی
        };
        
        // ایجاد باکس‌های امتیاز
        function createScoreBoxes() {
            scoreBoxes = [];
            for (let i = 0; i < 100; i++) {
                const boxTypes = [
                    { value: 10, size: 15, color: '#f1c40f', symbol: '+' }, // طلایی
                    { value: 20, size: 18, color: '#e74c3c', symbol: '★' }, // قرمز
                    { value: 15, size: 16, color: '#9b59b6', symbol: '♦' }, // بنفش
                    { value: 25, size: 20, color: '#2ecc71', symbol: '●' }  // سبز
                ];
                const boxType = boxTypes[Math.floor(Math.random() * boxTypes.length)];
                
                scoreBoxes.push({
                    x: carSettings.lanes[Math.floor(Math.random() * 3)],
                    y: -200 - (i * 100),
                    collected: false,
                    value: boxType.value,
                    size: boxType.size,
                    color: boxType.color,
                    symbol: boxType.symbol
                });
            }
        }
        
        // ایجاد ماشین‌های دشمن
        function createEnemyCars() {
            enemyCars = [];
            for (let i = 0; i < 3; i++) {
                enemyCars.push({
                    x: carSettings.lanes[Math.floor(Math.random() * 3)],
                    y: -200 - (i * 400),
                    speed: 3 + Math.random() * 5,
                    lane: Math.floor(Math.random() * 3),
                    width: 30,
                    height: 50
                });
            }
        }
        
        // رسم جاده
        function drawRoad() {
            ctx.fillStyle = '#34495e';
            ctx.fillRect(0, 0, canvas.width, canvas.height);
            
            // خطوط جاده
            ctx.strokeStyle = '#ecf0f1';
            ctx.lineWidth = 3;
            
            // خط وسط
            ctx.beginPath();
            for (let y = gameState.roadOffset % 50; y < canvas.height; y += 50) {
                ctx.moveTo(canvas.width / 2, y);
                ctx.lineTo(canvas.width / 2, y + 25);
            }
            ctx.stroke();
            
            // خطوط کناری
            ctx.beginPath();
            ctx.moveTo(100, 0);
            ctx.lineTo(100, canvas.height);
            ctx.moveTo(500, 0);
            ctx.lineTo(500, canvas.height);
            ctx.stroke();
        }
        
        // رسم ماشین بازیکن
        function drawPlayerCar() {
            // بدنه ماشین
            ctx.fillStyle = '#e74c3c';
            ctx.fillRect(gameState.carX - 15, gameState.carY - 25, 30, 50);
            
            // شیشه جلو
            ctx.fillStyle = '#3498db';
            ctx.fillRect(gameState.carX - 10, gameState.carY - 20, 20, 15);
            
            // چرخ‌ها
            ctx.fillStyle = '#2c3e50';
            ctx.fillRect(gameState.carX - 12, gameState.carY - 30, 6, 8);
            ctx.fillRect(gameState.carX + 6, gameState.carY - 30, 6, 8);
            ctx.fillRect(gameState.carX - 12, gameState.carY + 17, 6, 8);
            ctx.fillRect(gameState.carX + 6, gameState.carY + 17, 6, 8);
            
            // چراغ‌ها
            ctx.fillStyle = '#f1c40f';
            ctx.fillRect(gameState.carX - 8, gameState.carY - 25, 4, 3);
            ctx.fillRect(gameState.carX + 4, gameState.carY - 25, 4, 3);
        }
        
        // رسم ماشین‌های دشمن
        function drawEnemyCars() {
            enemyCars.forEach(car => {
                // بدنه ماشین دشمن
                ctx.fillStyle = '#95a5a6';
                ctx.fillRect(car.x - 15, car.y - 25, 30, 50);
                
                // شیشه جلو
                ctx.fillStyle = '#7f8c8d';
                ctx.fillRect(car.x - 10, car.y - 20, 20, 15);
                
                // چرخ‌ها
                ctx.fillStyle = '#2c3e50';
                ctx.fillRect(car.x - 12, car.y - 30, 6, 8);
                ctx.fillRect(car.x + 6, car.y - 30, 6, 8);
                ctx.fillRect(car.x - 12, car.y + 17, 6, 8);
                ctx.fillRect(car.x + 6, car.y + 17, 6, 8);
            });
        }
        
        // رسم باکس‌های امتیاز
        function drawScoreBoxes() {
            scoreBoxes.forEach(box => {
                if (!box.collected) {
                    // باکس رنگی
                    ctx.fillStyle = box.color;
                    ctx.fillRect(box.x - box.size/2, box.y - box.size/2, box.size, box.size);
                    
                    // حاشیه تیره
                    ctx.strokeStyle = '#2c3e50';
                    ctx.lineWidth = 2;
                    ctx.strokeRect(box.x - box.size/2, box.y - box.size/2, box.size, box.size);
                    
                    // نماد امتیاز
                    ctx.fillStyle = '#ffffff';
                    ctx.font = `${box.size * 0.6}px Arial`;
                    ctx.textAlign = 'center';
                    ctx.fillText(box.symbol, box.x, box.y + box.size * 0.2);
                }
            });
        }
        
        // رسم بازی
        function drawGame() {
            drawRoad();
            drawScoreBoxes();
            drawEnemyCars();
            drawPlayerCar();
        }
        
        // کنترل ماشین
        function carControl(action) {
            if (!gameState.gameRunning) return;
            
            fetch('/api/car-control', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({action: action})
            })
            .then(response => response.json())
            .then(data => {
                updateGameState();
            });
        }
        
        // شروع/توقف بازی
        function toggleGame() {
            fetch('/api/toggle-game', {
                method: 'POST'
            })
            .then(response => response.json())
            .then(data => {
                gameState.gameRunning = data.gameRunning;
                updateStatus();
                
                if (gameState.gameRunning) {
                    createScoreBoxes();
                    createEnemyCars();
                    startGameLoop();
                }
            });
        }
        
        // به‌روزرسانی وضعیت بازی
        function updateGameState() {
            fetch('/api/game-state')
            .then(response => response.json())
            .then(data => {
                gameState = data;
                updateUI();
            });
        }
        
        // به‌روزرسانی رابط کاربری
        function updateUI() {
            document.getElementById('score').textContent = gameState.score;
            document.getElementById('speed').textContent = Math.round(gameState.carSpeed);
        }
        
        // به‌روزرسانی وضعیت
        function updateStatus() {
            const statusEl = document.getElementById('status');
            const toggleBtn = document.getElementById('gameToggle');
            
            if (gameState.gameRunning) {
                statusEl.textContent = 'بازی در حال اجرا است';
                statusEl.className = 'status running';
                toggleBtn.textContent = 'توقف بازی';
            } else {
                statusEl.textContent = 'بازی متوقف است';
                statusEl.className = 'status stopped';
                toggleBtn.textContent = 'شروع بازی';
            }
        }
        
        // حلقه اصلی بازی
        function gameLoop() {
            if (gameState.gameRunning) {
                handleContinuousInput();
                fetch('/api/update-game', {
                    method: 'POST'
                })
                .then(response => response.json())
                .then(data => {
                    updateGameState();
                });
            }
            
            drawGame();
            requestAnimationFrame(gameLoop);
        }
        
        // شروع حلقه بازی
        function startGameLoop() {
            gameLoop();
        }
        
        // شروع اولیه
        updateGameState();
        gameLoop();
        
        // متغیرهای کنترل کیبورد
        let keysPressed = {};
        
        // کنترل با کیبورد
        document.addEventListener('keydown', (e) => {
            // جلوگیری از اسکرول صفحه
            if(['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', ' '].includes(e.key)) {
                e.preventDefault();
            }
            
            keysPressed[e.key] = true;
            
            if (!gameState.gameRunning) {
                if (e.key === ' ') {
                    toggleGame();
                }
                return;
            }
            
            // کنترل فوری برای کلیدهای جهت‌دار
            switch(e.key) {
                case 'ArrowUp':
                case 'w':
                case 'W':
                    carControl('gas');
                    break;
                case 'ArrowDown':
                case 's':
                case 'S':
                    carControl('brake');
                    break;
                case 'ArrowLeft':
                case 'a':
                case 'A':
                    carControl('left');
                    break;
                case 'ArrowRight':
                case 'd':
                case 'D':
                    carControl('right');
                    break;
            }
        });
        
        // رها کردن کلید
        document.addEventListener('keyup', (e) => {
            keysPressed[e.key] = false;
        });
        
        // حلقه کنترل مداوم برای کلیدهای نگه داشته شده
        function handleContinuousInput() {
            if (!gameState.gameRunning) return;
            
            // گاز مداوم
            if (keysPressed['ArrowUp'] || keysPressed['w'] || keysPressed['W']) {
                carControl('gas');
            }
            
            // ترمز مداوم
            if (keysPressed['ArrowDown'] || keysPressed['s'] || keysPressed['S']) {
                carControl('brake');
            }
        }
        
        // کنترل لمسی برای موبایل
        let touchStartX = 0;
        let touchStartY = 0;
        
        canvas.addEventListener('touchstart', (e) => {
            e.preventDefault();
            const touch = e.touches[0];
            touchStartX = touch.clientX;
            touchStartY = touch.clientY;
        });
        
        canvas.addEventListener('touchmove', (e) => {
            e.preventDefault();
        });
        
        canvas.addEventListener('touchend', (e) => {
            e.preventDefault();
            const touch = e.changedTouches[0];
            const deltaX = touch.clientX - touchStartX;
            const deltaY = touch.clientY - touchStartY;
            
            if (Math.abs(deltaX) > Math.abs(deltaY)) {
                // حرکت افقی
                if (deltaX > 30) {
                    carControl('right');
                } else if (deltaX < -30) {
                    carControl('left');
                }
            } else {
                // حرکت عمودی
                if (deltaY > 30) {
                    carControl('brake');
                } else if (deltaY < -30) {
                    carControl('gas');
                }
            }
        });
    </script>
</body>
</html>
)rawliteral";
}

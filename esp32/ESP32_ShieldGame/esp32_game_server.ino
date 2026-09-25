#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>

// تنظیمات WiFi
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ایجاد وب سرور روی پورت 80
WebServer server(80);

// ساختار بازیکن
struct Player {
    int x = 250;
    int y = 250;
    int speed = 5;
};

// ساختار آیتم
struct Item {
    int x;
    int y;
    int size = 20;
    String color;
    int value;
    String symbol;
    float pulse = 0;
};

// ساختار دشمن
struct Enemy {
    int x;
    int y;
    int size = 25;
    float speed;
    float direction;
    String color = "#e74c3c";
};

// ساختار قدرت‌ها
struct PowerUps {
    int shield = 0;
};

// متغیرهای بازی
Player player;
std::vector<Item> items;
std::vector<Enemy> enemies;
int score = 0;
int level = 1;
bool gameRunning = false;
PowerUps powerUps;

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
  
  // راه‌اندازی SPIFFS برای فایل‌های استاتیک
  if (!SPIFFS.begin(true)) {
    Serial.println("خطا در راه‌اندازی SPIFFS");
    return;
  }
  
  // تعریف مسیرهای API
  setupRoutes();
  
  // شروع سرور
  server.begin();
  Serial.println("سرور شروع شد");
}

void setupRoutes() {
  // صفحه اصلی بازی
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Content-Type", "text/html; charset=utf-8");
    server.send(200, "text/html", getGameHTML());
  });
  
  // API برای دریافت وضعیت بازی
  server.on("/api/game-state", HTTP_GET, []() {
    String json = "{";
    json += "\"playerX\":" + String(player.x) + ",";
    json += "\"playerY\":" + String(player.y) + ",";
    json += "\"score\":" + String(score) + ",";
    json += "\"level\":" + String(level) + ",";
    json += "\"gameRunning\":" + String(gameRunning ? "true" : "false") + ",";
    json += "\"shield\":" + String(powerUps.shield) + ",";
    
    // آیتم‌ها
    json += "\"items\":[";
    for(size_t i = 0; i < items.size(); i++) {
      if(i > 0) json += ",";
      json += "{";
      json += "\"x\":" + String(items[i].x) + ",";
      json += "\"y\":" + String(items[i].y) + ",";
      json += "\"size\":" + String(items[i].size) + ",";
      json += "\"color\":\"" + items[i].color + "\",";
      json += "\"value\":" + String(items[i].value) + ",";
      json += "\"symbol\":\"" + items[i].symbol + "\",";
      json += "\"pulse\":" + String(items[i].pulse);
      json += "}";
    }
    json += "],";
    
    // دشمنان
    json += "\"enemies\":[";
    for(size_t i = 0; i < enemies.size(); i++) {
      if(i > 0) json += ",";
      json += "{";
      json += "\"x\":" + String(enemies[i].x) + ",";
      json += "\"y\":" + String(enemies[i].y) + ",";
      json += "\"size\":" + String(enemies[i].size) + ",";
      json += "\"speed\":" + String(enemies[i].speed) + ",";
      json += "\"direction\":" + String(enemies[i].direction) + ",";
      json += "\"color\":\"" + enemies[i].color + "\"";
      json += "}";
    }
    json += "]";
    
    json += "}";
    
    server.sendHeader("Content-Type", "application/json");
    server.send(200, "application/json", json);
  });
  
  // API برای حرکت بازیکن
  server.on("/api/move", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      String body = server.arg("plain");
      
      // تجزیه JSON ساده
      if (body.indexOf("\"direction\":\"left\"") != -1) {
        player.x = max(0, player.x - player.speed);
      } else if (body.indexOf("\"direction\":\"right\"") != -1) {
        player.x = min(470, player.x + player.speed);
      } else if (body.indexOf("\"direction\":\"up\"") != -1) {
        player.y = max(0, player.y - player.speed);
      } else if (body.indexOf("\"direction\":\"down\"") != -1) {
        player.y = min(470, player.y + player.speed);
      }
    }
    
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });
  
  // API برای شروع/توقف بازی
  server.on("/api/toggle-game", HTTP_POST, []() {
    gameRunning = !gameRunning;
    if (gameRunning) {
      // بازنشانی متغیرها
      score = 0;
      level = 1;
      player.x = 250;
      player.y = 250;
      powerUps.shield = 0;
      items.clear();
      enemies.clear();
    }
    
    String json = "{\"gameRunning\":" + String(gameRunning ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  });
  
  // API برای افزایش امتیاز
  server.on("/api/score", HTTP_POST, []() {
    if (gameRunning) {
      score += 10;
    }
    server.send(200, "application/json", "{\"score\":" + String(score) + "}");
  });
  
  // API برای فعال کردن سپر
  server.on("/api/shield", HTTP_POST, []() {
    if (gameRunning && score >= 50) {
      powerUps.shield = 600; // 10 ثانیه (600 فریم)
      score -= 50;
      server.send(200, "application/json", "{\"status\":\"ok\",\"shield\":" + String(powerUps.shield) + "}");
    } else {
      server.send(400, "application/json", "{\"error\":\"امتیاز کافی نیست یا بازی متوقف است\"}");
    }
  });
}

// تابع ایجاد آیتم جدید
void createItem() {
  if (gameRunning) {
    Item item;
    item.x = random(0, 470);
    item.y = random(0, 470);
    
    // انتخاب تصادفی نوع آیتم
    int type = random(0, 4);
    switch(type) {
      case 0:
        item.color = "#f39c12";
        item.value = 10;
        item.symbol = "⭐";
        break;
      case 1:
        item.color = "#e74c3c";
        item.value = 20;
        item.symbol = "💎";
        break;
      case 2:
        item.color = "#9b59b6";
        item.value = 15;
        item.symbol = "💜";
        break;
      case 3:
        item.color = "#2ecc71";
        item.value = 25;
        item.symbol = "💚";
        break;
    }
    
    items.push_back(item);
  }
}

// تابع ایجاد دشمن جدید
void createEnemy() {
  if (gameRunning && level > 1) {
    Enemy enemy;
    enemy.x = random(0, 470);
    enemy.y = random(0, 470);
    enemy.speed = 1 + random(0, 20) / 10.0;
    enemy.direction = random(0, 628) / 100.0; // 0 تا 2π
    enemies.push_back(enemy);
  }
}

// تابع به‌روزرسانی بازی
void updateGame() {
  if (!gameRunning) return;
  
  // کاهش زمان سپر
  if (powerUps.shield > 0) powerUps.shield--;
  
  // حرکت دشمنان
  for (size_t i = 0; i < enemies.size(); i++) {
    enemies[i].x += cos(enemies[i].direction) * enemies[i].speed;
    enemies[i].y += sin(enemies[i].direction) * enemies[i].speed;
    
    // برگشت از لبه‌ها
    if (enemies[i].x < 0 || enemies[i].x > 470) {
      enemies[i].direction = PI - enemies[i].direction;
    }
    if (enemies[i].y < 0 || enemies[i].y > 470) {
      enemies[i].direction = -enemies[i].direction;
    }
  }
  
  // بررسی برخورد با دشمنان
  for (size_t i = 0; i < enemies.size(); i++) {
    float distance = sqrt(
      pow(player.x + 15 - (enemies[i].x + enemies[i].size/2), 2) +
      pow(player.y + 15 - (enemies[i].y + enemies[i].size/2), 2)
    );
    
    if (distance < 25) {
      if (powerUps.shield > 0) {
        // با سپر: دشمن نابود می‌شود و امتیاز اضافه می‌شود
        score += 30;
        enemies.erase(enemies.begin() + i);
        i--;
      } else {
        // بدون سپر: امتیاز کم می‌شود
        score = max(0, score - 50);
        enemies.erase(enemies.begin() + i);
        i--;
      }
    }
  }
  
  // بررسی برخورد با آیتم‌ها
  for (size_t i = 0; i < items.size(); i++) {
    float distance = sqrt(
      pow(player.x + 15 - (items[i].x + items[i].size/2), 2) +
      pow(player.y + 15 - (items[i].y + items[i].size/2), 2)
    );
    
    if (distance < 25) {
      score += items[i].value;
      items.erase(items.begin() + i);
      i--;
    }
  }
  
  // افزایش سطح
  int newLevel = score / 100 + 1;
  if (newLevel > level) {
    level = newLevel;
  }
  
  // ایجاد آیتم‌های جدید
  static int itemTimer = 0;
  itemTimer++;
  if (itemTimer >= 60) { // هر 1 ثانیه
    createItem();
    itemTimer = 0;
  }
  
  // ایجاد دشمنان جدید
  static int enemyTimer = 0;
  if (level > 1) {
    enemyTimer++;
    if (enemyTimer >= 120) { // هر 2 ثانیه
      createEnemy();
      enemyTimer = 0;
    }
  }
}

void loop() {
  server.handleClient();
  updateGame();
  delay(16); // حدود 60 FPS
}

String getGameHTML() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="fa" dir="rtl">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>🎮 بازی اول ESP32 - نسخه کامل</title>
    <style>
        /* متغیرهای CSS برای سفارشی‌سازی آسان */
        :root {
            --primary-color: #FF6B6B;
            --secondary-color: #4ECDC4;
            --accent-color: #45B7D1;
            --success-color: #27ae60;
            --warning-color: #f39c12;
            --danger-color: #e74c3c;
            --info-color: #3498db;
            --dark-color: #2c3e50;
            --light-color: #ecf0f1;
            
            --border-radius: 20px;
            --box-shadow: 0 20px 40px rgba(0,0,0,0.1);
            --transition: all 0.3s ease;
            
            --font-family: 'Tahoma', sans-serif;
            --font-size-large: 2.2em;
            --font-size-medium: 1.2em;
            --font-size-small: 0.9em;
        }

        /* استایل‌های اصلی */
        body {
            font-family: var(--font-family);
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
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
            border-radius: var(--border-radius);
            padding: 20px;
            box-shadow: var(--box-shadow);
            text-align: center;
            max-width: 100%;
            width: 100%;
            box-sizing: border-box;
            position: relative;
        }

        h1 {
            color: var(--dark-color);
            margin-bottom: 20px;
            font-size: var(--font-size-large);
            text-shadow: 2px 2px 4px rgba(0,0,0,0.1);
        }

        /* دکمه راهنما */
        .help-btn {
            position: absolute;
            top: 20px;
            left: 20px;
            background: linear-gradient(45deg, var(--info-color), #2980b9);
            color: white;
            border: none;
            padding: 10px 15px;
            border-radius: 25px;
            cursor: pointer;
            font-size: 14px;
            font-weight: bold;
            transition: var(--transition);
            box-shadow: 0 4px 8px rgba(0,0,0,0.2);
            z-index: 1000;
        }

        .help-btn:hover {
            background: linear-gradient(45deg, #2980b9, #1f618d);
            transform: translateY(-2px);
            box-shadow: 0 6px 12px rgba(0,0,0,0.3);
        }

        /* باکس راهنما */
        .help-overlay {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: rgba(0, 0, 0, 0.7);
            display: none;
            justify-content: center;
            align-items: center;
            z-index: 2000;
        }

        .help-box {
            background: white;
            border-radius: 20px;
            padding: 30px;
            max-width: 90%;
            max-height: 80%;
            overflow-y: auto;
            box-shadow: 0 20px 40px rgba(0,0,0,0.3);
            position: relative;
            animation: slideIn 0.3s ease-out;
        }

        @keyframes slideIn {
            from {
                opacity: 0;
                transform: scale(0.8) translateY(-50px);
            }
            to {
                opacity: 1;
                transform: scale(1) translateY(0);
            }
        }

        .help-close {
            position: absolute;
            top: 15px;
            right: 20px;
            background: var(--danger-color);
            color: white;
            border: none;
            width: 30px;
            height: 30px;
            border-radius: 50%;
            cursor: pointer;
            font-size: 18px;
            font-weight: bold;
            transition: var(--transition);
        }

        .help-close:hover {
            background: #c0392b;
            transform: scale(1.1);
        }

        .help-title {
            color: var(--dark-color);
            font-size: 1.8em;
            margin-bottom: 20px;
            text-align: center;
        }

        .help-section {
            margin: 20px 0;
            padding: 15px;
            background: #f8f9fa;
            border-radius: 10px;
            border-left: 4px solid var(--info-color);
        }

        .help-section h3 {
            color: var(--dark-color);
            margin-bottom: 10px;
            font-size: 1.2em;
        }

        .help-section p {
            color: #666;
            line-height: 1.6;
            margin: 5px 0;
        }

        .help-section ul {
            color: #666;
            line-height: 1.6;
            margin: 10px 0;
            padding-right: 20px;
        }

        .help-section li {
            margin: 5px 0;
        }

        /* اطلاعات بازی */
        .game-info {
            display: flex;
            justify-content: space-between;
            margin-bottom: 15px;
            font-size: var(--font-size-medium);
            font-weight: bold;
            flex-wrap: wrap;
            gap: 10px;
        }

        .score {
            color: var(--success-color);
            background: rgba(39, 174, 96, 0.1);
            padding: 8px 16px;
            border-radius: 20px;
        }

        .level {
            color: var(--danger-color);
            background: rgba(231, 76, 60, 0.1);
            padding: 8px 16px;
            border-radius: 20px;
        }

        /* وضعیت بازی */
        .status {
            font-size: 1.1em;
            margin: 15px 0;
            padding: 12px;
            border-radius: 15px;
            font-weight: bold;
        }

        .status.running {
            background: linear-gradient(45deg, #d4edda, #c3e6cb);
            color: #155724;
            border: 2px solid #28a745;
        }

        .status.stopped {
            background: linear-gradient(45deg, #f8d7da, #f5c6cb);
            color: #721c24;
            border: 2px solid #dc3545;
        }

        /* کانوس بازی */
        #gameCanvas {
            border: 4px solid var(--dark-color);
            border-radius: 15px;
            background: linear-gradient(45deg, #f0f8ff, #e6f3ff);
            margin: 20px 0;
            display: block;
            max-width: 100%;
            width: 100%;
            height: auto;
            box-sizing: border-box;
            box-shadow: 0 8px 16px rgba(0,0,0,0.2);
            aspect-ratio: 1;
            object-fit: contain;
        }

        /* قدرت‌ها */
        .power-ups {
            display: flex;
            justify-content: center;
            gap: 10px;
            margin: 15px 0;
            flex-wrap: wrap;
        }

        .power-up {
            width: 20px;
            height: 20px;
            border-radius: 50%;
            border: 2px solid var(--dark-color);
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 12px;
        }

        .power-up.speed {
            background: var(--warning-color);
        }

        .power-up.score {
            background: var(--danger-color);
        }

        .power-up.shield {
            background: var(--info-color);
        }

        /* کنترل‌ها */
        .controls {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
            margin: 20px 0;
            max-width: 100%;
        }

        .control-row {
            display: flex;
            gap: 8px;
            justify-content: center;
            flex-wrap: wrap;
        }

        /* جوی استیک برای موبایل */
        .joystick-container {
            display: none;
            position: relative;
            width: 120px;
            height: 120px;
            margin: 20px auto;
        }

        .joystick-base {
            width: 120px;
            height: 120px;
            border-radius: 50%;
            background: linear-gradient(45deg, #34495e, #2c3e50);
            border: 3px solid #ecf0f1;
            position: relative;
            box-shadow: 0 8px 16px rgba(0,0,0,0.3);
        }

        .joystick-knob {
            width: 50px;
            height: 50px;
            border-radius: 50%;
            background: linear-gradient(45deg, #e74c3c, #c0392b);
            border: 2px solid #fff;
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            cursor: pointer;
            box-shadow: 0 4px 8px rgba(0,0,0,0.2);
            transition: all 0.1s ease;
        }

        .joystick-knob:active {
            transform: translate(-50%, -50%) scale(0.95);
        }

        /* دکمه سپر موبایل */
        .mobile-shield-btn {
            display: none;
            background: linear-gradient(45deg, #3498db, #2980b9);
            color: white;
            border: none;
            padding: 15px 25px;
            border-radius: 25px;
            font-size: 16px;
            font-weight: bold;
            margin: 10px auto;
            min-width: 120px;
            min-height: 50px;
            box-shadow: 0 6px 12px rgba(0,0,0,0.3);
            transition: all 0.3s ease;
        }

        .mobile-shield-btn:active {
            transform: scale(0.95);
            box-shadow: 0 3px 6px rgba(0,0,0,0.3);
        }

        .mobile-shield-btn:disabled {
            background: linear-gradient(45deg, #95a5a6, #7f8c8d);
            cursor: not-allowed;
            opacity: 0.6;
        }

        /* دکمه‌ها */
        button {
            background: linear-gradient(45deg, var(--success-color), #45a049);
            color: white;
            border: none;
            padding: 12px 16px;
            border-radius: 25px;
            cursor: pointer;
            font-size: 14px;
            font-weight: bold;
            transition: var(--transition);
            min-width: 80px;
            min-height: 44px;
            touch-action: manipulation;
            box-shadow: 0 4px 8px rgba(0,0,0,0.2);
            user-select: none;
            -webkit-user-select: none;
            -webkit-tap-highlight-color: transparent;
        }

        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 6px 12px rgba(0,0,0,0.3);
        }

        button:active {
            transform: translateY(0);
        }

        .direction-btn {
            background: linear-gradient(45deg, var(--info-color), #2980b9);
        }

        .direction-btn:hover {
            background: linear-gradient(45deg, #2980b9, #1f618d);
        }

        .action-btn {
            background: linear-gradient(45deg, var(--danger-color), #c0392b);
        }

        .action-btn:hover {
            background: linear-gradient(45deg, #c0392b, #a93226);
        }

        .game-btn {
            background: linear-gradient(45deg, #9b59b6, #8e44ad);
            grid-column: 1 / -1;
            margin-top: 10px;
            font-size: 16px;
        }

        .game-btn:hover {
            background: linear-gradient(45deg, #8e44ad, #7d3c98);
        }

        /* راهنما */
        .instructions {
            font-size: var(--font-size-small);
            color: #666;
            margin-top: 15px;
            line-height: 1.4;
            background: rgba(102, 102, 102, 0.1);
            padding: 15px;
            border-radius: 10px;
        }

        /* ریسپانسیو - موبایل */
        @media (max-width: 768px) {
            body {
                padding: 5px;
                min-height: 100vh;
                overflow-x: hidden;
            }
            
            .game-container {
                padding: 15px;
                border-radius: 15px;
                margin: 0;
                width: 100%;
                box-sizing: border-box;
            }
            
            .help-btn {
                top: 15px;
                left: 15px;
                padding: 8px 12px;
                font-size: 12px;
                z-index: 1001;
            }
            
            .help-box {
                padding: 20px;
                max-width: 95%;
                max-height: 85%;
                margin: 10px;
            }
            
            h1 {
                font-size: 1.8em;
                margin-bottom: 15px;
                line-height: 1.2;
            }
            
            h2 {
                font-size: 1em !important;
                margin-bottom: 15px !important;
            }
            
            .game-info {
                font-size: 1em;
                margin-bottom: 10px;
                flex-direction: column;
                gap: 8px;
                align-items: center;
            }
            
            .score, .level {
                padding: 6px 12px;
                font-size: 0.9em;
            }
            
            #gameCanvas {
                margin: 15px 0;
                border-width: 3px;
                width: 100% !important;
                height: auto !important;
                max-width: 100%;
            }
            
            .controls {
                gap: 8px;
                margin: 15px 0;
                grid-template-columns: 1fr;
            }
            
            .control-row {
                gap: 6px;
                justify-content: center;
                flex-wrap: wrap;
            }
            
            button {
                padding: 10px 12px;
                font-size: 13px;
                min-width: 70px;
                min-height: 40px;
                flex: 1;
                max-width: 120px;
            }
            
            .game-btn {
                grid-column: 1;
                max-width: 100%;
                margin-top: 10px;
            }
            
            .instructions {
                font-size: 0.9em;
                margin: 10px 0;
                padding: 12px;
                line-height: 1.3;
            }
            
            .power-ups {
                margin: 10px 0;
                gap: 8px;
            }
            
            .power-up {
                width: 18px;
                height: 18px;
                font-size: 10px;
            }
            
            /* نمایش جوی استیک و دکمه سپر در موبایل */
            .joystick-container {
                display: block;
            }
            
            .mobile-shield-btn {
                display: block;
            }
            
            /* مخفی کردن کنترل‌های دسکتاپ در موبایل */
            .controls {
                display: none;
            }
        }

        /* ریسپانسیو - موبایل کوچک */
        @media (max-width: 480px) {
            body {
                padding: 2px;
            }
            
            .game-container {
                padding: 10px;
                margin: 0;
                border-radius: 10px;
            }
            
            .help-btn {
                top: 10px;
                left: 10px;
                padding: 6px 10px;
                font-size: 11px;
            }
            
            .help-box {
                padding: 15px;
                max-width: 98%;
                max-height: 90%;
                margin: 5px;
            }
            
            h1 {
                font-size: 1.5em;
                margin-bottom: 10px;
            }
            
            h2 {
                font-size: 0.9em !important;
                margin-bottom: 10px !important;
            }
            
            .game-info {
                font-size: 0.9em;
                flex-direction: column;
                gap: 5px;
                margin-bottom: 8px;
            }
            
            .score, .level {
                padding: 5px 10px;
                font-size: 0.8em;
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
                gap: 4px;
            }
            
            button {
                padding: 8px 10px;
                font-size: 12px;
                min-width: 60px;
                min-height: 36px;
                max-width: 100px;
            }
            
            .game-btn {
                font-size: 14px;
                padding: 10px 15px;
            }
            
            .instructions {
                font-size: 0.8em;
                margin: 8px 0;
                padding: 10px;
            }
            
            .power-ups {
                margin: 8px 0;
                gap: 6px;
            }
            
            .power-up {
                width: 16px;
                height: 16px;
                font-size: 9px;
            }
            
            .status {
                font-size: 1em;
                padding: 10px;
                margin: 10px 0;
            }
            
            /* تنظیمات جوی استیک برای موبایل کوچک */
            .joystick-container {
                width: 100px;
                height: 100px;
                margin: 15px auto;
            }
            
            .joystick-base {
                width: 100px;
                height: 100px;
            }
            
            .joystick-knob {
                width: 40px;
                height: 40px;
            }
            
            .mobile-shield-btn {
                padding: 12px 20px;
                font-size: 14px;
                min-width: 100px;
                min-height: 45px;
            }
        }

        /* انیمیشن‌های اضافی */
        @keyframes pulse {
            0% { transform: scale(1); }
            50% { transform: scale(1.05); }
            100% { transform: scale(1); }
        }

        .power-up.active {
            animation: pulse 1s infinite;
        }

        @keyframes fadeIn {
            from { opacity: 0; transform: translateY(20px); }
            to { opacity: 1; transform: translateY(0); }
        }

        .game-container {
            animation: fadeIn 0.5s ease-out;
        }
        
        /* بهبود نمایش در موبایل */
        @media (max-width: 768px) {
            .game-container {
                animation: none;
            }
            
            /* بهبود عملکرد لمسی */
            * {
                -webkit-tap-highlight-color: transparent;
                -webkit-touch-callout: none;
                -webkit-user-select: none;
                -khtml-user-select: none;
                -moz-user-select: none;
                -ms-user-select: none;
                user-select: none;
            }
            
            /* بهبود نمایش متن */
            body {
                -webkit-text-size-adjust: 100%;
                -ms-text-size-adjust: 100%;
                text-size-adjust: 100%;
            }
        }
    </style>
</head>
<body>
    <div class="game-container">
        <!-- دکمه راهنما -->
        <button class="help-btn" onclick="showHelp()">❓ راهنما</button>
        
        <h1>🎮 بازی اول ESP32</h1>
        <h2 style="color: #666; font-size: 1.2em; margin-bottom: 20px;">نسخه کامل و بهبود یافته</h2>
        
        <div class="game-info">
            <div class="score">امتیاز: <span id="score">0</span></div>
            <div class="level">سطح: <span id="level">1</span></div>
        </div>
        
        <div class="status stopped" id="status">بازی متوقف است</div>
        
        <canvas id="gameCanvas" width="500" height="500"></canvas>
        
        <div class="power-ups">
            <div class="power-up speed" title="سرعت">⚡</div>
            <div class="power-up score" title="امتیاز">⭐</div>
            <div class="power-up shield" title="محافظت">🛡️</div>
        </div>
        
        <div class="controls">
            <div class="control-row">
                <button class="direction-btn" onclick="movePlayer('up')">⬆️ بالا</button>
            </div>
            
            <div class="control-row">
                <button class="direction-btn" onclick="movePlayer('left')">⬅️ چپ</button>
                <button class="direction-btn" onclick="movePlayer('right')">➡️ راست</button>
            </div>
            
            <div class="control-row">
                <button class="direction-btn" onclick="movePlayer('down')">⬇️ پایین</button>
            </div>
            
            <div class="control-row">
                <button class="action-btn" onclick="collectItem()">🎯 جمع‌آوری</button>
                <button class="action-btn" onclick="usePowerUp()">⚡ قدرت</button>
            </div>
            
            <button class="game-btn" onclick="toggleGame()" id="gameToggle">شروع بازی</button>
        </div>
        
        <!-- جوی استیک برای موبایل -->
        <div class="joystick-container">
            <div class="joystick-base">
                <div class="joystick-knob" id="joystickKnob"></div>
            </div>
        </div>
        
        <!-- دکمه سپر موبایل -->
        <button class="mobile-shield-btn" id="mobileShieldBtn" onclick="usePowerUp()">
            🛡️ سپر
        </button>
        
        <div class="instructions">
            <strong>راهنمای سریع:</strong><br>
            🎮 حرکت: دکمه‌های جهت‌دار یا WASD<br>
            🎯 جمع‌آوری: دکمه جمع‌آوری یا Space<br>
            ⚡ سپر: 50 امتیاز - 10 ثانیه محافظت<br>
            🏆 هدف: جمع‌آوری آیتم‌ها و نابودی دشمنان<br>
            📱 موبایل: جوی استیک برای حرکت و دکمه سپر
        </div>
    </div>

    <!-- باکس راهنما -->
    <div class="help-overlay" id="helpOverlay">
        <div class="help-box">
            <button class="help-close" onclick="hideHelp()">×</button>
            <h2 class="help-title">📖 راهنمای کامل بازی</h2>
            
            <div class="help-section">
                <h3>🎮 نحوه بازی</h3>
                <p>هدف اصلی بازی جمع‌آوری آیتم‌های رنگی برای کسب امتیاز است. با افزایش امتیاز، سطح بازی بالا می‌رود و چالش‌های جدیدی ظاهر می‌شوند.</p>
            </div>
            
            <div class="help-section">
                <h3>🎯 انواع آیتم‌ها</h3>
                <ul>
                    <li><strong>⭐ آیتم طلایی:</strong> 10 امتیاز - آیتم پایه</li>
                    <li><strong>💎 آیتم قرمز:</strong> 20 امتیاز - آیتم باارزش</li>
                    <li><strong>💜 آیتم بنفش:</strong> 15 امتیاز - آیتم متوسط</li>
                    <li><strong>💚 آیتم سبز:</strong> 25 امتیاز - آیتم نادر</li>
                </ul>
            </div>
            
            <div class="help-section">
                <h3>🎮 کنترل‌ها</h3>
                <p><strong>دسکتاپ:</strong></p>
                <ul>
                    <li>حرکت: کلیدهای جهت‌دار یا WASD</li>
                    <li>جمع‌آوری: دکمه جمع‌آوری یا Space</li>
                    <li>قدرت: دکمه قدرت یا کلید P</li>
                </ul>
                <p><strong>موبایل:</strong></p>
                <ul>
                    <li>حرکت: کشیدن انگشت روی صفحه</li>
                    <li>عملیات: دکمه‌های روی صفحه</li>
                </ul>
            </div>
            
            <div class="help-section">
                <h3>⚡ سیستم قدرت‌ها</h3>
                <p>با داشتن حداقل 50 امتیاز می‌توانید از قدرت سپر استفاده کنید:</p>
                <ul>
                    <li><strong>سپر محافظت:</strong> برای 10 ثانیه فعال می‌شود</li>
                    <li><strong>تأثیر سپر:</strong> با برخورد به دشمنان، آن‌ها از بین می‌روند و 30 امتیاز اضافه می‌شود</li>
                    <li><strong>هزینه:</strong> 50 امتیاز</li>
                </ul>
                <p>هنگام فعال بودن سپر، دایره آبی دور بازیکن نمایش داده می‌شود.</p>
            </div>
            
            <div class="help-section">
                <h3>🏆 سیستم سطح</h3>
                <p>با کسب هر 100 امتیاز، سطح بازی یک درجه بالا می‌رود. از سطح 2 به بعد، دشمنان قرمز رنگ ظاهر می‌شوند که باید از آن‌ها دوری کنید.</p>
            </div>
            
            <div class="help-section">
                <h3>🎨 ویژگی‌های بصری</h3>
                <ul>
                    <li>آیتم‌های پالسی با انیمیشن</li>
                    <li>ذرات انفجاری هنگام جمع‌آوری</li>
                    <li>سایه‌ها و عمق بصری</li>
                    <li>پس‌زمینه متحرک با خطوط شبکه</li>
                </ul>
            </div>
            
            <div class="help-section">
                <h3>💡 نکات مهم</h3>
                <ul>
                    <li>از دشمنان دوری کنید تا امتیاز از دست ندهید</li>
                    <li>از قدرت سپر برای نابودی دشمنان و کسب امتیاز استفاده کنید</li>
                    <li>آیتم‌های سبز نادرتر و باارزش‌تر هستند</li>
                    <li>با سپر فعال، دشمنان را نابود کنید و 30 امتیاز کسب کنید</li>
                    <li>بازی در تمام دستگاه‌ها قابل اجرا است</li>
                </ul>
            </div>
        </div>
    </div>

    <script>
        // تنظیم Canvas برای موبایل
        function resizeCanvas() {
            const container = canvas.parentElement;
            const containerWidth = container.clientWidth - 40;
            const maxWidth = Math.min(containerWidth, 500);
            const aspectRatio = 500 / 500;
            const newHeight = maxWidth / aspectRatio;
            
            // تنظیم اندازه کانوس
            canvas.style.width = maxWidth + 'px';
            canvas.style.height = newHeight + 'px';
            
            // تنظیم اندازه واقعی کانوس برای رسم
            const scale = maxWidth / 500;
            canvas.width = 500;
            canvas.height = 500;
            
            // تنظیم context برای مقیاس‌بندی
            const ctx = canvas.getContext('2d');
            ctx.scale(scale, scale);
        }
        
        // تنظیم اولیه
        resizeCanvas();
        
        // تنظیم مجدد هنگام تغییر اندازه پنجره
        window.addEventListener('resize', resizeCanvas);
        
        // تنظیم مجدد هنگام تغییر جهت صفحه
        window.addEventListener('orientationchange', () => {
            setTimeout(() => {
                resizeCanvas();
                initJoystick();
            }, 100);
        });
        
        let gameState = {
            playerX: 250,
            playerY: 250,
            score: 0,
            level: 1,
            gameRunning: false,
            playerSpeed: 5,
            powerUps: {
                shield: 0
            }
        };
        
        let items = [];
        let enemies = [];
        let particles = [];
        let gameLoopId = null;
        let itemSpawnTimer = 0;
        let enemySpawnTimer = 0;
        let keysPressed = {};
        
        // متغیرهای جوی استیک
        let joystickActive = false;
        let joystickCenterX = 0;
        let joystickCenterY = 0;
        let joystickRadius = 0;
        
        // ایجاد آیتم‌های تصادفی
        function createItem() {
            if (gameState.gameRunning) {
                const itemTypes = [
                    { color: '#f39c12', value: 10, symbol: '⭐' },
                    { color: '#e74c3c', value: 20, symbol: '💎' },
                    { color: '#9b59b6', value: 15, symbol: '💜' },
                    { color: '#2ecc71', value: 25, symbol: '💚' }
                ];
                
                const itemType = itemTypes[Math.floor(Math.random() * itemTypes.length)];
                
                items.push({
                    x: Math.random() * (canvas.width - 30),
                    y: Math.random() * (canvas.height - 30),
                    size: 20,
                    color: itemType.color,
                    value: itemType.value,
                    symbol: itemType.symbol,
                    pulse: 0
                });
            }
        }
        
        // ایجاد دشمنان
        function createEnemy() {
            if (gameState.gameRunning && gameState.level > 1) {
                enemies.push({
                    x: Math.random() * (canvas.width - 25),
                    y: Math.random() * (canvas.height - 25),
                    size: 25,
                    speed: 1 + Math.random() * 2,
                    direction: Math.random() * Math.PI * 2,
                    color: '#e74c3c'
                });
            }
        }
        
        // ایجاد ذرات
        function createParticles(x, y, color) {
            for (let i = 0; i < 10; i++) {
                particles.push({
                    x: x,
                    y: y,
                    vx: (Math.random() - 0.5) * 10,
                    vy: (Math.random() - 0.5) * 10,
                    life: 30,
                    color: color
                });
            }
        }
        
        // رسم بازی
        function drawGame() {
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            
            drawBackground();
            drawItems();
            drawEnemies();
            drawPlayer();
            drawParticles();
            
            checkCollisions();
        }
        
        // رسم پس‌زمینه
        function drawBackground() {
            const gradient = ctx.createLinearGradient(0, 0, canvas.width, canvas.height);
            gradient.addColorStop(0, '#f0f8ff');
            gradient.addColorStop(1, '#e6f3ff');
            ctx.fillStyle = gradient;
            ctx.fillRect(0, 0, canvas.width, canvas.height);
            
            // خطوط شبکه
            ctx.strokeStyle = 'rgba(0,0,0,0.1)';
            ctx.lineWidth = 1;
            for (let x = 0; x < canvas.width; x += 50) {
                ctx.beginPath();
                ctx.moveTo(x, 0);
                ctx.lineTo(x, canvas.height);
                ctx.stroke();
            }
            for (let y = 0; y < canvas.height; y += 50) {
                ctx.beginPath();
                ctx.moveTo(0, y);
                ctx.lineTo(canvas.width, y);
                ctx.stroke();
            }
        }
        
        // رسم آیتم‌ها
        function drawItems() {
            items.forEach(item => {
                item.pulse += 0.1;
                const pulseSize = item.size + Math.sin(item.pulse) * 3;
                
                ctx.fillStyle = item.color;
                ctx.beginPath();
                ctx.arc(item.x + item.size/2, item.y + item.size/2, pulseSize/2, 0, 2 * Math.PI);
                ctx.fill();
                
                ctx.strokeStyle = '#333';
                ctx.lineWidth = 2;
                ctx.stroke();
                
                ctx.fillStyle = '#fff';
                ctx.font = '12px Arial';
                ctx.textAlign = 'center';
                ctx.fillText(item.symbol, item.x + item.size/2, item.y + item.size/2 + 4);
            });
        }
        
        // رسم دشمنان
        function drawEnemies() {
            enemies.forEach(enemy => {
                ctx.fillStyle = enemy.color;
                ctx.fillRect(enemy.x, enemy.y, enemy.size, enemy.size);
                
                ctx.fillStyle = '#fff';
                ctx.fillRect(enemy.x + 5, enemy.y + 5, 3, 3);
                ctx.fillRect(enemy.x + 17, enemy.y + 5, 3, 3);
                ctx.fillRect(enemy.x + 8, enemy.y + 15, 9, 2);
            });
        }
        
        // رسم بازیکن
        function drawPlayer() {
            const playerSize = 30;
            
            // سایه
            ctx.fillStyle = 'rgba(0,0,0,0.2)';
            ctx.fillRect(gameState.playerX + 3, gameState.playerY + 3, playerSize, playerSize);
            
            // بدنه بازیکن
            ctx.fillStyle = '#FF6B6B';
            ctx.fillRect(gameState.playerX, gameState.playerY, playerSize, playerSize);
            
            // حاشیه
            ctx.strokeStyle = '#333';
            ctx.lineWidth = 2;
            ctx.strokeRect(gameState.playerX, gameState.playerY, playerSize, playerSize);
            
            // چهره بازیکن
            ctx.fillStyle = '#333';
            ctx.fillRect(gameState.playerX + 8, gameState.playerY + 8, 4, 4);
            ctx.fillRect(gameState.playerX + 18, gameState.playerY + 8, 4, 4);
            ctx.fillRect(gameState.playerX + 10, gameState.playerY + 18, 10, 2);
            
            // محافظت
            if (gameState.powerUps.shield > 0) {
                ctx.strokeStyle = '#3498db';
                ctx.lineWidth = 3;
                ctx.beginPath();
                ctx.arc(gameState.playerX + playerSize/2, gameState.playerY + playerSize/2, playerSize/2 + 5, 0, 2 * Math.PI);
                ctx.stroke();
            }
        }
        
        // رسم ذرات
        function drawParticles() {
            particles = particles.filter(particle => {
                particle.x += particle.vx;
                particle.y += particle.vy;
                particle.life--;
                
                if (particle.life > 0) {
                    ctx.fillStyle = particle.color;
                    ctx.globalAlpha = particle.life / 30;
                    ctx.fillRect(particle.x, particle.y, 3, 3);
                    ctx.globalAlpha = 1;
                    return true;
                }
                return false;
            });
        }
        
        // بررسی برخوردها
        function checkCollisions() {
            // برخورد با آیتم‌ها
            items = items.filter(item => {
                const distance = Math.sqrt(
                    Math.pow(gameState.playerX + 15 - (item.x + item.size/2), 2) +
                    Math.pow(gameState.playerY + 15 - (item.y + item.size/2), 2)
                );
                
                if (distance < 25) {
                    gameState.score += item.value;
                    createParticles(item.x + item.size/2, item.y + item.size/2, item.color);
                    updateUI();
                    return false;
                }
                return true;
            });
            
            // برخورد با دشمنان
            enemies = enemies.filter(enemy => {
                const distance = Math.sqrt(
                    Math.pow(gameState.playerX + 15 - (enemy.x + enemy.size/2), 2) +
                    Math.pow(gameState.playerY + 15 - (enemy.y + enemy.size/2), 2)
                );
                
                if (distance < 25) {
                    if (gameState.powerUps.shield > 0) {
                        // با سپر: دشمن نابود می‌شود و امتیاز اضافه می‌شود
                        gameState.score += 30;
                        createParticles(enemy.x + enemy.size/2, enemy.y + enemy.size/2, '#00ff00');
                        updateUI();
                        return false;
                    } else {
                        // بدون سپر: امتیاز کم می‌شود
                        gameState.score = Math.max(0, gameState.score - 50);
                        createParticles(enemy.x + enemy.size/2, enemy.y + enemy.size/2, '#e74c3c');
                        updateUI();
                        return false;
                    }
                }
                return true;
            });
        }
        
        // حرکت بازیکن
        function movePlayer(direction) {
            if (!gameState.gameRunning) return;
            
            fetch('/api/move', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({direction: direction})
            })
            .then(response => response.json())
            .then(data => {
                updateGameState();
            });
        }
        
        // جمع‌آوری آیتم
        function collectItem() {
            if (gameState.gameRunning) {
                fetch('/api/score', {
                    method: 'POST'
                })
                .then(response => response.json())
                .then(data => {
                    gameState.score = data.score;
                    updateUI();
                });
            }
        }
        
        // استفاده از قدرت
        function usePowerUp() {
            if (gameState.gameRunning && gameState.score >= 50) {
                fetch('/api/shield', {
                    method: 'POST'
                })
                .then(response => response.json())
                .then(data => {
                    if (data.status === 'ok') {
                        gameState.powerUps.shield = data.shield;
                        gameState.score -= 50;
                        updateUI();
                    }
                });
            }
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
                    gameState.score = 0;
                    gameState.level = 1;
                    gameState.playerX = 250;
                    gameState.playerY = 250;
                    gameState.powerUps.shield = 0;
                    items = [];
                    enemies = [];
                    particles = [];
                    startGameLoop();
                } else {
                    stopGameLoop();
                }
            });
        }
        
        // شروع حلقه بازی
        function startGameLoop() {
            if (gameLoopId) clearInterval(gameLoopId);
            
            gameLoopId = setInterval(() => {
                if (gameState.gameRunning) {
                    // کاهش قدرت‌ها
                    if (gameState.powerUps.shield > 0) gameState.powerUps.shield--;
                    
                    // ایجاد آیتم‌ها
                    itemSpawnTimer++;
                    if (itemSpawnTimer >= 60) { // هر 1 ثانیه
                        createItem();
                        itemSpawnTimer = 0;
                    }
                    
                    // ایجاد دشمنان
                    if (gameState.level > 1) {
                        enemySpawnTimer++;
                        if (enemySpawnTimer >= 120) { // هر 2 ثانیه
                            createEnemy();
                            enemySpawnTimer = 0;
                        }
                    }
                    
                    // حرکت دشمنان
                    enemies.forEach(enemy => {
                        enemy.x += Math.cos(enemy.direction) * enemy.speed;
                        enemy.y += Math.sin(enemy.direction) * enemy.speed;
                        
                        if (enemy.x < 0 || enemy.x > canvas.width - enemy.size) {
                            enemy.direction = Math.PI - enemy.direction;
                        }
                        if (enemy.y < 0 || enemy.y > canvas.height - enemy.size) {
                            enemy.direction = -enemy.direction;
                        }
                    });
                    
                    // افزایش سطح
                    const newLevel = Math.floor(gameState.score / 100) + 1;
                    if (newLevel > gameState.level) {
                        gameState.level = newLevel;
                        updateUI();
                    }
                }
                
                drawGame();
            }, 1000/60);
        }
        
        // توقف حلقه بازی
        function stopGameLoop() {
            if (gameLoopId) {
                clearInterval(gameLoopId);
                gameLoopId = null;
            }
        }
        
        // به‌روزرسانی وضعیت بازی
        function updateGameState() {
            fetch('/api/game-state')
            .then(response => response.json())
            .then(data => {
                gameState = data;
                updateUI();
                updateStatus();
            });
        }
        
        // به‌روزرسانی رابط کاربری
        function updateUI() {
            document.getElementById('score').textContent = gameState.score;
            document.getElementById('level').textContent = gameState.level;
            updateShieldButton();
        }
        
        // به‌روزرسانی وضعیت
        function updateStatus() {
            const statusEl = document.getElementById('status');
            const toggleBtn = document.getElementById('gameToggle');
            
            if (gameState.gameRunning) {
                statusEl.textContent = `بازی در حال اجرا است - سطح ${gameState.level}`;
                statusEl.className = 'status running';
                toggleBtn.textContent = 'توقف بازی';
            } else {
                statusEl.textContent = 'بازی متوقف است';
                statusEl.className = 'status stopped';
                toggleBtn.textContent = 'شروع بازی';
            }
        }
        
        // توابع راهنما
        function showHelp() {
            document.getElementById('helpOverlay').style.display = 'flex';
        }
        
        function hideHelp() {
            document.getElementById('helpOverlay').style.display = 'none';
        }
        
        // بستن راهنما با کلیک روی پس‌زمینه
        document.getElementById('helpOverlay').addEventListener('click', function(e) {
            if (e.target === this) {
                hideHelp();
            }
        });
        
        // کنترل با کیبورد
        document.addEventListener('keydown', (e) => {
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
            
            switch(e.key) {
                case 'ArrowUp':
                case 'w':
                case 'W':
                    movePlayer('up');
                    break;
                case 'ArrowDown':
                case 's':
                case 'S':
                    movePlayer('down');
                    break;
                case 'ArrowLeft':
                case 'a':
                case 'A':
                    movePlayer('left');
                    break;
                case 'ArrowRight':
                case 'd':
                case 'D':
                    movePlayer('right');
                    break;
                case ' ':
                    collectItem();
                    break;
                case 'p':
                case 'P':
                    usePowerUp();
                    break;
            }
        });
        
        document.addEventListener('keyup', (e) => {
            keysPressed[e.key] = false;
        });
        
        // جلوگیری از اسکرول صفحه در موبایل
        document.addEventListener('touchmove', (e) => {
            if (e.target === canvas || e.target.closest('.joystick-container')) {
                e.preventDefault();
            }
        }, { passive: false });
        
        // توابع جوی استیک
        function initJoystick() {
            const joystickKnob = document.getElementById('joystickKnob');
            const joystickBase = document.querySelector('.joystick-base');
            
            if (!joystickKnob || !joystickBase) return;
            
            const rect = joystickBase.getBoundingClientRect();
            joystickCenterX = rect.left + rect.width / 2;
            joystickCenterY = rect.top + rect.height / 2;
            joystickRadius = rect.width / 2 - 25; // 25px برای knob
            
            // رویدادهای لمسی
            joystickKnob.addEventListener('touchstart', handleJoystickStart);
            joystickKnob.addEventListener('touchmove', handleJoystickMove);
            joystickKnob.addEventListener('touchend', handleJoystickEnd);
            
            // رویدادهای ماوس (برای تست)
            joystickKnob.addEventListener('mousedown', handleJoystickStart);
            joystickKnob.addEventListener('mousemove', handleJoystickMove);
            joystickKnob.addEventListener('mouseup', handleJoystickEnd);
        }
        
        function handleJoystickStart(e) {
            e.preventDefault();
            joystickActive = true;
            document.body.style.userSelect = 'none';
        }
        
        function handleJoystickMove(e) {
            if (!joystickActive) return;
            e.preventDefault();
            
            const clientX = e.touches ? e.touches[0].clientX : e.clientX;
            const clientY = e.touches ? e.touches[0].clientY : e.clientY;
            
            const deltaX = clientX - joystickCenterX;
            const deltaY = clientY - joystickCenterY;
            const distance = Math.sqrt(deltaX * deltaX + deltaY * deltaY);
            
            let angle = Math.atan2(deltaY, deltaX);
            let moveX = 0;
            let moveY = 0;
            
            if (distance > 10) { // حداقل فاصله برای حرکت
                if (distance > joystickRadius) {
                    // محدود کردن به دایره
                    const limitedX = Math.cos(angle) * joystickRadius;
                    const limitedY = Math.sin(angle) * joystickRadius;
                    moveX = limitedX;
                    moveY = limitedY;
                } else {
                    moveX = deltaX;
                    moveY = deltaY;
                }
                
                // تشخیص جهت حرکت
                const threshold = 20;
                if (Math.abs(moveX) > threshold || Math.abs(moveY) > threshold) {
                    if (Math.abs(moveX) > Math.abs(moveY)) {
                        if (moveX > 0) {
                            movePlayer('right');
                        } else {
                            movePlayer('left');
                        }
                    } else {
                        if (moveY > 0) {
                            movePlayer('down');
                        } else {
                            movePlayer('up');
                        }
                    }
                }
            }
            
            // حرکت knob
            const knob = document.getElementById('joystickKnob');
            if (knob) {
                knob.style.transform = `translate(${moveX - 25}px, ${moveY - 25}px)`;
            }
        }
        
        function handleJoystickEnd(e) {
            e.preventDefault();
            joystickActive = false;
            document.body.style.userSelect = '';
            
            // بازگشت knob به مرکز
            const knob = document.getElementById('joystickKnob');
            if (knob) {
                knob.style.transform = 'translate(-50%, -50%)';
            }
        }
        
        // به‌روزرسانی وضعیت دکمه سپر
        function updateShieldButton() {
            const shieldBtn = document.getElementById('mobileShieldBtn');
            if (shieldBtn) {
                if (gameState.gameRunning && gameState.score >= 50) {
                    shieldBtn.disabled = false;
                    shieldBtn.textContent = '🛡️ سپر (50)';
                } else {
                    shieldBtn.disabled = true;
                    shieldBtn.textContent = '🛡️ سپر (کم)';
                }
            }
        }
        
        // شروع بازی
        updateGameState();
        drawGame();
        initJoystick();
    </script>
</body>
</html>
)rawliteral";
  return html;
}

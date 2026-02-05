/*****************************************************************************************
 * WHACK-A-MOLE (DISPLAYLESS VERSION)
 * - NO display (LCD / 7-seg removed)
 * - EDGE-detected hits only
 * - ACTIVE-LOW relay module
 * - added 5 second count down prep before gameplay and random light up before initial button push (05/02/2026)
 *****************************************************************************************/
// ================= PIN DEFINITIONS =================
// Buttons (INPUT_PULLUP)
 #define BTN_RED       7
 #define BTN_GREEN     8
 #define BTN_BLUE      9
 #define BTN_YELLOW    10
 #define BTN_WHITE     11
 // ===================================================
 
 const int relayPins[5]  = { RELAY_RED, RELAY_GREEN, RELAY_BLUE, RELAY_YELLOW, RELAY_WHITE };
 const int buttonPins[5] = { BTN_RED, BTN_GREEN, BTN_BLUE, BTN_YELLOW, BTN_WHITE };
 const char* colors[5]  = { "RED", "GREEN", "BLUE", "YELLOW", "WHITE" };
 
 // ================= GAME CONSTANTS =================
 const int startLitTime = 3000;
 
 const int Level2000ms = 50;
 const int Level1000ms = 200;
 const int Level500ms  = 400;
 
 const int MoleLevel2  = 100;
 const int MoleLevel3  = 300;
 
 const int GAME_TIME_SEC = 30;
+const int PREP_COUNTDOWN_SEC = 5;
+const int BONUS_TIME_SEC = 15;
 const unsigned long DEBOUNCE_MS = 400;
 // ===================================================
 
 // ================= GAME STATE =================
 bool moleActive[5];
 unsigned long moleEnd[5];
 unsigned long lastPressed[5];
 
 int score = 0;
 bool bonusUsed = false;
 bool gameRunning = false;
 int molesLit = 0;
+int bonusTimeSec = 0;
 
 unsigned long gameStartMillis;
 
 int lastButtonState[5];
+unsigned long lastIdlePatternMillis = 0;
 // ===============================================
 
 void setup() {
   Serial.begin(9600);
   randomSeed(analogRead(0));
 
   // ===== FORCE RELAYS OFF FIRST =====
   for (int i = 0; i < 5; i++) {
     pinMode(relayPins[i], OUTPUT);
     digitalWrite(relayPins[i], HIGH);   // ACTIVE-LOW → OFF
   }
 
   delay(100);
 
   // ===== BUTTONS =====
   for (int i = 0; i < 5; i++) {
     pinMode(buttonPins[i], INPUT_PULLUP);
     lastButtonState[i] = digitalRead(buttonPins[i]);
     lastPressed[i] = 0;
     moleActive[i] = false;
   }
 
   Serial.println("Whack-a-Mole Ready");
   Serial.println("Press ANY button to start");
 }
 
 void loop() {
   if (!gameRunning) {
+    // Idle glow pattern (random relays) until game starts
+    if (millis() - lastIdlePatternMillis >= 250) {
+      int choice = random(6); // 0-4 single relay, 5 = all off
+      for (int i = 0; i < 5; i++) {
+        bool on = (choice < 5) ? (i == choice) : false;
+        digitalWrite(relayPins[i], on ? LOW : HIGH);
+      }
+      lastIdlePatternMillis = millis();
+    }
+
     // Start game on ANY button HIT
     for (int i = 0; i < 5; i++) {
       int state = digitalRead(buttonPins[i]);
       if (lastButtonState[i] == HIGH && state == LOW) {
         startGame();
         break;
       }
       lastButtonState[i] = state;
     }
     return;
   }
 
   // ===== GAME TIMER =====
-  int timeLeft = GAME_TIME_SEC - int((millis() - gameStartMillis) / 1000);
+  int timeLeft = (GAME_TIME_SEC + bonusTimeSec) - int((millis() - gameStartMillis) / 1000);
   if (timeLeft <= 0) {
     endGame();
     return;
   }
 
   // ===== BUTTON CHECK =====
   for (int i = 0; i < 5; i++) {
     int state = digitalRead(buttonPins[i]);
 
     if (lastButtonState[i] == HIGH && state == LOW) {
       unsigned long delta = millis() - lastPressed[i];
 
       if (delta > DEBOUNCE_MS) {
         lastPressed[i] = millis();
 
         if (moleActive[i]) {
           // HIT
           score += 10;
           moleActive[i] = false;
           digitalWrite(relayPins[i], HIGH); // OFF
           molesLit--;
 
           Serial.print("HIT → ");
           Serial.println(colors[i]);
 
           addMole();
 
           if (score >= MoleLevel2 && molesLit < 2) addMole();
           if (score >= MoleLevel3 && molesLit < 3) addMole();
 
           if (!bonusUsed && score >= 500) {
             bonusUsed = true;
+            bonusTimeSec += BONUS_TIME_SEC;
             Serial.println("BONUS +15 sec");
           }
         } else {
           // MISS
           score -= 5;
           if (score < 0) score = 0;
 
           Serial.print("MISS → ");
           Serial.println(colors[i]);
         }
       }
     }
 
     lastButtonState[i] = state;
   }
 
   // ===== MOLE TIMEOUT =====
   for (int i = 0; i < 5; i++) {
     if (moleActive[i] && millis() > moleEnd[i]) {
       moleActive[i] = false;
       digitalWrite(relayPins[i], HIGH);
       molesLit--;
       addMole();
     }
   }
 }
 
 // ================= START GAME =================
 void startGame() {
   Serial.println("GAME STARTED");
 
   score = 0;
   bonusUsed = false;
+  bonusTimeSec = 0;
   molesLit = 0;
   gameRunning = true;
 
   for (int i = 0; i < 5; i++) {
     moleActive[i] = false;
     digitalWrite(relayPins[i], HIGH);
   }
 
+  // Prep countdown before gameplay
+  Serial.print("GAME STARTING IN ");
+  Serial.print(PREP_COUNTDOWN_SEC);
+  Serial.println("...");
+  for (int i = PREP_COUNTDOWN_SEC; i > 0; i--) {
+    Serial.print(i);
+    Serial.println("...");
+    delay(1000);
+  }
+
   gameStartMillis = millis();
   addMole();
 }
 
 // ================= END GAME =================
 void endGame() {
   Serial.print("GAME OVER | SCORE = ");
   Serial.println(score);
 
   gameRunning = false;
 
   for (int i = 0; i < 5; i++) {
     moleActive[i] = false;
     digitalWrite(relayPins[i], HIGH);
   }
 
   Serial.println("Press any button to restart");
 }
 
 // ================= ADD MOLE =================
 void addMole() {
   int ttl = startLitTime;
 
   if (score >= Level2000ms) ttl = 2000;
   if (score >= Level1000ms) ttl = 1000;


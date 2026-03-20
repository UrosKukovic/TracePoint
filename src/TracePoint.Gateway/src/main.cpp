#include <Arduino.h>

// put function declarations here:
int myFunction(int, int);

void setup() {
  Serial.begin(115200);
  delay(2000); 
  
  Serial.println("======================================");
  Serial.println("   TRACEPOINT GATEWAY - STARTING      ");
  Serial.println("======================================");
  
  // Preverimo, če ESP32 vidi svoj PSRAM (tistih 8MB)
  if(psramInit()){
    Serial.printf("PSRAM inicializiran. Velikost: %d bytes\n", ESP.getPsramSize());
  } else {
    Serial.println("PSRAM ni bil najden!");
  }
}

void loop() {
  Serial.printf("[%lu] Gateway teče... Čakam na CAN ukaze.\n", millis());
  delay(2000);
}
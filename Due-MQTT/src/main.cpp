#include "main.h"
static void vBlink(void *parameters)
{
  pinMode(LED_BUILTIN, OUTPUT);
  for (;;)
  {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    vTaskDelay(1000);
  }
}
static void vController(void *parameters)
{
  unsigned long last;
  pinMode(ESP_IO, INPUT);
  Serial2.begin(115200);
  Serial.println("Ready");
  DynamicJsonDocument doc(1024);
  for (;;)
  {
    if(millis() - last > 500){
      direction = NULL;
    }
    if (Serial2.available() > 0)
    {
      String json = Serial2.readStringUntil('!');
      DeserializationError error = deserializeJson(doc, json);
      if (!error)
      {
        direction = doc["dir"];
        camera = doc["cam"];
        // Serial.println(direction);
        last = millis();
      }
    }
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

static void vMovement(void *parameters)
{
  for(;;){
    servo.write(camera);
    if(direction == 1) Serial.println("Maju");
    if(direction == 2) Serial.println("Kanan");
    if(direction == 3) Serial.println("Mundur");
    if(direction == 4) Serial.println("Kiri");
    vTaskDelay(1/portTICK_PERIOD_MS);
  }
}
void EN1()
{
  m1.isrHandler();
}

void EN2()
{
  m2.isrHandler();
}
#if defined(RAMAV1)
void EN3()
{
  m3.isrHandler();
}
void EN4()
{
  m4.isrHandler();
}
#endif

void timerIsr()
{
  m1.calculateRpm(SAMPLING_TIME);
  m2.calculateRpm(SAMPLING_TIME);
#if defined(RAMAV1)
  m3.calculateRpm(SAMPLING_TIME);
  m4.calculateRpm(SAMPLING_TIME);
#endif
}

void setup()
{
  Serial.begin(115200);
  servo.attach(srvPin);
  pinMode(ESP_IO, INPUT_PULLUP);
  // pinMode(10, INPUT);
  // m1.pid(5, 0.1, 0.4, 2000);
  // m2.pid(5, 0.1, 0.4, 2000);
  attachInterrupt(digitalPinToInterrupt(m1.en_a), EN1, FALLING);
  attachInterrupt(digitalPinToInterrupt(m2.en_a), EN2, FALLING);

  xTaskCreate(vBlink,
              "Blink",
              configMINIMAL_STACK_SIZE,
              NULL,
              tskIDLE_PRIORITY + 1,
              &t_blink);
  xTaskCreate(vController,
              "Controller",
              configMINIMAL_STACK_SIZE + 300,
              NULL,
              tskIDLE_PRIORITY + 3,
              &t_controller);
  xTaskCreate(vMovement,
              "Movement",
              configMINIMAL_STACK_SIZE + 300,
              NULL,
              tskIDLE_PRIORITY + 2,
              &t_movement);
  vTaskStartScheduler();
}

void loop()
{
  // int y = ex7.y();
  // int rotate = ex7.rotate();
  // int cameraDeg = ex7.cameraDegree();
  // servo.write(cameraDeg);
  // if (ex7.connected && (abs(y)> 50 || abs(rotate)> 50))
  // {
  //   if (abs(rotate) < 50)
  //   {
  //     Serial.println("Maju mundur");
  //     y > 0 ? base.forward(y) : base.reverse(y);
  //   }
  //   else
  //   {
  //     Serial.println("muter");
  //     rotate > 0 ? base.rotateRight(rotate) : base.rotateLeft(rotate);
  //   }
  // }
  // else
  // {
  //   Serial.println("Mandek");
  //   base.brake();
  // }
}
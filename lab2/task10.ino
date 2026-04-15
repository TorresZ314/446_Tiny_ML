#include <PDM.h>
#include <Arduino_APDS9960.h>
#include <Arduino_BMI270_BMM150.h>

short sampleBuffer[256];
volatile int samplesRead = 0;

void onPDMdata() {
  int bytesAvailable = PDM.available();
  PDM.read(sampleBuffer, bytesAvailable);
  samplesRead = bytesAvailable / 2;
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  PDM.onReceive(onPDMdata);

  if (!PDM.begin(1, 16000)) {
    Serial.println("Failed to initialize microphone.");
    while (1);
  }

  if (!APDS.begin()) {
    Serial.println("Failed to initialize APDS9960 sensor.");
    while (1);
  }

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU.");
    while (1);
  }

  Serial.println("Workspace classifier started");
}

void loop() {
  static int mic = 0;
  static int clear = 0;
  static float motion = 0.0;
  static int prox = 0;

  if (samplesRead) {
    long sum = 0;
    for (int i = 0; i < samplesRead; i++) {
      sum += abs(sampleBuffer[i]);
    }
    mic = sum / samplesRead;
    samplesRead = 0;
  }

  if (APDS.colorAvailable()) {
    int r, g, b, c;
    APDS.readColor(r, g, b, c);
    clear = c;
  }

  if (APDS.proximityAvailable()) {
    prox = APDS.readProximity();
  }

  if (IMU.accelerationAvailable()) {
    float x, y, z;
    IMU.readAcceleration(x, y, z);
    float magnitude = sqrt((x * x) + (y * y) + (z * z));
    motion = abs(magnitude - 1.0); 
  }

  int sound_flag = (mic > 50) ? 1 : 0;
  int dark_flag = (clear < 15) ? 1 : 0;
  int moving_flag = (motion > 0.15) ? 1 : 0;
  int near_flag = (prox > 100) ? 1 : 0;

  String state = "UNKNOWN";

  if (sound_flag == 0 && dark_flag == 0 && moving_flag == 0 && near_flag == 0) {
    state = "QUIET_BRIGHT_STEADY_FAR";
  } 
  else if (sound_flag == 1 && dark_flag == 0 && moving_flag == 0 && near_flag == 0) {
    state = "NOISY_BRIGHT_STEADY_FAR";
  } 
  else if (sound_flag == 0 && dark_flag == 1 && moving_flag == 0 && near_flag == 1) {
    state = "QUIET_DARK_STEADY_NEAR";
  } 
  else if (sound_flag == 1 && dark_flag == 0 && moving_flag == 1 && near_flag == 1) {
    state = "NOISY_BRIGHT_MOVING_NEAR";
  }

  Serial.print("raw,mic=");
  Serial.print(mic);
  Serial.print(",clear=");
  Serial.print(clear);
  Serial.print(",motion=");
  Serial.print(motion, 3);
  Serial.print(",prox=");
  Serial.println(prox);

  Serial.print("flags,sound=");
  Serial.print(sound_flag);
  Serial.print(",dark=");
  Serial.print(dark_flag);
  Serial.print(",moving=");
  Serial.print(moving_flag);
  Serial.print(",near=");
  Serial.println(near_flag);

  Serial.print("state,");
  Serial.println(state);

  delay(500);
}

#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <DHT.h>

#define WIFI_SSID "Minh's Galaxy S21 5G"  // Tên mạng Wi-Fi của bạn
#define WIFI_PASSWORD "nguyetminh212"  // Mật khẩu Wi-Fi của bạn
#define FIREBASE_HOST "https://smartwatering-4fe6a-default-rtdb.firebaseio.com/"  // Firebase Hostname
#define FIREBASE_AUTH "Zn7B2TE091ApU2XRbyzu0L0wjJn3NxfcIEMUuHpw"  // Firebase Database Secret

// Định nghĩa loại cảm biến DHT và chân kết nối
#define DHTPIN D1        // Chân GPIO5 (D1 trên NodeMCU)
#define soilPin A0       
#define DHTTYPE DHT11    // Chọn DHT11 (nếu bạn dùng DHT22 thì thay bằng DHT22)

// Định nghĩa chân điều khiển relay
#define RELAY_PIN D0     

DHT dht(DHTPIN, DHTTYPE);
//Declares Variables
FirebaseData firebasedata;
FirebaseConfig config;
FirebaseAuth auth;

void setup() {
  Serial.begin(115200);

 // Kết nối Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Đang kết nối WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Đã kết nối WiFi.");

  // Cấu hình Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  // Khởi động Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  dht.begin();  // Khởi động cảm biến DHT11
  pinMode(RELAY_PIN, OUTPUT); // Thiết lập chân relay là OUTPUT
  digitalWrite(RELAY_PIN, LOW); // Tắt máy bơm ban đầu
}

void loop() {
  // Đọc giá trị nhiệt độ và độ ẩm
  float h = dht.readHumidity();
  float t = dht.readTemperature();  // Đọc nhiệt độ bằng độ C
  int soilMoistureValue = analogRead(soilPin);
  int soilMoisturePercent = ((1024 - soilMoistureValue) * 100) / 1024;

  // Gửi dữ liệu lên Firebase
  Firebase.setFloat(firebasedata, "/sensor_data/temperature", t);
  Firebase.setFloat(firebasedata, "/sensor_data/humidity", h);
  Firebase.setFloat(firebasedata, "/sensor_data/soil_moisture", soilMoisturePercent);
  
  //Firebase.setBool(firebasedata, "/is_auto", true);
  //Firebase.setInt(firebasedata,"/soil_moisture_lower_threshold", 20);
  //Firebase.setInt(firebasedata,"/soil_moisture_upper_threshold", 25);

  // Nhận giá trị từ Firebase (is_auto, is_pump_on)
  bool isAuto = false;
  if (Firebase.getBool(firebasedata, "/is_auto")) {
    isAuto = firebasedata.boolData();
  }
  Serial.println(isAuto);
  
  bool isPumpOn = false;
  if (Firebase.getBool(firebasedata, "/is_pump_on")) {
    isPumpOn = firebasedata.boolData();
  }
  Serial.println(isPumpOn);

  int soilMoistureLowerThreshold = 0;
  if (Firebase.getInt(firebasedata, "/soil_moisture_lower_threshold")) {
    soilMoistureLowerThreshold = firebasedata.intData();
  } else {
    Serial.println("Không thể lấy giá trị soil_moisture_lower_threshold, dùng giá trị mặc định.");
  }

  int soilMoistureUpperThreshold = 0;
  if (Firebase.getInt(firebasedata, "/soil_moisture_upper_threshold")) {
    soilMoistureUpperThreshold = firebasedata.intData();
  } else {
    Serial.println("Không thể lấy giá trị soil_moisture_upper_threshold, dùng giá trị mặc định.");
  }

  int temperatureLowerThreshold = 0;
  if (Firebase.getInt(firebasedata, "/temperature_lower_threshold")) {
    temperatureLowerThreshold = firebasedata.intData();
  } else {
    Serial.println("Không thể lấy giá trị temperature_lower_threshold, dùng giá trị mặc định.");
  }

  int temperatureUpperThreshold = 0;
  if (Firebase.getInt(firebasedata, "/temperature_upper_threshold")) {
    temperatureUpperThreshold = firebasedata.intData();
  } else {
    Serial.println("Không thể lấy giá trị temperature_upper_threshold, dùng giá trị mặc định.");
  }

  // Kiểm tra chế độ tự động
  if (isAuto) {
    // // Điều kiện để bật máy bơm (tự động)
    // Serial.print("soil Moisture: ");
    // Serial.print(soilMoisturePercent);
    // Serial.println();
    // Serial.print("Temperature: ");
    // Serial.print(t);
    // Serial.print("moisturelower: ");
    // Serial.print(soilMoistureLowerThreshold);
    // Serial.print("moistureUpper: ");
    // Serial.print(soilMoistureUpperThreshold);
    // Serial.print("temperaturelower: ");
    // Serial.print(temperatureLowerThreshold);
    //  Serial.print("temperatureUpper: ");
    // Serial.print(temperatureUpperThreshold);
    
    // if (soilMoisturePercent < soilMoistureLowerThreshold && t >= temperatureLowerThreshold && t <= temperatureUpperThreshold) {
    //   digitalWrite(RELAY_PIN, LOW);  // Bật máy bơm
    //   Firebase.setBool(firebasedata, "/is_pump_on", true);
    //   Serial.println("Máy bơm tự động bật do độ ẩm thấp và nhiệt độ phù hợp.");
    // } else if (soilMoisturePercent >= soilMoistureUpperThreshold || t < temperatureLowerThreshold || t > temperatureUpperThreshold) {
    //   digitalWrite(RELAY_PIN, HIGH);  // Tắt máy bơm
    //   Firebase.setBool(firebasedata, "/is_pump_on", false);
    //   Serial.println("Máy bơm tự động tắt do điều kiện không phù hợp.");
    // }
    registerAuto();
    registerRelay();
  } else {
    // Chế độ thủ công: bật/tắt bơm dựa vào "is_pump_on"
    // if (isPumpOn) {
    //   digitalWrite(RELAY_PIN, LOW);  // Bật máy bơm
    //   Serial.println("Máy bơm được bật thủ công.");
    // } else {
    //   digitalWrite(RELAY_PIN, HIGH);  // Tắt máy bơm
    //   Serial.println("Máy bơm được tắt thủ công.");
    // }
    registerRelay();
  }

  delay(2000);  // Chờ 2 giây trước khi lặp lại
}

void registerAuto() {
  // Điều kiện để bật máy bơm (tự động)
  float h = dht.readHumidity();
  float t = dht.readTemperature();  // Đọc nhiệt độ bằng độ C
  int soilMoistureValue = analogRead(soilPin);
  int soilMoisturePercent = ((1024 - soilMoistureValue) * 100) / 1024;
  int soilMoistureLowerThreshold = 0;
  if (Firebase.getInt(firebasedata, "/soil_moisture_lower_threshold")) {
    soilMoistureLowerThreshold = firebasedata.intData();
  } else {
    Serial.println("Không thể lấy giá trị soil_moisture_lower_threshold, dùng giá trị mặc định.");
  }

  int soilMoistureUpperThreshold = 0;
  if (Firebase.getInt(firebasedata, "/soil_moisture_upper_threshold")) {
    soilMoistureUpperThreshold = firebasedata.intData();
  } else {
    Serial.println("Không thể lấy giá trị soil_moisture_upper_threshold, dùng giá trị mặc định.");
  }

  int temperatureLowerThreshold = 0;
  if (Firebase.getInt(firebasedata, "/temperature_lower_threshold")) {
    temperatureLowerThreshold = firebasedata.intData();
  } else {
    Serial.println("Không thể lấy giá trị temperature_lower_threshold, dùng giá trị mặc định.");
  }

  int temperatureUpperThreshold = 0;
  if (Firebase.getInt(firebasedata, "/temperature_upper_threshold")) {
    temperatureUpperThreshold = firebasedata.intData();
  } else {
    Serial.println("Không thể lấy giá trị temperature_upper_threshold, dùng giá trị mặc định.");
  }

    Serial.print("soil Moisture: ");
    Serial.print(soilMoisturePercent);
    Serial.println();
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print("moisturelower: ");
    Serial.print(soilMoistureLowerThreshold);
    Serial.print("moistureUpper: ");
    Serial.print(soilMoistureUpperThreshold);
    Serial.print("temperaturelower: ");
    Serial.print(temperatureLowerThreshold);
     Serial.print("temperatureUpper: ");
    Serial.print(temperatureUpperThreshold);
    
    if (soilMoisturePercent < soilMoistureLowerThreshold && t >= temperatureLowerThreshold && t <= temperatureUpperThreshold) {
      // digitalWrite(RELAY_PIN, LOW);  // Bật máy bơm
      Firebase.setBool(firebasedata, "/is_pump_on", true);
      Serial.println("Máy bơm tự động bật do độ ẩm thấp và nhiệt độ phù hợp.");
    } else if (soilMoisturePercent >= soilMoistureUpperThreshold || t < temperatureLowerThreshold || t > temperatureUpperThreshold) {
      // digitalWrite(RELAY_PIN, HIGH);  // Tắt máy bơm
      Firebase.setBool(firebasedata, "/is_pump_on", false);
      Serial.println("Máy bơm tự động tắt do điều kiện không phù hợp.");
    }
}

void registerRelay() {
  bool isPumpOn = true;
  if (Firebase.getBool(firebasedata, "/is_pump_on")) {
    isPumpOn = firebasedata.boolData();
  }
  Serial.println(isPumpOn);
  if (isPumpOn) {
      digitalWrite(RELAY_PIN, LOW);  // Bật máy bơm
      Serial.println("Máy bơm được bật thủ công.");
    } else {
      digitalWrite(RELAY_PIN, HIGH);  // Tắt máy bơm
      Serial.println("Máy bơm được tắt thủ công.");
    }
}

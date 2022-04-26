#include <WiFi.h>
#include <FirebaseESP32.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_http_client.h>

#define FIREBASE_HOST "eclh20130.firebaseio.com"
#define FIREBASE_AUTH "5dJnYD7Gx1QceE67FmbvHw3scUYe5ABPFerACoyF"
#define WIFI_SSID "USER 的 iPhone"
#define WIFI_PASSWORD "12345678"

const char *HOST = "fcm.googleapis.com";

const char *host = "maker.ifttt.com";
const char *privateKey = "PemHEfclm-e5InkJR3z9f";

String url = "http://api.thingspeak.com/update?api_key=3BRTWLZU6X1DIDR1";
 
//Define FirebaseESP32 data object
FirebaseData firebaseData;
FirebaseJson json;

#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 26

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int fsr_alert = 700;
int tmp_alert = 27;
int air_alert = 400;
int co_alert = 1000;

#define Fsr 39
int Air = 36; 
int CO = 34;
int LED = 25;
int buzzer = 17;
int Airdata, COdata, Fsrdata = 0; 
String yes = "是";
String no = "否";

unsigned long start_time;
unsigned long duration;

WiFiClient client;

bool Post = false;

esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
  if(evt->event_id == HTTP_EVENT_ON_DATA) {
      Serial.printf("%.*s", evt->data_len, (char*)evt->data);
  }
}

void setup()
{

  Serial.begin(115200);

  Post = false;
    
  pinMode(Air, INPUT);
  pinMode(CO, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(buzzer, OUTPUT);
 
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
 
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
 
  //Set database read timeout to 1 minute (max 15 minutes)
  Firebase.setReadTimeout(firebaseData, 500 * 1);
  //tiny, small, medium, large and unlimited.
  //Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).
  Firebase.setwriteSizeLimit(firebaseData, "tiny");
 
  /*
  This option allows get and delete functions (PUT and DELETE HTTP requests) works for device connected behind the
  Firewall that allows only GET and POST requests.
  
  Firebase.enableClassicRequest(firebaseData, true);
  */
 
  //String path = "/data";
  
 
  Serial.println("------------------------------------");
  Serial.println("Connected...");
}

void loop()
{
   
   sensors.requestTemperatures();
   float temp = sensors.getTempCByIndex(0);
   Airdata = analogRead(Air);
   COdata = analogRead(CO);
   Fsrdata = analogRead(Fsr);
   int Adata = map(Airdata,0,4095,0,1000);
   int Cdata = map(COdata,0,4095,0,1000);
   Serial.println(Adata, DEC);
   Serial.println(Cdata, DEC);
   Serial.println(temp); 
   Serial.println(Fsrdata);
   delay(500);

   //離床照明
   if(Fsrdata > fsr_alert)
   {
     digitalWrite(LED, LOW);
     json.set("/Fsr", yes);
     Serial.println(yes);
     
   }
   else if(Fsrdata <= fsr_alert)
   {
     digitalWrite(LED, HIGH);
     json.set("/Fsr", no);
     Serial.println(no);
   }

  //離床過久警示
   if(Fsrdata <= fsr_alert) //沒有感測到壓力開始計時
   {   
       start_time = millis(); //將目前時間設定為起始時間
       while(Fsrdata <= fsr_alert)
       {
         duration = millis() - start_time;
                 
         if(duration >= 3000)
         {
          Serial.println("回來");          
          //send_event("back");

  esp_http_client_config_t config = {}; // important to initialize with "{}" when using C++ on ESP-IDF http client or it will crash easily
    config.url = "https://fcm.googleapis.com/fcm/send";
    config.event_handler = _http_event_handler;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    const char *post_data = "{\"to\": \"eoNe5-i1HXg:APA91bHnSjQepgJQYX8AZ2Bk8zqkGU-D27tKi6pr_jg7njmS2jCNucJ38fIZlthrGGk8_TdGKLCvKm2mmvy9QiExFVqWkFlAbFCIH56ewqNutsXMEuCkMIUyDm3NW6O3EN80fOhrITVP\",\"notification\": {\"body\": \"離床超過預設時間\",\"title\" : \"離床過久警示\"} }";
    esp_http_client_set_header(client, "Authorization", "key=AAAA3v9DRkQ:APA91bFmI68OtGACSeQWg6AVXnoa3iuI59zg0CocOhKD8lk4Y0EvgPhX2DZRprCI6M6ZKhqQzm70ztB3xdH5EpDb5EnfSuzNWhJwLYDY1ZD-8GzokovbPWNJ5Q2aqGQnhVCpFG7CFV8w");
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);  

          Fsrdata = 701;
         }
       }
   }

   //防火警報
   if(Adata > air_alert || Cdata > co_alert)
   {
       
     if(Post == false){
       //send_event("fire");

       esp_http_client_config_t config = {}; // important to initialize with "{}" when using C++ on ESP-IDF http client or it will crash easily
    config.url = "https://fcm.googleapis.com/fcm/send";
    config.event_handler = _http_event_handler;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    const char *post_data = "{\"to\": \"eoNe5-i1HXg:APA91bHnSjQepgJQYX8AZ2Bk8zqkGU-D27tKi6pr_jg7njmS2jCNucJ38fIZlthrGGk8_TdGKLCvKm2mmvy9QiExFVqWkFlAbFCIH56ewqNutsXMEuCkMIUyDm3NW6O3EN80fOhrITVP\",\"notification\": {\"body\": \"氣體濃度過高\",\"title\" : \"防火警報\"} }";
    esp_http_client_set_header(client, "Authorization", "key=AAAA3v9DRkQ:APA91bFmI68OtGACSeQWg6AVXnoa3iuI59zg0CocOhKD8lk4Y0EvgPhX2DZRprCI6M6ZKhqQzm70ztB3xdH5EpDb5EnfSuzNWhJwLYDY1ZD-8GzokovbPWNJ5Q2aqGQnhVCpFG7CFV8w");
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);

     for(int i = 250; i > 0 ; --i)
          {
            digitalWrite(buzzer, HIGH);           
            delay(1);
            digitalWrite(buzzer, LOW);
            delay(1);
            digitalWrite(buzzer, HIGH);           
            delay(1);
            digitalWrite(buzzer, LOW);
            delay(1);
            digitalWrite(buzzer, HIGH);           
            delay(1);
            digitalWrite(buzzer, LOW);
            delay(1);
          }

          delay(1000);
          Post = true;    
    }
   }

   //防火數值重製
   if(Adata < air_alert || Cdata < co_alert)
   {
     Post = false;
   }
   

   if(temp < tmp_alert)
   {
       //send_event("cold");
       
       esp_http_client_config_t config = {}; // important to initialize with "{}" when using C++ on ESP-IDF http client or it will crash easily
    config.url = "https://fcm.googleapis.com/fcm/send";
    config.event_handler = _http_event_handler;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    const char *post_data = "{\"to\": \"eoNe5-i1HXg:APA91bHnSjQepgJQYX8AZ2Bk8zqkGU-D27tKi6pr_jg7njmS2jCNucJ38fIZlthrGGk8_TdGKLCvKm2mmvy9QiExFVqWkFlAbFCIH56ewqNutsXMEuCkMIUyDm3NW6O3EN80fOhrITVP\",\"notification\": {\"body\": \"溫度過低\",\"title\" : \"室溫監控\"} }";
    esp_http_client_set_header(client, "Authorization", "key=AAAA3v9DRkQ:APA91bFmI68OtGACSeQWg6AVXnoa3iuI59zg0CocOhKD8lk4Y0EvgPhX2DZRprCI6M6ZKhqQzm70ztB3xdH5EpDb5EnfSuzNWhJwLYDY1ZD-8GzokovbPWNJ5Q2aqGQnhVCpFG7CFV8w");
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);  
   }
   
   
   //上傳firebase
   json.set("/Air", Adata);
   json.set("/CO", Cdata);
   json.set("/Tmp", temp);
   //json.set("/Fsr", Fsrdata);
 
   Firebase.updateNode(firebaseData,"/Sensor",json);

   //開始傳送到thingspeak
  Serial.println("啟動網頁連線");
  HTTPClient http;
  //將溫度及濕度以http get參數方式補入網址後方
  String url1 = url + "&field1=" + Adata + "&field2=" + Cdata + 
  "&field3=" + temp + "&field4=" + Fsrdata;
  //http client取得網頁內容
  http.begin(url1);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK)      {
    //讀取網頁內容到payload
    String payload = http.getString();
    //將內容顯示出來
    Serial.print("網頁內容=");
    Serial.println(payload);
  } else {
    //讀取失敗
    Serial.println("網路傳送失敗");
  }
  http.end();
  delay(1000);//休息20秒
 
}


//傳送Line訊息
/*void send_event(const char *event)
{
  Serial.println(host);
  WiFiClient client;
  const int httpPort = 80;

  if (!client.connect(host, httpPort)) {
    return;
  }

  String url = "/trigger/";
  url += event;
  url += "/with/key/";
  url += privateKey;

  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  while (client.connected())
  {
    if (client.available())
    {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    } else {
      delay(50);
    };
  }

  Serial.println();
  client.stop();
}*/

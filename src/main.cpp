#include <Arduino.h>
#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

const char* ssid = "ATPlus-DaDi"; 
const char* password = "papito29";
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

const int oneWireBus = 4; 
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
unsigned long lastReconnectAttempt = 0;
bool redeEstavaFora = false; // Flag para quando a rede voltar

void setup_wifi() {
  Serial.print("\nConectando ao Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
}

void reconnect() {
  Serial.print("Tentando conexao MQTT... ");
  String clientId = "ESP32-TCC-StoreForward-";
  clientId += String(random(0xffff), HEX);
  
  if (client.connect(clientId.c_str())) {
    Serial.println("[Conectado ao Broker!]");
  } else {
    Serial.print("[Falha] Erro: ");
    Serial.println(client.state());
  }
}

// ---------------------------------------------------------
// 1: STORE
// ---------------------------------------------------------
void salvarDadoOffline(float temperatura) {
  File file = SPIFFS.open("/dados_offline.json", FILE_APPEND);
  if (!file) {
    Serial.println("Erro critico ao abrir SPIFFS");
    return;
  }
  
  StaticJsonDocument<200> doc;
  doc["temperatura"] = temperatura;
  doc["timestamp"] = millis(); 
  
  String linhaJson;
  serializeJson(doc, linhaJson);
  
  file.println(linhaJson);
  file.close();
  
  Serial.print("[STORE] Dado salvo no chip: ");
  Serial.println(linhaJson);
}

// ---------------------------------------------------------
// 2: FORWARD
// ---------------------------------------------------------
void realizarForward() {
  // Verifica se o arquivo existe
  if (!SPIFFS.exists("/dados_offline.json")) return;

  File file = SPIFFS.open("/dados_offline.json", FILE_READ);
  if (!file || file.size() == 0) {
    return; // Arquivo vazio ou erro
  }

  Serial.println("\n=========================================");
  Serial.println("[FORWARD] Iniciando envio do historico...");
  
  bool sucessoTotal = true;

  // Lê o arquivo linha por linha
  while (file.available()) {
    String linha = file.readStringUntil('\n');
    linha.trim(); // Remove quebras de linha
    
    if (linha.length() > 0) {
      // Publica no "historico" para diferenciar das leituras em tempo real
      if (client.publish("tcc/adrian/temperatura/historico", linha.c_str())) {
        Serial.print(" -> Enviado: ");
        Serial.println(linha);
        delay(100); // delay para não travar o Broker
      } else {
        Serial.println(" -> Erro ao publicar pacote. Abortando Forward.");
        sucessoTotal = false;
        break; // Interromper o envio se o houver falha de conexão no broker
      }
    }
  }
  file.close();

  // Se bem sucedido, apaga o arquivo e libera a memória f 
  if (sucessoTotal) {
    SPIFFS.remove("/dados_offline.json");
    Serial.println("[FORWARD] Todos os dados sincronizados. Memoria local limpa.");
  } else {
    Serial.println("[FORWARD] Sincronizacao incompleta. Tentara novamente depois.");
  }
  Serial.println("=========================================\n");
}

void setup() {
  Serial.begin(115200);
  
  if(!SPIFFS.begin(true)){
    Serial.println("Erro ao montar SPIFFS");
    return;
  }
  Serial.println("Memoria Flash (SPIFFS) Pronta!");

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  sensors.begin();
}

void loop() {

  if (!client.connected()) {
    redeEstavaFora = true; // Sinaliza que esta off
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      if (WiFi.status() == WL_CONNECTED) {
        reconnect();
      }
    }
  } else {
    client.loop();
    
    // Se a rede estava fora e voltou, executa o FORWARD
    if (redeEstavaFora) {
      realizarForward();
      redeEstavaFora = false; // Reseta a flag para não ficar enviando em loop
    }
  }

  // Leitura do Sensor a cada 5 segundos
  unsigned long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    
    sensors.requestTemperatures(); 
    float temperatureC = sensors.getTempCByIndex(0);
    
    if (WiFi.status() == WL_CONNECTED && client.connected()) {
      char tempString[8];
      dtostrf(temperatureC, 1, 2, tempString);
      
      Serial.print("Online: Publicando ao vivo... ");
      if(client.publish("tcc/adrian/temperatura", tempString)) {
          Serial.println("[Sucesso]");
      } else {
          Serial.println("[Falha]");
      }
    } else {
      salvarDadoOffline(temperatureC);
    }
  }
}
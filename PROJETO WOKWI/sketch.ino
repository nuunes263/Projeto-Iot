// Incluir bibliotecas
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>

// Configurações WiFi
char SSIDName[] = "Wokwi-GUEST"; // Nome da rede WiFi
char SSIDPass[] = "";            // Senha da rede WiFi

// Configurações MQTT
char BrokerURL[] = "broker.hivemq.com"; // URL do broker MQTT
char MQTTClientName[] = "mqtt-mack-pub-sub"; // Nome do cliente MQTT
int BrokerPort = 1883; // Porta do broker MQTT

// Tópicos MQTT
char Topico_Ambiente[] = "temperatura/ambiente";
char Topico_Ar_condicionado[] = "temperatura/ar-condicionado";
char Topico_Chave[] = "temperatura/Chave";

// Pinos dos sensores
const int DHT_PIN_AMBIENTE = 15;
const int DHT_PIN_AR_CONDICIONADO = 23;

// Variáveis globais
WiFiClient espClient;
PubSubClient clienteMQTT(espClient);
DHTesp dhtSensorAmbiente;        // Objeto para o sensor de temperatura ambiente
DHTesp dhtSensorArCondicionado;  // Objeto para o sensor de ar-condicionado

unsigned long ultimoEnvio = 0;
const unsigned long intervalo = 10000; // Intervalo de 10 segundos
float temperaturaArCondicionadoExterno = 25.0; // Valor inicial para a temperatura do ar-condicionado

// Função de callback para receber mensagens MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  // Converte o payload para string
  String mensagem = "";
  for (int i = 0; i < length; i++) {
    mensagem += (char)payload[i];
  }

  // Verifica se a mensagem é do tópico do ar-condicionado
  if (String(topic) == Topico_Ar_condicionado) {
    temperaturaArCondicionadoExterno = mensagem.toFloat(); // Atualiza a temperatura externa
    Serial.print("Nova temperatura do ar-condicionado recebida: ");
    Serial.println(temperaturaArCondicionadoExterno);
  }
}

// Função para reconectar ao broker MQTT
void mqttReconnect() {
  while (!clienteMQTT.connected()) {
    Serial.println("Conectando-se ao broker MQTT...");
    if (clienteMQTT.connect(MQTTClientName)) {
      Serial.println("Conectado ao broker MQTT!");
      clienteMQTT.subscribe(Topico_Ar_condicionado); // Inscreve-se no tópico do ar-condicionado
      clienteMQTT.subscribe(Topico_Chave);          // Inscreve-se no tópico de chave
    } else {
      Serial.print("Falha na conexão, rc=");
      Serial.print(clienteMQTT.state());
      Serial.println(". Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

// Setup inicial
void setup() {
  // Configuração do MQTT e sensores
  clienteMQTT.setServer(BrokerURL, BrokerPort);
  clienteMQTT.setCallback(callback);
  dhtSensorAmbiente.setup(DHT_PIN_AMBIENTE, DHTesp::DHT22);
  dhtSensorArCondicionado.setup(DHT_PIN_AR_CONDICIONADO, DHTesp::DHT22);

  // Configuração do LED embutido e inicialização serial
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  Serial.print("Conectando-se ao Wi-Fi");

  // Conexão WiFi
  WiFi.begin(SSIDName, SSIDPass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("\nConectado ao Wi-Fi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// Loop principal
void loop() {
  // Verifica conexão com o broker MQTT
  if (!clienteMQTT.connected()) {
    mqttReconnect();
  }
  clienteMQTT.loop();

  // Enviar dados a cada intervalo
  unsigned long tempoAtual = millis();
  if (tempoAtual - ultimoEnvio >= intervalo) {
    ultimoEnvio = tempoAtual;

    // Leitura do sensor de ambiente
    TempAndHumidity tempUmidAmbiente = dhtSensorAmbiente.getTempAndHumidity();
    float temperaturaAmbiente = tempUmidAmbiente.temperature;

    // Publicar temperatura/ambiente
    char temperaturaAmbienteStr[8];
    dtostrf(temperaturaAmbiente, 1, 2, temperaturaAmbienteStr);
    clienteMQTT.publish(Topico_Ambiente, temperaturaAmbienteStr);
    Serial.print("Temperatura ambiente publicada: ");
    Serial.println(temperaturaAmbienteStr);

    // Publicar temperatura/ar-condicionado com valor externo recebido
    char temperaturaArCondicionadoStr[8];
    dtostrf(temperaturaArCondicionadoExterno, 1, 2, temperaturaArCondicionadoStr);
    clienteMQTT.publish(Topico_Ar_condicionado, temperaturaArCondicionadoStr);
    Serial.print("Temperatura ar-condicionado publicada: ");
    Serial.println(temperaturaArCondicionadoStr);
  }
}

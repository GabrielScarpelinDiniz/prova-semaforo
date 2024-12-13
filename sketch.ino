#include <WiFi.h>
#include <HTTPClient.h>

#define led_azul 10 // Pino utilizado para controle do led azul
#define led_verde 41 // Pino utilizado para controle do led verda
#define led_vermelho 40 // Pino utilizado para controle do led vermelho
#define led_amarelo 9 // Pino utilizado para controle do led azul

const int botao = 18;  // número da porta do botão
int estado_botao = 0;  // Variável do estado do botão

const int ldr = 4;  // número da porta do sensor ldr
const int limite_leitura=600; // limite de leitura do sensor ldr

int amarelo_piscante_ultima_troca = millis(); // variável que armazena o tempo de troca do amarelo piscante

int botao_ultima_leitura = millis(); // variável que armazena a ultima leitura do botão
const int tempo_debounce = 25; // guarda o tempo mínimo para debounce via software

HTTPClient http; // instância do objeto para criar um cliente http

int status_semaforo = 0; // 0 - VERDE | 1 - AMARELO | 2 - VERMELHO | 3 - AMARELO PISCANTE

// Guarda o tempo para troca de cada semaforo
const int tempo_verde_millis = 3000;
const int tempo_amarelo_millis = 2000;
const int tempo_vermelho_millis = 5000; 

// Ultima troca do semaforo
int ultima_troca_semaforo = millis();


void setup() {

  // Configuração inicial dos pinos para controle dos leds como OUTPUTs (saídas) do ESP32
  pinMode(led_azul,OUTPUT);
  pinMode(led_verde,OUTPUT);
  pinMode(led_vermelho,OUTPUT);
  pinMode(led_amarelo, OUTPUT);

  // Inicialização das entradas
  pinMode(botao, INPUT);
  pinMode(ldr, INPUT);

  // Define todos os leds como desligado
  digitalWrite(led_azul, LOW);
  digitalWrite(led_verde, LOW);
  digitalWrite(led_vermelho, LOW);
  digitalWrite(led_amarelo, LOW);

  Serial.begin(9600); // Configuração para debug por interface serial entre ESP e computador com baud rate de 9600

  WiFi.begin("Wokwi-GUEST", ""); // Conexão à rede WiFi aberta com SSID Wokwi-GUEST

  while (WiFi.status() == WL_CONNECT_FAILED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println("Conectado ao WiFi com sucesso!"); // Considerando que saiu do loop acima, o ESP32 agora está conectado ao WiFi (outra opção é colocar este comando dentro do if abaixo)

}

int vermelho_botao_contador = 0; // quantas vezes o botão foi pressionado em um intervalo de tempo de 1000ms
int primeiro_high_botao = millis();


void loop() {
  int ldr_status=analogRead(ldr);
  const int millis_atual = millis();

  if(ldr_status<=limite_leitura){
    if (status_semaforo != 3){
      status_semaforo = 3;
    }
    Serial.print("Esta escuro, amarelo piscante");
    Serial.println(ldr_status);
    if (millis_atual - amarelo_piscante_ultima_troca > 1000) {
      digitalWrite(led_amarelo, !digitalRead(led_amarelo));
      amarelo_piscante_ultima_troca = millis_atual;
    }

  }
  else {
    Serial.print("Claro, retirar do amarelo piscante");
    Serial.println(ldr_status);
    if (status_semaforo == 3) {
      status_semaforo = 0;
    }

    if (status_semaforo == 0) {
      digitalWrite(led_amarelo, LOW);
      digitalWrite(led_vermelho, LOW);
      digitalWrite(led_verde, HIGH);
      if (millis_atual - ultima_troca_semaforo > tempo_verde_millis) { 
        status_semaforo = 1;
        Serial.println("VERDE LIGADO");
        ultima_troca_semaforo = millis_atual;
      }
    }
    if (status_semaforo == 1) {
      Serial.println("AMARELO LIGADO");
      digitalWrite(led_amarelo, HIGH);
      digitalWrite(led_vermelho, LOW);
      digitalWrite(led_verde, LOW);
      if (millis_atual - ultima_troca_semaforo > tempo_amarelo_millis) {
        status_semaforo = 2;
        Serial.println("VERMELHO LIGADO");
        ultima_troca_semaforo = millis_atual;
        vermelho_botao_contador = 0;
        primeiro_high_botao = 0;
      }
    }
    if (status_semaforo == 2) {
      digitalWrite(led_amarelo, LOW);
      digitalWrite(led_vermelho, HIGH);
      digitalWrite(led_verde, LOW);
      estado_botao = leitura_botao(millis_atual);

      if (estado_botao && vermelho_botao_contador == 0) {
        Serial.println("VERDE LIGADO");
        vermelho_botao_contador += 1;
        primeiro_high_botao = millis_atual;
      }
      if (estado_botao && millis_atual - primeiro_high_botao < 1000) {
        vermelho_botao_contador += 1;
      }


      if (millis_atual - primeiro_high_botao > 1000 && vermelho_botao_contador >= 3) {
        if(WiFi.status() == WL_CONNECTED){ // Se o ESP32 estiver conectado à Internet
          String serverPath = "http://www.google.com.br/"; // Endpoint da requisição HTTP

          http.begin(serverPath.c_str());

          int httpResponseCode = http.GET(); // Código do Resultado da Requisição HTTP

          if (httpResponseCode>0) {
            Serial.print("HTTP código da resposta: ");
            Serial.println(httpResponseCode);
            String payload = http.getString();
            Serial.println(payload);
            }
          else {
            Serial.print("Código de erro: ");
            Serial.println(httpResponseCode);
            }
            http.end();
        }
        else {
          Serial.println("WiFi desconectado");
        }
      } else if (millis_atual - primeiro_high_botao > 1000 && vermelho_botao_contador >= 1) {
        status_semaforo = 0;
        ultima_troca_semaforo = millis_atual;
      }
      if (millis_atual - ultima_troca_semaforo > tempo_amarelo_millis) {
        status_semaforo = 0;
        ultima_troca_semaforo = millis_atual;
      }
    }
  }
}

bool leitura_botao(int millis_atual) {
  if (millis_atual - botao_ultima_leitura > tempo_debounce) {
    botao_ultima_leitura = millis_atual;
    return digitalRead(botao);
  } else {
    return estado_botao;
  }
}

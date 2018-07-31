//Keypad includes
#include <Key.h>
#include <Keypad.h>

//RFID includes
#include <SPI.h>
#include <RFID.h>

#define SDA_DIO 53
#define RESET_DIO 9

RFID RC522(SDA_DIO, RESET_DIO);

//Ethernet includes
#include <Ethernet.h>
#include <SPI.h>
#include <ArduinoJson.h>

/* Relay variables */
const int RELAY_PIN = 2;

/* Ethernet Variables */
const byte MAC[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

//const char* host = "albergue-api.herokuapp.com";
const byte SERVER[] = { 192, 168, 1, 71 };
const IPAddress ip(192, 168, 1, 177);

EthernetClient client;

/* RFID Users */
//const byte Usuario1[4] = {0xB4, 0xDC, 0x11, 0x71} ; //código del usuario 1
//const byte Usuario2[4] = {0x55, 0xF6, 0x1F, 0xA6} ; //código del usuario 2
const String usuario1 = "180220171138";
const String usuario2 = "852463116626";

/* Status LEDs Variables */
const int CORRECT_LED = 38;
const int INCORRECT_LED = 40;
const int STATUS_LED = 42;

const byte ROWS = 4;
const byte COLS = 4;
const char NUMPAD_KEYS[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

const byte ROW_PINS[ROWS] = {22, 24, 26, 28}; //connect to the row pinouts of the keypad
const byte COL_PINS[COLS] = {30, 32, 34, 36}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(NUMPAD_KEYS), ROW_PINS, COL_PINS, ROWS, COLS );
void setup() {
  Serial.begin(9600);
  //RFID
  SPI.begin();
  RC522.init();

  EthernetConnection();
  pinMode(CORRECT_LED, OUTPUT);
  pinMode(INCORRECT_LED, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
}

void loop() {
  Serial.println("Presione A para escribir código o B para usar tarjeta");
  while (true) {
    char key = keypad.getKey();
    if (key == 'A') {
      Serial.println("Ingrese código y presione D para finalizar");
      digitalWrite(STATUS_LED, HIGH);
      String code = getCodeFromKeyPad();

      String responseCode = auth(code);
      digitalWrite(STATUS_LED, LOW);
      if (responseCode.equals("200")) {
        Serial.println("Sesión iniciada");
        digitalWrite(CORRECT_LED, HIGH);
        unlock();
        digitalWrite(CORRECT_LED, LOW);
      } else if (responseCode.equals("404")) {
        Serial.println("\nCódigo Incorrecto");
        Serial.println("Presione A para escribir código");
        digitalWrite(INCORRECT_LED, HIGH);
        delay(2000);
        digitalWrite(INCORRECT_LED, LOW);
      } else if (responseCode.equals("401")) {
        Serial.println("\n Usuario Bloqueado");
        digitalWrite(INCORRECT_LED, HIGH);
        delay(2000);
        digitalWrite(INCORRECT_LED, LOW);
      }
    }
    if (RC522.isCard()) {
      String newCard = "";
      Serial.println("Reading card...");
      RC522.readCardSerial();
      Serial.print(("Card UID:"));
      for (int i = 0; i < 5; i++) {
        newCard += RC522.serNum[i];
      }
      Serial.print("     ");
      Serial.print(newCard);

      bool canAccess = newCard.equals(usuario1) || newCard.equals(usuario2); 
      if (canAccess) {
        digitalWrite(CORRECT_LED, HIGH);
        unlock();
        digitalWrite(CORRECT_LED, LOW);
      } else if (!canAccess && newCard.length() == usuario1.length()){
        Serial.println("Acceso denegado...");
      }
    }
  }
}

/**
  Captura el código que el usuario introduce en el numpad.
  @return String con el valor del código.
*/
String getCodeFromKeyPad() {
  String code = "";
  do {
    char keyChar = keypad.getKey();
    if (keyChar) {
      digitalWrite(STATUS_LED, LOW);
      delay(200);
      digitalWrite(STATUS_LED, HIGH);
      if (keyChar == 'D') {
        break;
      } else if (keyChar == 'C') {
        int codeLength = code.length();
        if (codeLength != 0) {
          code.remove(codeLength - 1);
        }
        Serial.print("\n" + code);
      } else {
        code += keyChar;
        Serial.print(keyChar);
      }
    }
  } while (true);
  return code;
}

/**
  Realiza la llamada HTTP para autentificar al usuario
  @param code Codigo a autentificar
  @return String Código del status de la llamada
  200: OK, 404: Not Found, 401: Usuario bloqueado
*/
String auth(String code) {
  String responseCode = "";
  String result = "";
  String data = String("GET /people/auth/") + code + String(" HTTP/1.1");
  Serial.print("\n[Connecting to  ... ");
  //  Serial.println(host);
  //host 80
  if (client.connect(SERVER, 9000))
  {
    Serial.println("connected");
    Serial.println("[Sending a request]");
    client.println(data);
    //    client.println("Host: albergue-api.herokuapp.com");
    client.println("Host: 192.168.1.71:9000");
    client.println("User-Agent: arduino/1.8.3");
    client.println("Connection: close");
    client.println();
    Serial.println("[Response:]");
    while (client.connected())
    {
      if (client.available())
      {
        String line = client.readStringUntil('\n');
        if (line.startsWith("HTTP/1.1")) {
          responseCode = line.substring(9, 12);
          Serial.println(responseCode);
        }
        Serial.println("\t" + line);
      }
    }
  }
  else
  {
    Serial.println("Cannot connect to Server");
    Serial.println();
  }


  while (client.connected() && !client.available()) delay(1);
  while (client.connected() || client.available())
  {
    char c = client.read();
    result = result + c;
  }
  client.stop();
  Serial.println("\n[Disconnected]");
  result.replace('[', ' ');
  result.replace(']', ' ');
  Serial.println(result);
  return responseCode;
}

/**
  Realiza la configuración Ethernet
*/
void EthernetConnection() {
  if (Ethernet.begin(MAC) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(MAC, ip);
  }
  Serial.println("connected");
  Serial.println(Ethernet.localIP());
  delay(1000);
}


/**
  Desbloquea la cerradura
*/
void unlock()
{
  digitalWrite(RELAY_PIN, LOW);
  Serial.println("\nAbierto");
  delay(2000);
  digitalWrite(RELAY_PIN, HIGH);
  Serial.println("Cerrada");
}


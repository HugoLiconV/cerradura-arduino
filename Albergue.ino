//Keypad includes
#include <Key.h>
#include <Keypad.h>

//RFID includes
#include <SPI.h>
//#include <MFRC522.h>
#include <RFID.h>

//#define RST_PIN  9    //Pin 9 para el reset del RC522
//#define SS_PIN  53   //Pin 53 para el SS (SDA) del RC522

#define SDA_DIO 53
#define RESET_DIO 9

RFID RC522(SDA_DIO, RESET_DIO);

const String usuario1 = "180220171138";
const String usuario2 = "852463116626";

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

/* RFID Configuration */
//MFRC522 mfrc522(SS_PIN, RST_PIN); ///Creamos el objeto para el RC522

/* RFID Variables */
byte ActualUID[4]; //almacenará el código del Tag leído

/* RFID Users */
const byte Usuario1[4] = {0xB4, 0xDC, 0x11, 0x71} ; //código del usuario 1
const byte Usuario2[4] = {0x55, 0xF6, 0x1F, 0xA6} ; //código del usuario 2

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
  //  mfrc522.PCD_Init();
  RC522.init();
  EthernetConnection();
  pinMode(CORRECT_LED, OUTPUT);
  pinMode(INCORRECT_LED, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  Serial.println("Presione A para escribir código o B para usar tarjeta");
}

void loop() {
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
    } else if (key == 'B') {
      Serial.println("Esperando Tarjeta...");
      bool foundCard = false;
      while (!foundCard) {
        //        if ( mfrc522.PICC_IsNewCardPresent()) {
        //        Serial.println("PICC_IsNewCardPresent");
        //Seleccionamos una tarjeta
        //          if ( mfrc522.PICC_ReadCardSerial()) {
        if (RC522.isCard()) {
          // Enviamos serialemente su UID
          String newCard = "";
          Serial.println("Reading card...");
          RC522.readCardSerial();
          Serial.print(F("Card UID:"));
          //            for (byte i = 0; i < mfrc522.uid.size; i++) {
          //              Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
          //              Serial.print(mfrc522.uid.uidByte[i], HEX);
          //              ActualUID[i] = mfrc522.uid.uidByte[i];
          //            }
          for (int i = 0; i < 5; i++)
          {
            //      Serial.println(RC522.serNum[i], DEC);
            newCard += RC522.serNum[i];
            //Serial.print(RC522.serNum[i],HEX); //to print card detail in Hexa Decimal format
          }
          Serial.print("     ");
          Serial.print(newCard);
          //comparamos los UID para determinar si es uno de nuestros usuarios
          //            if (compareArray(ActualUID, Usuario1) || compareArray(ActualUID, Usuario2)) {
          if (newCard.equals(usuario1) || newCard.equals(usuario2)) {
            digitalWrite(CORRECT_LED, HIGH);
            unlock();
            digitalWrite(CORRECT_LED, LOW);
          } else {
            Serial.println("Acceso denegado...");
          }
          // Terminamos la lectura de la tarjeta tarjeta  actual
          //            mfrc522.PICC_HaltA();
          if (newCard.length() == usuario1.length()) {
            foundCard = true;
          }
        }
      }
    }
  }
}

/**
  Compara los arrays con los ID de las tarjetas

  @param array1 Primer arreglo para comparar
  @param array2 Segundo arreglo para comparar
  @return Boolean true si ambos arreglos son iguales, false si no lo son.
*/
boolean compareArray(byte array1[], byte array2[])
{
  if (array1[0] != array2[0])return (false);
  if (array1[1] != array2[1])return (false);
  if (array1[2] != array2[2])return (false);
  if (array1[3] != array2[3])return (false);
  return (true);
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


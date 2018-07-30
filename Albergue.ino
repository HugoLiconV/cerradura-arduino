//Keypad includes
#include <Key.h>
#include <Keypad.h>

//RFID includes
#include <SPI.h>
#include <MFRC522.h>

//Ethernet includes
#include <Ethernet.h>
#include <SPI.h>
#include <ArduinoJson.h>

//Relay variables
int relay = 2;

// Ethernet Variables
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

//const char* host = "albergue-api.herokuapp.com";
byte server[] = { 192, 168, 1, 67 };
IPAddress ip(192, 168, 1, 177);

EthernetClient client;
String result;
String responseCode;

//RFID Configuration
#define RST_PIN  9    //Pin 9 para el reset del RC522
#define SS_PIN  53   //Pin 53 para el SS (SDA) del RC522
MFRC522 mfrc522(SS_PIN, RST_PIN); ///Creamos el objeto para el RC522

typedef struct {
  int id;
  String codigo;
  String nombre;
} Persona;

Persona personas[10];

byte ActualUID[4]; //almacenará el código del Tag leído
//RFID Users
byte Usuario1[4] = {0xB4, 0xDC, 0x11, 0x71} ; //código del usuario 1
byte Usuario2[4] = {0x55, 0xF6, 0x1F, 0xA6} ; //código del usuario 2

//Leds indicadores para la introduccion del codigo
int ledCorrect = 38;
int ledIncorrect = 40;
int ledStatus = 42;

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {22, 24, 26, 28}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {30, 32, 34, 36}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

void setup() {
  Serial.begin(9600);
//  EthernetConnection();
  pinMode(ledCorrect, OUTPUT);
  pinMode(ledIncorrect, OUTPUT);
  pinMode(ledStatus, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH);
  
//  personas[0] = Persona{1, "1234", "Juan Perez" };
//  personas[1] = Persona{2, "4321", "Hugo Licon" };
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  Serial.println("connected");
  Serial.println(Ethernet.localIP());
  delay(1000);
  //RFID
  SPI.begin();        //Iniciamos el Bus SPI
  mfrc522.PCD_Init(); // Iniciamos el MFRC522  
}

void loop() {
  Serial.println("Presione A para escribir código");
  while (true) {
    char key = keypad.getKey();
    String code = "";
    if (key == 'A') {
      Serial.println("Ingrese código y presione D para finalizar");
      digitalWrite(ledStatus, HIGH);
      code = getCodeFromKeyPad();

//      Persona loggedUser = verifyUser(code);
//      200 = OK 404 = Not Found 401 = Usuario bloqueado
      //hacer request para auth
      auth();
      digitalWrite(ledStatus, LOW);
      if (responseCode.equals("200")) {
        Serial.println("Sesión iniciada");
//        print("Hola " + loggedUser.nombre);
        digitalWrite(ledCorrect, HIGH);   // turn the LED on (HIGH is the voltage level)
        unlock();
//        delay(2000);
        digitalWrite(ledCorrect, LOW);
      } else if (responseCode.equals("404")){
        Serial.println("\nCódigo Incorrecto");
        Serial.println("Presione A para escribir código");
        digitalWrite(ledIncorrect, HIGH);   // turn the LED on (HIGH is the voltage level)
        delay(2000);
        digitalWrite(ledIncorrect, LOW);
      } else if (responseCode.equals("401")) {
        Serial.println("\n Usuario Bloqueado");
      }
    }

    if ( mfrc522.PICC_IsNewCardPresent())
    {
      Serial.println("PICC_IsNewCardPresent");
      //Seleccionamos una tarjeta
      if ( mfrc522.PICC_ReadCardSerial())
      {
        // Enviamos serialemente su UID
        Serial.println("Reading card...");
        Serial.print(F("Card UID:"));
        for (byte i = 0; i < mfrc522.uid.size; i++) {
          Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
          Serial.print(mfrc522.uid.uidByte[i], HEX);
          ActualUID[i] = mfrc522.uid.uidByte[i];
        }
        Serial.print("     ");
        //comparamos los UID para determinar si es uno de nuestros usuarios
        if (compareArray(ActualUID, Usuario1)) {
          Serial.println("Acceso concedido...");
          digitalWrite(ledCorrect, HIGH);
//          postRegister();
          unlock();
//          delay(5000);
          digitalWrite(ledCorrect, LOW);
        }
        else if (compareArray(ActualUID, Usuario2))
        {
          Serial.println("Acceso concedido...");
          digitalWrite(ledCorrect, HIGH);
//          postRegister();
          unlock();
//          delay(5000);
          digitalWrite(ledCorrect, LOW);
        }
        else
          Serial.println("Acceso denegado...");
        // Terminamos la lectura de la tarjeta tarjeta  actual
        mfrc522.PICC_HaltA();

      }
    }
  }
}

//Función para comparar dos vectores
 boolean compareArray(byte array1[],byte array2[])
{
  if(array1[0] != array2[0])return(false);
  if(array1[1] != array2[1])return(false);
  if(array1[2] != array2[2])return(false);
  if(array1[3] != array2[3])return(false);
  return(true);
}

/*
** Verifica el codigo dado con los usuarios guardados
*/
Persona verifyUser(String code) {
  Persona loggedUser;
  for (int i = 0; i < sizeof(personas); i++) {
    if (code.equals(personas[i].codigo)) {
      Serial.println("\nCódigo Correcto");
      loggedUser = personas[i];
      break;
    }
  }
  return loggedUser;
}

String getCodeFromKeyPad() {
  String code = "";
  do {
    char keyChar = keypad.getKey();
    if (keyChar) {
      digitalWrite(ledStatus, LOW);
      delay(200);
      digitalWrite(ledStatus, HIGH);
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

void print(String text) {
  Serial.println(text);
}

void auth(){
  
//  int userId = 1;
//  String data = String("{\"userId\":") + userId + String("}");
//  char data[]="{\"userId\": 1}";
  Serial.print("\n[Connecting to  ... ");
//  Serial.println(host);
//host 80
  if (client.connect(server, 9000))
  {
    Serial.println("connected");
    Serial.println("[Sending a request]");
    client.println("GET /people/auth/1234 HTTP/1.1");
//    client.println("Host: albergue-api.herokuapp.com");
    client.println("Host: 192.168.1.67:9000");
    client.println("User-Agent: arduino/1.8.3");
//    client.println("Content-Type: application/json");
//    client.print("Content-Length: ");
//    client.println(data.length());
    client.println("Connection: close");
    client.println();
//    client.print(data);

    Serial.println("[Response:]");
    while (client.connected())
    {
      if (client.available())
      {
        String line = client.readStringUntil('\n');
        if(line.startsWith("HTTP/1.1")) {
          responseCode = line.substring(9, 12);
          Serial.println(responseCode);
        }
        Serial.println(line);
      }
    }
  }
  else
  {
    Serial.println("Cannot connect to Server");
    Serial.println();
  }


  while (client.connected() && !client.available()) delay(1); //waits for data
  while (client.connected() || client.available())
  { //connected or data available
    char c = client.read(); //gets byte from ethernet buffer
    result = result + c;
  }
  client.stop(); //stop client
  Serial.println("\n[Disconnected]");
  result.replace('[', ' ');
  result.replace(']', ' ');
  Serial.println(result);
}

void EthernetConnection(){
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  Serial.println("connected");
  Serial.println(Ethernet.localIP());
  delay(1000);
}

void unlock()
{
  digitalWrite(relay, LOW);
  Serial.println("Abierto");
  delay(3000);
  digitalWrite(relay, HIGH);
  Serial.println("Cerrada");
}


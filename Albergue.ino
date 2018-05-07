//Keypad includes
#include <Key.h>
#include <Keypad.h>

//RFID includes
#include <SPI.h>
#include <MFRC522.h>

//RFID Configuration
#define RST_PIN  9    //Pin 9 para el reset del RC522
#define SS_PIN  53   //Pin 10 para el SS (SDA) del RC522
MFRC522 mfrc522(SS_PIN, RST_PIN); ///Creamos el objeto para el RC522

typedef struct {
  String codigo;
  String nombre;
} Persona;

Persona personas[10];

byte ActualUID[4]; //almacenará el código del Tag leído
//RFID Users
byte Usuario1[4] = {0xB4, 0xDC, 0x11, 0x71} ; //código del usuario 1
byte Usuario2[4] = {0xC1, 0x2F, 0xD6, 0x0E} ; //código del usuario 2

//Leds indicadores para la introduccion del codigo
int ledCorrect = 23;
int ledIncorrect = 25;
int ledStatus = 27;

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
  pinMode(ledCorrect, OUTPUT);
  pinMode(ledIncorrect, OUTPUT);
  pinMode(ledStatus, OUTPUT);
 
  personas[0] = Persona{ "1234", "Juan Perez" };
  personas[1] = Persona{ "4321", "Hugo Licon" };

  //RFID
  SPI.begin();        //Iniciamos el Bus SPI
  mfrc522.PCD_Init(); // Iniciamos el MFRC522
    // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
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

      Persona loggedUser = verifyUser(code);
      digitalWrite(ledStatus, LOW);
      if (loggedUser.nombre.length() > 0) {
        Serial.println("Iniciando Sesion");
        print("Hola " + loggedUser.nombre);
        digitalWrite(ledCorrect, HIGH);   // turn the LED on (HIGH is the voltage level)
        delay(2000);
        digitalWrite(ledCorrect, LOW);
      } else {
        Serial.println("\nCódigo Incorrecto");
        Serial.println("Presione A para escribir código");
        digitalWrite(ledIncorrect, HIGH);   // turn the LED on (HIGH is the voltage level)
        delay(2000);
        digitalWrite(ledIncorrect, LOW);
      }
    }

    if ( mfrc522.PICC_IsNewCardPresent())
    {
      //Seleccionamos una tarjeta
      if ( mfrc522.PICC_ReadCardSerial())
      {
        // Enviamos serialemente su UID
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
          digitalWrite(LED_BUILTIN, HIGH);
          delay(5000);
          digitalWrite(LED_BUILTIN, HIGH);
        }
        else if (compareArray(ActualUID, Usuario2))
        {
          Serial.println("Acceso concedido...");
          digitalWrite(LED_BUILTIN, HIGH);
          delay(5000);
          digitalWrite(LED_BUILTIN, HIGH);
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


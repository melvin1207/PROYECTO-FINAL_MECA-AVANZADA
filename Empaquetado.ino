//librerias
#include <HCSR04.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include "PubSubClient.h"

//Objeto del sensor ultrasonico
HCSR04 hc(18, 19); //initialisation class HCSR04 (trig pin , echo pin)

//servomotor
Servo servo;

//constantes de la conexion WIFI
//conexiones
const char* ssid = "BUAP_Estudiantes";
const char* password = "f85ac21de4";
const char* mqtt_server = "test.mosquitto.org";


//constantes de los pines
const int pinServo = 2;
const int PARO = 21;
const int LED = 12;
const int sensor = 13;
const int START = 15;
const int in2 = 16;
const int in1 = 17;

//variables
bool a = true;

String startMensaje = "";
String startLedMensaje = "";
String paroMensaje = "";
String paroLedMensaje = "";
String sensorMensaje = "";
String panMensaje = "";
String servoMensaje = "";

WiFiClient espClient;
PubSubClient client(espClient);

//INICIALIZAR WIFI
void setup_wifi() {
  delay(10);
  Serial.print("");
  Serial.print("Conectandose a: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Wifi conectado");
  Serial.print("direccion IP: ");
  Serial.println(WiFi.localIP());
}

//PARA RECONECTAR AL SERVIDOR
void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando la conexion....");

    if (client.connect("3bd6bdec-6da1-418b-9507-4b3f5dd57f25")) {
      Serial.println("Conectado");
      client.subscribe("MecaAvanzada/Empaquetado");
      client.subscribe("MecaAvanzada/Paro");
      Serial.println("suscrito al tema");
    } else {
      Serial.print("falla");
      Serial.print(client.state());
      Serial.print("Nuevo intento en 5 segundos");
      delay(500);
    }
  }
}

//FUNCION PARA RECIBIR LOS MENSAJES
void callback(char* tema, byte* mensaje, unsigned int tamano){
  String mensajeTemp;

  for(int i = 0; i < tamano; i++){
    mensajeTemp += (char)mensaje[i];
  }

  Serial.println(mensajeTemp);

  if(strcmp(tema, "MecaAvanzada/Paro") == 0){
    paroMensaje = mensajeTemp;
  } else if (strcmp(tema, "MecaAvanzada/Empaquetado") == 0) {
    startMensaje = mensajeTemp;
  } 
}

void STOP() {
   if(digitalRead(PARO) == 1 || paroMensaje == "0Paro"){
    servo.write(0);
    while(digitalRead(PARO) == 1 || paroMensaje == "0Paro"){
      client.loop();
      paroLedMensaje = "encendido";
      client.publish("MecaAvanzada/empaquetadoParoLed", paroLedMensaje.c_str(), true);
      digitalWrite(in1, LOW);
      digitalWrite(in2, LOW);
      digitalWrite(LED, HIGH);
    }
  } else {
    paroLedMensaje = "apagado";
    client.publish("MecaAvanzada/empaquetadoParoLed", paroLedMensaje.c_str(), true);
    digitalWrite(LED, LOW); 
  }
}

void setup() {
  //inicialización serial
  Serial.begin(115200);

  setup_wifi();

  //inicialización del servo
  servo.attach(pinServo, 500, 2500);

  //SERVER DEL MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //inicializacion de ls entradas y salidas de la tarjeta
  pinMode(sensor, INPUT);
  pinMode(START, INPUT_PULLUP);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(PARO, INPUT_PULLUP);
  pinMode(LED, OUTPUT);

  //se inicializa el servo
  servo.write(0);
  servoMensaje = "abajo";
      client.publish("MecaAvanzada/empaquetadoServo", servoMensaje.c_str(), true);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  //se verifica otra vez el boton de PARO
  STOP();

  //se verifica que el boton de inico este encendido o no
  if(digitalRead(START) == 1 || startMensaje == "inicioEmpaquetado") {

    panMensaje = "no";
    client.publish("MecaAvanzada/empaquetadoPan", panMensaje.c_str(), true);
    startLedMensaje = "encendido";
    client.publish("MecaAvanzada/empaquetadoLed", startLedMensaje.c_str(), true);
    delay(200);

     //se verifica otra vez el boton de PARO
    STOP();
 
    //si el boton start esta activado entonces el motor estara encendido
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);


    //si la banda esta avanzando y la caja ya llego a la posicion
    if (digitalRead(sensor) == 1){

      sensorMensaje = "detectado";
      client.publish("MecaAvanzada/empaquetadoSensor", sensorMensaje.c_str(), true);

      //se verifica otra vez el boton de PARO
      STOP();
     
      //si el sensor detecta la caja, el motor se apaga
      digitalWrite(in1, LOW);
      digitalWrite(in2, LOW);
      
      //se hace una espera hasta que el pan llegue a la estación
      while(a == true){

        //se verifica otra vez el boton de PARO
        STOP();

        //hasta que la distancia sea menor que 5, la espera termina
        if ((hc.dist()) < 5){
          a = false; 
          panMensaje = "detectado";
          client.publish("MecaAvanzada/empaquetadoPan", panMensaje.c_str(), true);
          delay(2000);
        } else {
          
          panMensaje = "no";
          client.publish("MecaAvanzada/empaquetadoPan", panMensaje.c_str(), true);
        }
      }
      
      //se baja el servo para cerrar la caja y se da una espera de 2 seg
      servo.write(85);
      servoMensaje = "abajo";
      client.publish("MecaAvanzada/empaquetadoServo", servoMensaje.c_str(), true);
      delay(2000);
        
      //se vuelve a inicializar la variable de control
      a = true;

      //se hace un ciclo donde mientras la caja vaya avanzando ya cerrada el servo no suba y la cinta no se detenga
      while(a == true) {

        //se verifica otra vez el boton de PARO
        STOP();

        if(digitalRead(sensor) == 1){
          sensorMensaje = "detectado";
          client.publish("MecaAvanzada/empaquetadoSensor", sensorMensaje.c_str(), true);

          //si el sensor sigue detectando la caja la cinta no se detendrra
          digitalWrite(in1, HIGH);
          digitalWrite(in2, LOW);
        } else {
          sensorMensaje = "no";
          client.publish("MecaAvanzada/empaquetadoSensor", sensorMensaje.c_str(), true);
          delay(200);
          //si el sensor ya no detecta la caja detienen la cinta y reinizializa la banda
          digitalWrite(in1, LOW);
          digitalWrite(in2, LOW);
          a = false; 
        }
      }

      //se sube el brazo que cierra la , se da un pequeño delay y se reinicizliza la variable de control
      servo.write(0);
      servoMensaje = "arriba";
      client.publish("MecaAvanzada/empaquetadoServo", servoMensaje.c_str(), true);
      delay(200);
      
      a = true;
    } else{
      sensorMensaje = "no";
      client.publish("MecaAvanzada/empaquetadoSensor", sensorMensaje.c_str(), true);
      delay(200);
    } 
  } else{

    //se envia para apagar el led
    startLedMensaje = "apagado";
    client.publish("MecaAvanzada/empaquetadoLed", startLedMensaje.c_str(), true);
    delay(200);
    
    // si no esta presionado el boton encendido entonces la cinta no avanza
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }
}



/*
**************************************************************
NUOVA VERSIONE DELL'ANTIFURTO PER LAURA SU BOX AZDELIVERY
prima versione dicembre 2024
************************************************************

   la scheda deve essere esp32 Dev Module     
   il display è 240 di larghezza e 320 di ampiezza

   pin di  I/O ACCESSIBILI SULLA SCHEDA 

   IO32  RFID PN532 SCL ARANCIONE
   IO25  MORSETTO N.1 ALLARME  filo  rosso +3,2 = allarme
   IO26
   IO33  RFID PN532 SDA ROSSO

   AO (IO36) INPUT ONLY
   IO35      INPUT ONLY  MORSETTO 2  GIALLO PORTONE QUANDO CHIUSO A MASSA
   IO34      INPUT ONLY  MORSETTO 3 VERDE   MICROONDE SI ATTIVA   A MASSA  
   IO39      INPUT ONLY  MORSETTO 4 PANIC BUTTON  IL FILO E' A MASSA QUANDO TIRATO AGISCE IL PULLUP E VA A 3,3 

   IO33  rfid PN532 SCL
   IO32  rfid PN532 SDA
   IO21  beeper
   IO25  portone USCITA 4 

Il prototipo è costituito da una ESP32 Dev Module montata su box azdelivery 
e da un lettore rfid PN532 collegato in I2C tramite SDA pin 33  e SCL pin 32 

************  logica di funzionamento 
l'attivazione e la disattivazione dell'allarme avviene tramite avvicinamento dei tag rfid in logica flip flop 
esistono dei tag speciali colorati in rosso che invece permettono di accedere alle pagine di setup 

******************** BYTE STATO INDICA LA NUOVA STRUTTURA  DELLE PAGINE
0=PAGINA VUOTA	
1=PAGINA INIZIALE 
2=PAGINA CONTATORE USCITA	
3=PAGINA CONTATORE  ENTRATA	
4=PAGINA SISTEMA ARMATO	
5=PAGINA ALLARME IN CORSO	
6=PAGINA SETUP	
7=PAGINA LOG 
8=PAGINA PREVISIONI                 // ATTENTO VIENE DOPO LA PAGINA METEO
9=PAGINA METEO

**********************  AUTOMAZIONI SOMIS quando di accende il led dell'allarme V0 scatta una automazione che manda una mail e una notifica

************************************************************
  Blynk is a platform with iOS and Android apps to control
  ESP32, Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build mobile and web interfaces for any
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: https://www.blynk.io
    Sketch generator:           https://examples.blynk.cc
    Blynk community:            https://community.blynk.cc
    Follow us:                  https://www.fb.com/blynkapp
                                https://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************
  This example runs directly on ESP32 chip.

  NOTE: This requires ESP32 support package:
    https://github.com/espressif/arduino-esp32

  Please be sure to select the right ESP32 module
  in the Tools -> Board menu!

  Change WiFi ssid, pass, and Blynk auth token to run :)
  Feel free to apply it to any other example. It's simple!
 ************************************************************
 */


// #define BLYNK_PRINT Serial                             // Comment this out to disable prints and save space  
// #define BLYNK_DEBUG                                    // Optional, this enables more detailed prints

#define BLYNK_TEMPLATE_ID   "TMPLv7zpgNaH"
#define BLYNK_TEMPLATE_NAME "New Template"
#define BLYNK_AUTH_TOKEN    "ofeQmHiaxmBuUpAO3zdovlGOcvtJhDm2"

char    ssid1 []              ="TIM-74712152";                                                            
char    pass1 []              ="Lallina1987!";
 

char    ssid2 []              ="netsisto";
char    pass2 []              ="bruttazigolaccia";

/***************************************************
 * Change these settings to match your need
 * Professional settings
 ***************************************************/
#define BLYNK_DEFAULT_DOMAIN     "blynk.cloud"
#define BLYNK_DEFAULT_PORT       80
#define BLYNK_DEFAULT_PORT_SSL   443
#define BLYNK_HEARTBEAT          40      // Heartbeat period in seconds.
#define BLYNK_TIMEOUT_MS         6000UL  // Network timeout in milliseconds.
#define BLYNK_MSG_LIMIT          40      // Limit the amount of outgoing commands per second.
#define BLYNK_MAX_READBYTES      256     // Limit the incoming command length.   
#define BLYNK_MAX_SENDBYTES      128     // Limit the outgoing command length.


#include <NTPClient.h>                // per orologio
#include <WiFi.h>
#include <WiFiClient.h>
#include <Arduino_JSON.h>                                                  // per meteo
#include <HTTPClient.h>                                                    // per meteo
// #include <string>

#include <BlynkSimpleEsp32.h>
#include "Adafruit_ILI9341.h"
#include <XPT2046_Touchscreen.h> 
#include <Adafruit_PN532.h>
#include <Wire.h>

String openWeatherMapApiKey = "5e40b795dc2591c5aa19620e2916c593";          // per meteo
//String serverPath =  "http://api.openweathermap.org/data/2.5/weather?lat=41.8947&lon=12.4839&APPID=" + openWeatherMapApiKey+ "&units=metric&lang=it" ;
String jsonBuffer;                                                         // per meteo

//  #include "time.h"
//  #include <Adafruit_PN532.h>
//  #include <PN532_I2C.h>
//  #include <PN532.h>
//  #include <NfcAdapter.h>



#define I2C_SDA 33      // filo rosso
#define I2C_SCL 32      // filo arancione

#define TOUCH_IRQ 27    // questo è quello valido PER IL MODELLO AZ_TOUCH_MOD_BIG_TFT

//                  __Pin definitions for the ESP32__ 
#define TFT_CS   5
#define TFT_DC   4
#define TFT_LED  15  
#define TFT_MOSI 23
#define TFT_CLK  18
#define TFT_RST  22
#define TFT_MISO 19
#define TFT_LED  15  
#define TOUCH_CS 14

// RFID
#define SS_PIN    25    // SPI Slave Select Pin
#define IRQ_PIN   26    // IRQ pin - not in use  
#define RST_PIN   27    // SPI Reset Pin


/*____Calibrate Touchscreen_____*/
#define MINPRESSURE 10      // minimum required force for touch event
#define TS_MINX 370
#define TS_MINY 470
#define TS_MAXX 3700
#define TS_MAXY 3600
/*______End of Calibration______*/

// Color definitions
#define BLACK 0x0000       /*   0,   0,   0 */
#define NAVY 0x000F        /*   0,   0, 128 */
#define DARKGREEN 0x03E0   /*   0, 128,   0 */
#define DARKCYAN 0x03EF    /*   0, 128, 128 */
#define MAROON 0x7800      /* 128,   0,   0 */
#define PURPLE 0x780F      /* 128,   0, 128 */
#define OLIVE 0x7BE0       /* 128, 128,   0 */
#define LIGHTGREY 0xC618   /* 192, 192, 192 */
#define DARKGREY 0x7BEF    /* 128, 128, 128 */
#define BLUE 0x001F        /*   0,   0, 255 */
#define GREEN 0x07E0       /*   0, 255,   0 */
#define CYAN 0x07FF        /*   0, 255, 255 */
#define RED 0xF800         /* 255,   0,   0 */
#define MAGENTA 0xF81F     /* 255,   0, 255 */
#define YELLOW 0xFFE0      /* 255, 255,   0 */
#define WHITE 0xFFFF       /* 255, 255, 255 */
#define ORANGE 0xFD20      /* 255, 165,   0 */
#define GREENYELLOW 0xAFE5 /* 173, 255,  47 */
#define PINK 0xF81F


unsigned long currenttime = millis();      // Current time
unsigned long p1time = 0;                  // tempo di refresh data/ora
unsigned long p2time = 0;                  // tempo di scrittura unattended dei dati su blink
unsigned long p3time = 0;                  // tempo di reset a pagina vuota  per inattività 
const long    t1time = 5000;               // Define timeout time in milliseconds (example: 5000ms  = 5s)  
const long    t2time = 30000;              // Define timeout time in milliseconds (example: 30000ms = 30s)  
const long    t3time = 90000;              // Define timeout time in milliseconds (example: 90000ms = 90s)
unsigned long conta_scollegamenti = 0;     // contatore degli scollegamenti di blynk

int conta_entrata     =0;
int conta_uscita      =0;
int conta_allarme     =0;
int flip_loop         =0;
int max_conta_eu      =15;
int max_conta_allarme =15;
int cod_evento        =0;
int quale_wifi        =0;    // 0=nessun wifi  1 = casa laura 2 = netsisto

const char* ntpServer = "pool.ntp.org";   // definisce quale è l' NTP SERVER
const long  gmtOffset_sec = 3600;         // fuso +1
const int   daylightOffset_sec = 3600;    // usa ora solare

/* definizioni per un tastierino numerico 
String symbol[4][4] = {
                 { "7", "8", "9" },
                 { "4", "5", "6" },
                 { "1", "2", "3" },
                 { "C", "0", "OK" }
               };
*/

int X,Y;                  // utilizzo spare
int x,y;                  // utilizzo spare
int oggi;                 // usate dal meteo
int giorno;               // usate dal meteo
int ore;                       // usate dal meteo
String tempchar  = "000000";    // usate dal meteo
int temp = 0;                   // usate dal meteo

bool in_avvio     = true;              // flag di avvio
bool success      = false;             // utilizzata da rfid
bool bounce_tag   = false;             // antibounce per evitare la doppia lettura di un tag rfid
bool flip_micro   = false;             // utilizzata dal microonde
bool flip_panico  = false;             // utilizzata dall'antipanico
bool blink_attivo = true;              // segnala se blink è online
bool connected    = false;             // utilizzata dall' rfid

TS_Point p;

WidgetTerminal terminal(V12);	


// variabili per orario NT
String mesi[12]={"Gennaio", "Febb.", "Marzo", "Aprile", "Maggio", "Giugno", "Luglio", "Agosto", "Sett.", "Ottob.", "Novemb.", "Dicemb."};
char   daysOfTheWeek[7][12]    = {"Domenica", "Lunedi", "Martedi", "Mercoledi", "Giovedi", "Venerdi", "Sabato"};
const long utcOffsetInSeconds = 3600;                                  // questo per il time zone UTC + 1

String giorno_sett             = "123456789012";
String orario_formattato       = "00.00.00";
String nome_mese_utc           = "  ";
int    ggsettasnum             = 0 ;
int    giorno_del_mese_utc     = 0 ;
int    mese_utc                = 0 ;
int    anno_utc                = 0;
int    ore_utc                 = 0 ;
int    minuti_utc              = 0 ;
int    secondi_utc             = 0 ;
bool   ora_legale              = false ;                                        // flag  per ora legale
String dataora_in_chiaro_R1    ="  ";
String dataora_in_chiaro_R2    ="  ";
String avvio                   = " ";                       // stringa per memorizzare i dati di avvio del sistema



// *********************************************  matrice degli stati 

uint8_t STATO   = 0;                     // stato del sistema
uint8_t ARMATO  = 0;                     // sistema armato o disarmato
uint8_t ALLARME = 0;                     // allarme  1 = attivo
uint8_t SIRENA  = 1;                     // sirena attiva
uint8_t PORTONE = 0;                     // porta di ingresso
uint8_t quanti_allarmi = 0;              // contatore del numero di allarmi consecutivi per evitare troppi allarmi
 
String campo             = "campo";                        // variabile per le subroutine
String ultima_card_letta = "";                             // variabile per display del codice rfid
String campocard         = "";                             // ulrimo rfid letto
int statocard            = 0;
int numerocard           = 0;
String desc_evento       = "desc_evento ";
String info_di_avvio1    = " ";                             // messaggio contenente i dati di avvio 1
String info_di_avvio2    = " ";                             // messaggio contenente i dati di avvio 2
String msg_allarme1       = "*";                            // messaggio per indicare che è avvenuto un allarme 1
String msg_allarme2       = "*";                            // messaggio per indicare che è avvenuto un allarme 2
String msg_allarme_fuso   = " ";                            // serve per la segnalazione a blink

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

int eventi [20] [7];      
                    // matrice degli ultimi 20 eventi 
/*             STRUTTURA DELLA MATRICE  log degli EVENTI 
              INDEX  0 = GIORNO DEL MESE
                     1 = ORE
                     2 = MINUTI
                     3 = SECONDI 
                     4 = TIPOLOGIA EVENTO  
              
*/

int tag[12][5];                            // matrice dei tag conosciuti da mettere nella memoria non volatile


Adafruit_PN532 nfc(I2C_SDA, I2C_SCL);



// If using the breakout or shield with I2C, define just the pins connected
// to the IRQ and reset lines.  Use the values below (2, 3) for the shield!
// #define PN532_IRQ   (2)
// #define PN532_RESET (3)  // Not connected by default on the NFC Shield



//Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
//XPT2046_Touchscreen touch(TOUCH_CS);


Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

  WiFiServer server(80);

#define tonobasso   1000                    // toni per il beeper
#define tonoalto    2400                    // toni per il beeper

//**************************************FINE DELLE DEFINIZIONI  


//****************************************************************************************************************
void setup()  {      // ********************************* SETUP **************************************************

    Serial.begin(115200);
    WiFi.disconnect();                        // delete old config      
    Blynk.disconnect();
    delay (1000);

     pinMode(TFT_LED, OUTPUT);        //    define as output for backlight control
    pinMode(21, OUTPUT);             //    sound configuration    // settaggi proprietà pwm
    ledcAttach(21, 5000, 2);
   
    pinMode(34, INPUT);     // microonde
    pinMode(35, INPUT);     // portone
    pinMode(39, INPUT);     // panico

    pinMode(25, OUTPUT);    // relay allarme
    digitalWrite(25, LOW);  // Spegni il relay

// *********************************** INIZIALIZZAZIONE WIRE  
    Wire.begin(I2C_SDA, I2C_SCL);    // obbligatoria per sovrapporre i valori di default SCL 21 e SDA 22

// *********************************  INIZIALIZZAZIONE DEL TFT
//    Serial.println("Init TFT and Touch...");
    
    tft.begin();
    touch.begin();

    // Serial.print("tftx ="); Serial.print(tft.width()); Serial.print(" tfty ="); Serial.println(tft.height());

    tft.setRotation(2);
    tft.setTextSize(2);
    tft.fillScreen(BLACK);
    tft.setTextColor(WHITE);
    digitalWrite(TFT_LED, LOW);              // LOW to turn backlight on; 
 
    tft.setCursor (0,0);
    tft.println("  ANTIFURTO LAURA");
   
    tft.print     ("++++");
    tft.print     (__FILE__);
    tft.println   ("++++");
  
    delay(500);  

    //  *********************************************   INIZIALIZZAZIONE CONFIGURAZIONE WIFI
    

    uint8_t contawifi   = 0; 
  
    tft.println("connessione wifi 1: ");  tft.println(ssid1 );  
    quale_wifi=1 ;                                            //  provo il wifi 1
    WiFi.begin(ssid1, pass1);                  
           
      while (WiFi.status() != WL_CONNECTED) {  
          delay(500);  
          tft.print(".");  
          contawifi ++;
          if (contawifi >=25) { 
               tft.println("NO wifi 1"); 
               quale_wifi = 0;  
               break;             // esco con errore
               }
       }

  if (quale_wifi==0) {   // provo il secondo wifi 
          quale_wifi  = 2;
          contawifi   = 0;
          tft.println("connessione wifi 2: ");  tft.println(ssid2 );  
          WiFi.begin(ssid2, pass2);

           while (WiFi.status() != WL_CONNECTED) {  
           delay(500);  
           tft.print(".");  
           contawifi ++;
           if (contawifi >=25) { 
               tft.println("NO wifi 2"); 
               quale_wifi=0;  
               break;             // esco con errore
               }
           }       
       } 

  if (quale_wifi ==0)   {
                  tft.println("NESSUN WIFI TROVATO.. RESTART");  
                  delay(3000);    
                  ESP.restart();             // eseguo il restart
                  }
 
    tft.println("wifi connesso !!");    
    tft.print ("IP: "); tft.println(WiFi.localIP());
  
   //  server.begin();
   //  delay (1000);

  
  //  ***************************  INZIALIZZAZIONE DEL TIME SERVER
  
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);  // Init and get the time
    delay(500);
   
   // *************************** INIZIALIZZAZIONE DEI TAG

  tag[0][0]  = 62;   tag[0][1]  = 241;   tag[0][2]  = 212;   tag[0][3]  = 131;   tag[0][4]  = 1;  // tag n.1  attiva
  tag[1][0]  = 60;   tag[1][1]  = 14;    tag[1][2]  = 72;    tag[1][3]  = 115;   tag[1][4]  = 1;  // tag n.2  attiva
  tag[2][0]  = 94;   tag[2][1]  = 218;   tag[2][2]  = 69;    tag[2][3]  = 115;   tag[2][4]  = 1;  // tag n.3  attiva
  tag[3][0]  = 39;   tag[3][1]  = 47;    tag[3][2]  = 44;    tag[3][3]  = 185;   tag[3][4]  = 1;  // tag n.4  attiva
  tag[4][0]  = 57;   tag[4][1]  = 218;   tag[4][2]  = 69;    tag[4][3]  = 115;   tag[4][4]  = 1;  // tag n.5  attiva
  tag[5][0]  = 54;   tag[5][1]  = 124;   tag[5][2]  = 148;   tag[5][3]  = 121;   tag[5][4]  = 2;  // tag n.6  SETUP
  tag[6][0]  = 189;  tag[6][1]  = 0;     tag[6][2]  = 80;    tag[6][3]  = 115;   tag[6][4]  = 2;  // tag n.7  SETUP
  tag[7][0]  = 12;   tag[7][1]  = 63;    tag[7][2]  = 188;   tag[7][3]  = 169;   tag[7][4]  = 2;    // scheda bianca per setup 
  tag[8][0]  = 131;  tag[8][1]  = 186;   tag[8][2]  = 234;   tag[8][3]  = 53;    tag[8][4]  = 2;    // scheda bianca per setup
  tag[9][0]  = 0;    tag[9][1]  = 0;     tag[9][2]  = 0;     tag[9][3]  = 0;     tag[9][4]  = 0;
  tag[10][0] = 0;    tag[10][1] = 0;     tag[10][2] = 0;     tag[10][3] = 0;     tag[10][4] = 0;
  tag[11][0] = 0;    tag[11][1] = 0;     tag[11][2] = 0;     tag[11][3] = 0;     tag[11][4] = 0;


  // ************************* INIZIALIZZAZIONE DEL LOG EVENTI
  for (x = 0; x <= 19; x++) { for (y = 0; y <= 5; y++) { eventi[x][y] = 0; }  }

   
// ************************** INIZIALIZZAZIONE BLYNK

 
   tft.println("Blynk.begin"); 

   if (quale_wifi ==1)   Blynk.begin(BLYNK_AUTH_TOKEN, ssid1, pass1 );   
   if (quale_wifi ==2)   Blynk.begin(BLYNK_AUTH_TOKEN, ssid2, pass2 );   
   
 
  delay(100);      
   uint8_t contablk   = 0; 
  
    while ( ! Blynk.connected() ) {   
        delay(100);  
        tft.print(".");  
        contablk ++;
        if (contablk >=20) { 
               tft.println("RESTART BLK !!"); 
               delay(3000);    
               ESP.restart();             // eseguo il restart
               }
       }

   tft.println("blynk ok !!! "); 
   delay(500);                             
  

// ******************  INIZIALIZZAZIONE RFID ****************************
// **********************************************************************

  tft.println("avvio NFC "); 
  
  nfc.begin();
  
  delay(500);

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
   tft.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  tft.print("Found chip PN5"); tft.println((versiondata>>24) & 0xFF, HEX);
  tft.print("Firmware ver. "); tft.print((versiondata>>16) & 0xFF, DEC);
  tft.print('.'); tft.println((versiondata>>8) & 0xFF, DEC);

 tft.println("NFC avviato !");

 

  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  // nfc.setPassiveActivationRetries(0xFF);   // attento cosi significa che prova all'infinito 
  // nfc.SAMConfig();                         // configure board to read RFID tags
 

 // ***************************  END CONNESSIONE RFID

 delay (1000);
   
   // timer.setInterval(20000L, blynk_timer);      // Setup a function to be called every second 20 
   // timer.run();                                 // IL TIMER DI BLYNK è DIABILITATO PERCHE NON SO COME RESETTARLO

}      // *************************  end  setup



//**********************************************************************************************************
//**********************************************************************************************************

void loop()   // ******************************* LOOP ******************************************************
{

 Blynk.run();      

//             ************* SOLO ALLO STARTUP 
if (in_avvio) {                                // eseguo solo la prima volta 
    leggi_ora_utp();
    info_di_avvio1  =   dataora_in_chiaro_R1;
    info_di_avvio2  =   dataora_in_chiaro_R2;
    pagina_iniziale();
    scrivi_evento(85);                         //  **************************** SCRITTURA DEL PRIMO LOG 
    pagina_iniziale();                         // è importante scriverlo due volte per allineare il tutto    
    p1time=millis(); 
    p2time=millis();      
    p3time=millis();  
    in_avvio = 0;
  }        

 // ******** ora controllo lo stato di BLYNK segnalando se offline o se torna online
 bool bconn = Blynk.connected();
 
   if (bconn == true ) {                                           // il collegamento è attivo      
         if (blink_attivo == false) {  
            blink_attivo = true;  
            scrivi_evento(52); 
         //   Serial.println ("di nuovo online"); 
            if (STATO==0 || STATO==1) {pagina_iniziale();}          // aggiorno la pagina iniziale
            } 
         }

     if (bconn == false ) {                                         // il collegamento  è giu                           
          if (blink_attivo == true) {  
              blink_attivo = false; 
              scrivi_evento(51); 
             // Serial.println  ("vado offline ");   
              conta_scollegamenti ++;  
              if (STATO==0 || STATO==1) {pagina_iniziale();}          // aggiorno la pagina iniziale
              } 
          WiFi.reconnect();
          bool nulla = Blynk.connect();
         }
           
   
// controllo la scadenze del timer T2 e T3 che servono rispettivamente  a riportare il sistema a pagina vuota oppure a pagina iniziale a seconda dei casi
// mentre il timer T3  serve   a scrivere a blink 
  currenttime = millis(); 

  if (currenttime-p2time >= t2time) {                            // se sono passati 30 secondi senza altre scritture vado ad aggiornare blink  
        scrivi_blink()  ;                                        // attenzione è  scrivi_blink   che si occupa di resettare il timer     
       }
  
  // SE SONO NELLA PAGINA INIZIALE O NELLE PAGINE DI SETUP O DI  LOG o meteo E TRASCORRE TROPPO TEMPO VADO A PAGINA VUOTA 
  // ATTENZIONE !! SOLO IN QUESTE QUATTRO PAGINE PERCHE NEGLI ALTRI CASI LE PAGINE POSSONO VARIARE SOLO IN BASE A PRECISE AZIONI 
  
     if (STATO==1 || STATO==6 || STATO==7 || STATO==8 || STATO==9 ) {          // PAGINA INIZIALE OPPURE PAGINA SETUP OPPURE PAGINA LOG OPPURE PAGINA METEO E FORECAST
   
          if (currenttime-p3time >= t3time)            // in QUESTI STATI   se passa troppo tempo ( 60 SECONDI ) reset a pagina vuota     
             {    
             p3time =  millis();  
              pagina_vuota(); 
             STATO=0;
             }
       }

// ora inizia il controllo per i beep intermittenti da emettere in caso di pagina di entrata od uscita 
// la logica è suonare un loop si ed uno no tramite  apposito bool flip_loop

 if (STATO ==5 ) {  ledcWriteTone(21,tonoalto);  }        // se lo stat è 5 allarme suona sempre con un suono diverso
       else {
            if (flip_loop == 3 )  
                {
                 if (STATO ==  2 ) {  ledcWriteTone(21,tonobasso);  }   // pagina uscita
                 if (STATO ==  3 ) {  ledcWriteTone(21,tonoalto) ;  }   // pagina entrata
                 flip_loop =0;
               }
               else {  ledcWriteTone(21,0); flip_loop ++; }

            }
   

// **********************************************************************************
  // ************ ORA INIZIA LA LETTURA DEI PIN DI INPUT (PORTONE MICROONDE PANICO)
  // IO35      INPUT ONLY  MORSETTO 2  GIALLO PORTONE QUANDO CHIUSO A MASSA
  // IO34      INPUT ONLY  MORSETTO 3 VERDE   MICROONDE SI ATTIVA   A MASSA  
  // IO39      INPUT ONLY  MORSETTO 4 PANIC BUTTON  IL FILO E' A MASSA QUANDO TIRATO AGISCE IL PULLUP E VA A 3,3 
  //
  //            SI DEVE CONTROLLARE UN CAMBIAMENTO DI STATO DEFINITO DA TRE BISTABILI FLIP FLOP 
  //              FLIP_MICRO FLIP_PANICO INIZIALIZZATI A ZERO 
   
  x=0;
  x=analogRead(34);

if (x >= 1000) {                          //  attenzione se il microonde si alza 
    
    if (STATO==0 )        {  pagina_iniziale();  }                        // SE BUIA PASSO A PAGINA INIZIALE  
    if (STATO==1 )        {  tft.fillRect(220, 40, 20, 20, RED);  }  
    if (flip_micro  == 0)                                                 // è la prima volta che si accende 
        { 
        flip_micro = 1 ; 
        if (STATO==4 )                                                    // se il sistema è armato  ALLARME     
           { 
            conta_allarme=max_conta_allarme;   
            quanti_allarmi ++;                                            // incremento di 1 il contatore degli allarmi          
            pagina_allarme();  
            scrivi_evento(62);                                            // ALLARME microonde
          }                                           
        }   
   } 

    else {                                                                // se si spegne
               if (STATO==1 )  {  tft.fillRect(220, 40, 20, 20, GREEN);  }                       
                flip_micro = 0 ;        
           }

//  **************************  ora inizia la lettura del portone  
  y=0;
  y=analogRead(35);
  
   if (y >= 1000) {            // se la porta e aperta
        
        if (PORTONE==0)  {    //                         Se   prima era chiusa 
         // Serial.println ("porta aperta") ;
          PORTONE = 1 ;
          if (STATO==0 || STATO == 1 )   {  pagina_iniziale();  }            // SE BUIA PASSO A PAGINA INIZIALE
                if (STATO==4 )   {                                            //      se il sistema è armato                              
                    STATO=3;                                                  //      passo allo stato contatore in entrata
                    conta_entrata=max_conta_eu;                   
                    pagina_entrata();  
                    scrivi_evento(86);                                        //     porta aperta con sistema in allarme !!!!  ********* FORSE NON SERVE CONTROLLA !!!!
                    }
            }
     }


     else   {                                     // se la porta è chiusa

           if (PORTONE==1)  {         // ma prima era aperta  
             //  Serial.println ("porta chiusa") ;
               PORTONE = 0;
              if (STATO==0 || STATO == 1 )   {  pagina_iniziale();  }
            }

        }

 // ADESSO INIZIA UNA SERIE DI CONTROLLI CHE SCATTANO OGNI 5 SECONDI ( SCANDITI DA PITIME ) CHE SERVONO PER DECREMENTARE I CONTATORI DELLE 
 // PAGINE TEMPORIZZATE OVVERO I CONTATORI IN ENTRATA ED USCITA E L'ALLARME 
 
if (currenttime-p1time >= t1time)                  // SONO PASSATI 5 SECONDI 
    { 
    p1time =  millis();    

     if (STATO ==1) {   aggiorna_ora();   }     //   siamo nella pagina iniziale e allora provvedo   ad aggiornare la data 

     if (STATO == 2 ) {    //   CONTATORE IN USCITA DECREMENTO IL RISPETTIVO CONTATORE E CONTROLLO SE FINITO VAI IN SISTEMA ARMATO 
         conta_uscita --;
            if  (conta_uscita <=0 ) {                
                 STATO=4;
                 pagina_armato();
               //   scrivi_blink();  
              }
       else pagina_uscita();          // di nuovo display della pagina di uscita
      }

      if (STATO == 3 ) {    //   CONTATORE IN ENTRATA DECREMENTO IL RISPETTIVO CONTATORE E CONTROLLO SE FINITO 
       conta_entrata --;
            if  (conta_entrata <=0 ) {                
                 STATO=5;
                 conta_allarme=max_conta_allarme;  
                 quanti_allarmi ++;                                            // incremento di 1 il contatore degli allarmi                       
                 pagina_allarme();
                 scrivi_evento(61);                                           // ALLARME PORTA 
             }
        else pagina_entrata();
      }


      if (STATO ==5)  {    //   ALLARME IN CORSO   DECREMENTO IL RISPETTIVO CONTATORE E CONTROLLO SE FINITO 
      conta_allarme --;
            if  (conta_allarme <=0 ) { 
                  pagina_armato();   
                  scrivi_blink();            // questo serve per aggiornare blynk
                  }
              else                    pagina_allarme();                 // di nuovo il display della pagina allarme
        }

      }         // FINE DELL'ESAME OGNI 5 SECONDI 


  //************************************************************************************
  //************** ora inizia l'analisi della lettura del RFID  *************************


  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };     // Buffer to store the UID
  uint8_t uidLength;                           // UID size (4 or 7 bytes depending on card type)

/*
  connected = connect();

  if (!connected) {                           // significa che non si è connesso al setup
       tft.println("malfunzione NFC");  
       delay (5000);                         // aspetto 5 secondi poi reset
       ESP.restart();             // eseguo il restart   
    }

  if (connected) {

 
*/
 //  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength,500);
   success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength,500);
   // l'ultimo parametro è il timeout se non impostato o a zero significa che sta in attesa fino alla lettura per sempre
  delay (100);
   

  if (success == true && bounce_tag == false   ) {   //    ++++++++++++++++++  the card is detected, print the UID
   
    ultima_card_letta = "Card: ";
       for (uint8_t i = 0; i < uidLength; i++) {
           ultima_card_letta += uid[i]& 0xff;
           ultima_card_letta += "-";
         }
 
  //  Serial.print  (ultima_card_letta);  
  //  Serial.print  (" lenght : "); 
  //  Serial.print  (uidLength); 
   
    //  ***********************  inizia il controllo della card 
    numerocard = 0;
    statocard = 0;
    for (x = 0; x <= 11; x++) {
     
       if (tag[x][0] == uid[0]) {
        if (tag[x][1] == uid[1]) {
         if (tag[x][2] == uid[2]) {
          if (tag[x][3] == uid[3]) {
            numerocard = x + 1;
            statocard = tag[x][4];
            break;
            }
         }
       }
     }
  }                  // FINE DEL FOR 
  
 Serial.print (" carta n.") ; Serial.print (numerocard); Serial.print (" stato :"); Serial.println (statocard);
  

// ***********************  qui deve iniziare l'analisi dovuta alla lettura di un rfid 
// statocard = 0 --> lettura di carta errata
// statocard = 1 --> carta abilitata esclusivamente all accensione / spegnimento del sistema
// statocard = 2 --> carta VERDE abilitata alla gestione del sistema 
// statocard = 3 --> carta ROSSA per forzare il reset del sistema  


 if (statocard == 0 ) {                       // card o tag invalido     

    display_messaggio("    TAG INVALIDO");
    ledcWriteTone(21,tonoalto);  delay(150);  ledcWriteTone(21,0);   // se card errata suono un altro beep diverso
    delay(100);     
    ledcWriteTone(21,tonobasso);  delay(150);  ledcWriteTone(21,0);   
    delay(100);  
    ledcWriteTone(21,tonoalto);  delay(150);  ledcWriteTone(21,0);   
 }

 if (statocard == 1  || statocard == 2 ) { 
      ledcWriteTone(21,tonoalto);  delay(50);  ledcWriteTone(21,0);    // per prima cosa suono  un beep 

    switch (STATO) {
    case 0:                //    PAGINA VUOTA
       p3time =  millis();                // armo il contatore timout di tastiera 
       pagina_iniziale();
       break;

    case 1:               //PAGINA INIZIALE sistema disarmato  in questo caso inizia in conteggio per armare   
       if(statocard==1) { 
          STATO=2; 
          conta_uscita=max_conta_eu;         
          pagina_uscita();
          scrivi_evento (20+numerocard);
          break;
          }

       if(statocard==2) {    
          STATO=6;                   // pagina setup
          p3time =  millis();        // armo il contatore timout di tastiera           
          pagina_setup();
        //  scrivi_evento(40+numerocard);    // disabilito log per impostazioni
          break; 
         }
   

   case 2:              //PAGINA CONTATORE USCITA  in questo caso annullo l'uscita 
           
      STATO=1; 
      p3time =  millis();
      pagina_iniziale();    
      scrivi_evento(30+numerocard); 
      break;
   
  case 3:              //PAGINA CONTATORE  ENTRATA     
      STATO=1; 
      
      p3time =  millis();
      pagina_iniziale();
      scrivi_evento(30+numerocard);
      break;

  case 4:             //PAGINA SISTEMA ARMATO   
      STATO=1; 
      
      p3time =  millis();
      pagina_iniziale();
      scrivi_evento(30+numerocard);
      break;

  case 5:             //PAGINA  ALLARME IN CORSO       
      STATO=1; 
     
      p3time =  millis();
      pagina_iniziale();
      scrivi_evento(70+numerocard);
      break;

  case 6:             //PAGINA SETUP      
      STATO=1; 
      p3time =  millis();
      pagina_iniziale();
      break;

  case 7:             //PAGINA LOG     
      STATO=1; 
      p3time =  millis();
      pagina_iniziale();
      break;

  case 8:             //PAGINA FORECAST PREVISIONI  METEO    
      STATO=1; 
      p3time =  millis();
      pagina_iniziale();
      break;
      
  case 9:             //PAGINA METEO    
      STATO=1; 
      p3time =  millis();
      pagina_iniziale();
      break;
      
   }  // fine switch
}     // fine statocard =1 oppure = 2

if (statocard == 3) {                       // speciale per il reset del sistema 

// *************************** PER ORA NULLA ****************************

   }
//  sistema di controllo antibounce serve per evitare la doppia lettura di un tagTouch_Event

bounce_tag = true;

     delay (500);                      // antibounce sul lettore rfid questo metodo va sostituito con un piu sofisticato metodo di antibounce 
     delay (500);                       // ad esempio con l'accenzione di un flag di blocco lettura disabilitato da un else dopo il  if success 
     delay (500);                      // metto due secondi per una card letta

}                                    // ***************  end routine success  ovvero letta una carta 
   
   else bounce_tag = false;          // else della routine success

  

  // ************************************************   PRESSIONE SUL TOUCH SCREEN
  // ***************************************************************************************
  if (Touch_Event()== true) {  
    
     ledcWriteTone(21,tonoalto);  delay(50);  ledcWriteTone(21,0);  // PER PRIMA COSA BEEP POI ESAMINO LO STATO
     p3time = millis();                                           // poi resetto il  terzo timer di timeout attività

    switch (STATO) {
    case 0:  pagina_iniziale();   break;              
    case 1:  pagina_meteo();      break;      
    case 7:  pagina_iniziale();   break;
    case 8:  pagina_iniziale();   break;   
    case 9:  pagina_previsioni(); break;   
     } 
      
    if (STATO==6)  {                                               // stato=6 pagina setup impostazioni

       int cosafare =0;

       if (Y>= 265 ) {  cosafare=3; }                                                              // controllo sw premuto per vedere log        
          else if (Y  <= 35 ) {  cosafare=1; }                                                     // zona vuota 
            else if (Y  <= 90 && X >= 140  ) {                                                     // tasto sirena
                      if (SIRENA == 1 ) {SIRENA =0;}  else { SIRENA =1;  }
                                cosafare=2; 
                               }  
              else if (Y  <= 145 && X >= 140  ) {                                                  // tasto ora legale
                                    if (ora_legale ==1) {ora_legale=0; }   else { ora_legale=1;  }
                                    cosafare=2; 
                                  } 
               else if (Y  <= 200 && X >= 190 ) {                                                              //  tasto e/u --    
                                    max_conta_eu --;
                                    cosafare=2; 
                                    }
                  else if (Y  <= 200 && X >= 140 ) {                                               //  tasto e/u ++--    
                                    max_conta_eu ++;
                                    cosafare=2; 
                                    }
                    else if (Y  <= 250 && X >= 190 ) {                                            // tasto durata all --
                                    max_conta_allarme --;                                   
                                    cosafare=2; 
                                    }
                       else if (Y  <= 250 && X >= 140 ) {                                        // tasto durata all --
                                    max_conta_allarme ++;                                   
                                    cosafare=2; 
                                    }
                          else  {  cosafare=1;                                                    // residuo
                                    }

        switch (cosafare) {
             case 1:  pagina_iniziale();  break;
             case 2:  pagina_setup();     break;
             case 3:  pagina_eventi();    break;
             }
     }     // fine analisi pagina 6 setup 
      
     
   
  }                  // **********  fine analisi touch event



}  ///////////////  fine loop /////////////////////
///////////////////////////////////////////////////




/******************************************************************** 
 * @brief     detects a touch event and converts touch data 
 * @param[in] None
 * @return    boolean (true = touch pressed, false = touch unpressed) 
 *********************************************************************/
bool Touch_Event() {
  p = touch.getPoint(); 
  delay(10);
   
  if (p.z < MINPRESSURE) return false;  
 // Serial.print (p.x); Serial.print (" ");  Serial.print (p.y); 

  
  // questo vale per vista schermo con connettori del tft in alto

  p.x = map(p.x, 300, 3900, 1, 320);   
  p.y = map(p.y, 300, 3800, 240, 1);  
  Y = p.x; X = p.y;                                       // inverto x con y 
 

 // Serial.print (" <--> "); Serial.print (X); Serial.print (" ");  Serial.println (Y); 
  delay (500);  // aspetta mezzo secondo per antibounce
   return true;  
  
}


// *********************************  gestione dell'orologio real time ***************************
void leggi_ora_utp(){

  
  timeClient.update();

  time_t epochTime  = timeClient.getEpochTime();
  giorno_sett       = (daysOfTheWeek[timeClient.getDay()]);
  ggsettasnum       = (timeClient.getDay());
  ore_utc           = (timeClient.getHours()); 
  minuti_utc        = (timeClient.getMinutes());
  secondi_utc       = (timeClient.getSeconds());
  orario_formattato = (timeClient.getFormattedTime());

  struct tm *ptm = gmtime ((time_t *)&epochTime); 
  giorno_del_mese_utc  = ptm->tm_mday;
  mese_utc             = ptm->tm_mon+1;
  anno_utc             = 1900 + ptm->tm_year;
  nome_mese_utc = mesi[mese_utc-1];
 
 if (ora_legale ==true)  ore_utc++;                             //  Per ora legale


// *****************  preparo la stringa dataora_in_chiaro
 
dataora_in_chiaro_R1 = (giorno_sett); 
dataora_in_chiaro_R1 += (" "); 
dataora_in_chiaro_R1 += (giorno_del_mese_utc); 
dataora_in_chiaro_R1 += (" "); 
dataora_in_chiaro_R1 += (nome_mese_utc); 
dataora_in_chiaro_R2 = (" ore "); 
if (ore_utc <10) {   dataora_in_chiaro_R2 += ("0");}        
dataora_in_chiaro_R2 += (ore_utc);                          
dataora_in_chiaro_R2 += (":");                              
if (minuti_utc <10) {  dataora_in_chiaro_R2 += ("0"); }      
dataora_in_chiaro_R2+= (minuti_utc);  
dataora_in_chiaro_R2 += (":");                              
if (secondi_utc <10) {  dataora_in_chiaro_R2+= ("0"); }      
dataora_in_chiaro_R2 += (secondi_utc);  
}

/********************************************************************//**
 * @brief     disegna un bottone circondato in rosso e del colore voluto 

 * @param[in] xpos   posizione sull'asse x del vertice in alto a sx del bottone
 * @param[in] ypos   posizione sull'asse y del vertice in alto a sx del bottone
 * @param[in] xlarg   larghezza del bottone
 * @param[in] xalt    altezza   del bottone
 * @param[in] fillcolor   colore del bottone
 * @param[in] text   testo del borrone
 * @param[in] textcolor   colore del testo
 * @return    None

 tft.drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
Effect: Draw a square outline on the TFT screen

Parameters:
x：x coordinate of the starting point
y：y coordinate of the starting point
w：the length of the square
h：the width of the square
color：the color of the square
 *********************************************************************/
void bottone (int xpos, int ypos, int xlarg, int xalt, int color, char text [10], int textcolor) 
{
tft.drawRect(xpos, ypos, xlarg, xalt, RED);
tft.fillRect(xpos+1, ypos+1, xlarg-2, xalt-2, color);
int txpos = xpos + 15;
int typos = ypos + (xalt/3);
tft.setCursor(txpos , typos);
tft.setTextColor(textcolor);
tft.setTextSize(2);
tft.println(text);
}


 //*******************************************************************
 //            esegue il display della pagina iniziale 
 //     parametri nessuno 
 //     return    nessuno
 // *********************************************************************/
 void  pagina_iniziale()  {
     

    STATO=1; ARMATO=0; ALLARME=0; digitalWrite(25, LOW);
    
     p3time =   millis();   // RESET CONTATORE PAGINA
     p1time=    millis();   // reset contatore data/ora             
    
         digitalWrite(TFT_LED, LOW);    // LOW to turn backlight on; 
        
         tft.fillScreen(BLACK);
         aggiorna_ora();
         tft.setTextSize(2);       
         tft.setCursor (30,45);
         tft.setTextColor(YELLOW);
         tft.println(" CIAO LAURA");
           
         int spazio =75; 
         int passo  =24; 
         tft.setCursor (0,spazio);    
         tft.setTextColor(WHITE); 
         tft.print (" SISTEMA  ");
         if(ARMATO==1)  {  tft.setTextColor(RED);     tft.println("ARMATO");}
           else         {  tft.setTextColor(GREEN);   tft.println("PRONTO");}
          
        spazio=spazio+ passo;  tft.setCursor (0,spazio);    
        tft.setTextColor(WHITE);   
        tft.print  (" PORTA    ");   
        if(PORTONE==1)  {  tft.setTextColor(RED);     tft.println("APERTA");}
           else         {  tft.setTextColor(GREEN);   tft.println("CHIUSA");}
           
        spazio=spazio+ passo;  tft.setCursor (0,spazio);    
        tft.setTextColor(WHITE);   
        tft.print (" SIRENA   ");
         if(SIRENA==1)  {  tft.setTextColor(GREEN);   tft.println("SI");}
           else         {  tft.setTextColor(RED);     tft.println("NO");}
    
        spazio=spazio+ passo;  tft.setCursor (0,spazio);    
        tft.setTextColor(WHITE);   
        tft.print  (" Ora leg. "); 
        tft.setTextColor(GREEN); 
        if (ora_legale ==1) { tft.println  ("SI"); }             
             else           { tft.println  ("NO"); }
     
       spazio=spazio+ passo;  tft.setCursor (0,spazio);    
       tft.setTextColor(WHITE); 
       tft.print   (" BLK att. ");   
       if (blink_attivo) {  tft.setTextColor(GREEN);   tft.print ("SI ");}
          else           {  tft.setTextColor(RED);     tft.print ("NO ");}
       tft.print   ("sc:"); tft.println (conta_scollegamenti);
      
      
//**************************  ora stampo la segnalazione di avvenuto allarme che resta in piedi finchè non faccio un operazione di setup
 if (msg_allarme1 != "*" ) { 

    tft.fillRect(0, 180, 240, 150,RED);   
    tft.setTextColor(WHITE);
    tft.setCursor (0,200);
    tft.setTextSize(2);  
    tft.println ("ALLARME !!");
    tft.println (msg_allarme1);
    tft.println (msg_allarme2);
    tft.print   ("ALL. consecutivi ");tft.println(quanti_allarmi);
    tft.println("");
    tft.println ("ESAMINA IL LOG !!");}
 
    else {                                    // se nessun allarme stampo invece questo 

        tft.setTextSize(2);  
        tft.setTextColor(WHITE);
        tft.println(""); 
        tft.println ("Ultima operazione"); 
       campo = "";
       analisi_evento(0);                // analisi codice evento
       tft.setTextColor(GREEN);
       tft.println(campo);
       tft.println(desc_evento);
       tft.setTextSize(1); 
       tft.println("");
       tft.setTextSize(2); 
       tft.setTextColor(WHITE);
       tft.println("Sistema online da:");
       tft.setTextColor(GREEN);
       tft.println(info_di_avvio1);
       //tft.println(info_di_avvio2);
       tft.setTextSize(1); 
       tft.println("");
       tft.setTextSize(2); 
       tft.setTextColor(MAGENTA);
       tft.println ("  PREMI PER METEO");

        }
  

 }  // fine pagina iniziale

//*******************************************************************
 //            esegue il display della pagina vuota
 //     parametri nessuno 
 //     return    nessuno
 // *********************************************************************/
 void  pagina_vuota() 
 {
           
          STATO=0; ARMATO=0; ALLARME=0; digitalWrite(25, LOW);
         digitalWrite(TFT_LED, HIGH);                                     // LOW to turn backlight on; 
         tft.fillScreen(BLACK);
 }


 /*  
 ******************************************************************* 
 * @brief     mostra un messaggio nel'area messaggi
 * @param[in] None
 * @return    None
 ********************************************************************
 */
void display_messaggio(String Messaggio)
  {

   p3time =  millis(); 
    digitalWrite(TFT_LED, LOW);                     // LOW to turn backlight on;  
    tft.fillRect     (0, 0, 230, 35,YELLOW);        //clear result box
    tft.setCursor    (0, 2);
    tft.setTextSize  (2);
    tft.setTextColor (BLUE);
    tft.println      (Messaggio);
    
  }   


    BLYNK_WRITE(V4)    //  *******************  bottone inserisci antifurto **************************** 
{
  int pin4 = param.asInt(); // assigning incoming value from pin V4 to a variable
  // You can also use:
  // String i = param.asStr();
  // double d = param.asDouble();
    Serial.print(" BOTTONE BLYNK V4 value is: ");   Serial.println(pin4);

  
 // if (pin4 ==1)  ARMATO=1;      else   ARMATO=0;
 // if (pin4 ==1)  STATO=4;      else   ARMATO=0;
 
 
  ledcWriteTone(21,tonoalto);  delay(80);  ledcWriteTone(21,0);                   // UN BEEP  
  if (pin4 ==1)  {  pagina_armato();   scrivi_evento(69); }    
       else      {  pagina_iniziale(); scrivi_evento(68); }
 
  delay (100);                                 // per anti bounce

}

BLYNK_WRITE(V5)    //  *******************  bottone attiva immediatamente  allarme **************************** 
{
  int pin5 = param.asInt(); // assigning incoming value from pin v5 to a variable
  // You can also use:
  // String i = param.asStr();
  // double d = param.asDouble();

  Serial.print("BOTTONE BLYNK V5 value is: ");   Serial.println(pin5);
 ledcWriteTone( 0,tonobasso);  delay(80);  ledcWriteTone(21,0);                   // UN BEEP  

    
  if (pin5 ==1)  {
            
            conta_allarme=max_conta_allarme;   
            quanti_allarmi ++;                            // incremento di 1 il contatore degli allarmi       
            pagina_allarme() ; 
            scrivi_evento (63);                           // ALLARME AVVIATO BLK
                 }     
                 
                 else  {                      
                     pagina_iniziale() ;
                     scrivi_evento (65);
                     } 

  delay (100);                                 // per anti bounce
}

BLYNK_WRITE(V12)    //  *******************  bottone del teminale **************************** 
{ 
char pin12 = param.asInt();                       // assigning incoming value from pin V1 to a variable
   Serial.println ("bottone v12 terminale");
scrivi_blink()  ;
}



//**********************************************************************************************              
void scrivi_blink()  {                   //  routine innescata dal timer di BLYNK     

 Blynk.virtualWrite(V0,ALLARME);  
 Blynk.virtualWrite(V1,ARMATO);  
 Blynk.virtualWrite(V5,ALLARME);  
 Blynk.virtualWrite(V4,ARMATO);         

terminal.clear();                     // invio gli ultimi dieci eventi
 
         for (x=0; x <=9;x++) {            
              
             terminal.print("#"); 
             if (x<=8) terminal.print("0");
             terminal.print(x+1);terminal.print(" ");
             analisi_evento (x);
             terminal.print (campo);    terminal.println (desc_evento);    
             }
terminal.println  ("---"); 
 
terminal.print  ("ONLINE DA: "); terminal.print (info_di_avvio1);    terminal.println (info_di_avvio2);  

if  (SIRENA ==1)      { terminal.print    (" Sirena ON ") ; }
       else           { terminal.print    (" Sirena OFF "); }
if (ora_legale ==1)   { terminal.print    (" - Ora legale SI "); }             
       else           { terminal.print    (" - Ora legale NO "); }
if (blink_attivo ==1) { terminal.print    (" - Blk SI sc:"); }             
       else           { terminal.print    (" - Blk NO sc:"); }

terminal.println (conta_scollegamenti); 

analisi_evento ( 0 );                   // analisi del log 0 
campo = desc_evento;                    // se il ciclo è il numero 0 significa che è relativo all'ultimo messaggio   
Blynk.run(); 
Blynk.virtualWrite(V11 ,campo);         // V11 è la stringa dello stato attuale del sistema
     
p2time =  millis();                   // ATTENZIONE RESETTO IL TIMER *** IMPORTANTE ******************
}

// ***********************************************************************************
// ******************  BLYNK_CONNECTED  funzione per connettere BLYNK ***************
// If your hardware loses an Internet connection or resets, 
// you can restore all the values from Datastreams in the Blynk app.
// **********************************************************************************

BLYNK_CONNECTED() {
  
Blynk.syncAll();
//  Blynk.virtualWrite(V11 ,"INIZIALIZZAZIONE !!");  
terminal.clear();
terminal.println ("blynk CONNESSO");
terminal.flush(); 
delay(500);
Blynk.run();
scrivi_blink()  ;
}


 
// ************************************************  scrittura del log 

void scrivi_evento(int cevento) { 

  //  ****************** SCRITTURA DEL LOG *******************
  for (x = 0; x <= 18; x++) {  // rotazione del log
    for (y = 0; y <= 5; y++) { eventi[19 - x][y] = eventi[18 - x][y]; }
  }

  leggi_ora_utp() ;
  delay(100);
  eventi[0][0] = giorno_del_mese_utc ;
  eventi[0][1] = ore_utc ;
  eventi[0][2] = minuti_utc;
  eventi[0][3] = secondi_utc;
  eventi[0][4] = cevento;
   Serial.print ("scrivo evento "); Serial.println(cevento);
  scrivi_blink();           // ogni volta che scrivo il log lo segnalo a blynk

  // **********************************************************  se evento è allarme  61 62 63 64 preparo il messaggio allarme
       if    ( cevento >60 && cevento < 65) {          
        analisi_evento(0);
        msg_allarme1     =  campo ; 
        msg_allarme2     =  desc_evento; 
        msg_allarme_fuso =  "ALLARME ";  msg_allarme_fuso += campo ; msg_allarme_fuso += desc_evento ;

  // *********************************************************************************
  // PER ORA   DISABILITO IL LOG EVENTI CHE FA SCATTARE ALLARME SU BLYNK
  // *********************************************************************************

          //   Blynk.logEvent      ("A1", msg_allarme_fuso);

        }


  }      

// ****************************************************  void  analisi_evento
void analisi_evento(int c) { 
 

  /*
                         TIPOLOGIA EVENTO   0= ELEMENTO VUOTO      
                                           10 = RESTART DEL SISTEMA     15 = sirena disabilitata web   16 = sirena abilitata web    
                                           20-29 ALLARME INSERITO DA CARTA    20 - 29 ID CARTA 
                                           30-39 ALLARME DISINSERTO DA CARTA  30 - 39 ID CARTA
                                           40-49 INSERITA CARTA BLOCCATA      40 - 49 ID CARTA
                                           51 = BLYNK OFF 
                                           52 = BLINK ON
                                           60-69 SCATTATO ALLARME 61= ALLARME PORTA 62=ALLARME RADAR 63 =ALLARME DA WEB 64= PANIC BUTTON 
                                           70-79 INTERROTTO ALLARME DA CARTA   70 - 79  ID CARTA   
                                           80 =  INTERROTTO ALLARME DA WEB 
                                           81 =  AVVIO DEL SISTEMA
  */
                  campo = "gg:" ; 
                  campo += eventi [c] [0] ; 
                  campo += " ";
                  if (eventi [c] [1] <= 9 ) campo += "0" ;
                  campo += eventi [c] [1] ;  
                  campo += ":"; 
                  if (eventi [c] [2] <= 9 ) campo += "0" ;      
                  campo += eventi [c] [2] ;  
                  campo += ":";  
                  if (eventi [c] [3] <= 9 ) campo += "0" ;     
                  campo += eventi [c] [3] ; 
                  campo += " e:";   
                  campo += eventi [c] [4] ;
                  campo+=" ";
       cod_evento =eventi [c] [4] ;
  desc_evento = "";
  if (cod_evento == 0) { desc_evento = ""; }
  //                                          "12345678901234567890"
   else if (cod_evento <=19)  {  }                                                                          //  zona vuota non dovrebbe su<ccedere mai 
   else if (cod_evento <= 30) { desc_evento = "Sistema ON  key "; desc_evento += (cod_evento - 20);  }      //  20 + ncard SISTEMA ON DA CARD 
   else if (cod_evento <= 40) { desc_evento = "Sistema OFF key "; desc_evento += (cod_evento - 30);  }      //  30 + ncard SISTEMA OFF DA CARD 
   else if (cod_evento <= 50) { desc_evento = "Impostazioni key "; desc_evento += (cod_evento - 40); }      //  40 + ncard IMPOSTAZONI DA CARD -- NON USATO --
   else if (cod_evento == 51) { desc_evento = "Blynk OFFLINE";}                                                 //  51         BLYNK E' ANDATO OFFLINE
   else if (cod_evento == 52) { desc_evento = "Blynk ONLINE";}                                                  //  52         BLYNK E' ANDATO ONLINE
   else if (cod_evento == 61) { desc_evento = "ALLARME PORTA"; }                                            
   else if (cod_evento == 62) { desc_evento = "ALLARME RADAR";}                                            
   else if (cod_evento == 63) { desc_evento = "ALL. AVVIATO BLK";}                                           
   else if (cod_evento == 64) { desc_evento = "ALLARME PANICO";}                                          // NON  ATTIVATO
   else if (cod_evento == 65) { desc_evento = "ALL STOP DA BLK";} 
   else if (cod_evento == 66) { desc_evento = "Sist ON da WEB";} 
   else if (cod_evento == 67) { desc_evento = "Sist OFF da WEB";} 
   else if (cod_evento == 68) { desc_evento = "Sist OFF da BLK";} 
   else if (cod_evento == 69) { desc_evento = "Sist ON  da BLK";} 
   else if (cod_evento <= 80) { desc_evento = "ALL. STOP key "; desc_evento += (cod_evento - 70);  }        // 70 + ncard ALLARME FERMATO DA CARD
   else if (cod_evento == 81) { desc_evento = "SIR abilitata ";}                                            // NON ATTIVATO
   else if (cod_evento == 82) { desc_evento = "SIR disabilitata";}                                          // NON ATTIVATO
   else if (cod_evento == 85) { desc_evento = "Avvio del Sistema";}    
   else if (cod_evento == 86) { desc_evento = "Porta Aperta !!"; }                                          // NON ATTIVATO
  //                                                              "12345678901234567890"
}  // fine analisi evento

//         ********************************************   display della pagina degli eventi
//         *********************************************************************************
void pagina_eventi() {
 
  STATO=7; ARMATO=0 ; ALLARME=0; digitalWrite(25, LOW);
  p1time = millis();  //            *****************  
  msg_allarme1    = "*";             // azzero il messaggio di allarme 1
  msg_allarme2    = "*";             // azzero il messaggio di allarme 2
  quanti_allarmi = 0;               // azzero anche il contatore degli allarmi
  tft.fillScreen(BLACK);

  tft.setCursor     (0, 2);
  tft.setTextColor  (GREEN);
  tft.setTextSize   (2);
  tft.println       (" LOG DEGLI EVENTI");
  tft.setTextSize   (2);
  tft.println       ("");
  y = 0;
  for (x = 0; x <= 12; x++) {
    if (eventi[x][4] == 0) break;
       analisi_evento(x);    
       tft.setTextColor(WHITE);
       tft.println (campo);
     
     if (cod_evento >= 61 && cod_evento <= 64) tft.setTextColor(RED); else tft.setTextColor(GREEN);
    
     tft.println(desc_evento);

  }
}

//         ********************************************   display della pagina entrata 
//         ***************************************************************************
void pagina_entrata() {
   
  STATO=3; ARMATO=1;
  tft.fillScreen     (BLACK);
  tft.setCursor      (2, 40);
  tft.setTextColor   (GREEN);
  tft.setTextSize    (4);
  tft.println        ("  ENTRATA ");
  tft.println        ("");
  tft.print          ("    ");
  tft.setTextSize    (6);
  tft.println        (conta_entrata);
  }

//         ********************************************   display della pagina uscita
//         ***************************************************************************
void pagina_uscita() {
   

  STATO=2; ARMATO=1 ; ALLARME=0; digitalWrite(25, LOW);
  tft.fillScreen   (BLACK);
  tft.setCursor    (0, 40);
  tft.setTextColor (GREEN);
  tft.setTextSize  (4);
  tft.println      ("  USCITA ");
  tft.println      ("");
  tft.print        ("    ");
  tft.setTextSize  (6);
  tft.println      (conta_uscita);
  }


//         ********************************************   display della pagina allarme
//         ****************************************************************************
void pagina_allarme() {
  
  STATO=5; ARMATO=1; ALLARME=1;
  
  if (quanti_allarmi <=  4 &&  SIRENA ==1)   digitalWrite(25, HIGH); else  digitalWrite(25, LOW);        // accendi il relay allarme                       

  digitalWrite    (TFT_LED, LOW);    // LOW to turn backlight on;  
  tft.fillScreen  (RED);
  tft.setCursor   (0,10);
  tft.setTextColor(YELLOW);
  tft.setTextSize (4);
  tft.print       ("ALLARME !!  n. "); tft.println (quanti_allarmi );
  tft.println     ("");
  tft.println     ("");
  tft.setTextSize (6);
  tft.print       ("   ");
  tft.println     (conta_allarme);
  }

//         ********************************************   display della pagina sistema armato
//         ***********************************************************************************
void pagina_armato() {
  

  STATO=4; ARMATO=1; ALLARME=0;
  digitalWrite      (TFT_LED, LOW);    // LOW to turn backlight on; 
  tft.fillScreen    (BLACK);
  tft.setTextColor  (RED);
  tft.setTextSize   (4);
  tft.setCursor     (40, 40);
  tft.println       ("SISTEMA");
  tft.setCursor     (40, 100);
  tft.println       ("ARMATO");  
  }

//         ********************************************   display della pagina setup
//         **************************************************************************
void pagina_setup() {
  
  STATO=6; ALLARME=0;ARMATO=0; digitalWrite(25, LOW) ;
  tft.fillScreen   (BLACK);
  tft.setCursor    (0, 2);
  tft.setTextColor (GREEN);
  tft.setTextSize  (3);
  tft.println      ("IMPOSTAZIONI  ");
  tft.setTextColor (WHITE);
  tft.setTextSize  (2);
  tft.setCursor    (0, 45);
  tft.println      ("SIRENA ");
   if (SIRENA ==1) { 
      tft.setTextColor(GREEN);      tft.println ("attiva"); } 
     else {  tft.setTextColor(RED); tft.println ("disattiva"); } 
   tft.setTextColor(WHITE); 
  if (SIRENA ==1) { 
           bottone (140,35,100,50,BLUE," OFF",WHITE) ; } 
     else {bottone (140,35,100,50,BLUE,"  ON ",WHITE) ; } 
     
  tft.setCursor    (0, 100);
  tft.println      ("ORA LEGALE");
 
     if (ora_legale ==1) {
                 bottone (140,90,100,50,BLUE,"  NO",WHITE) ;  tft.setTextColor(GREEN);  tft.println ("attiva"); } 
        else    {bottone (140,90,100,50,BLUE,"  SI",WHITE) ;  tft.setTextColor(GREEN);  tft.println ("disattiva"); } 
   tft.setTextColor(WHITE);
  
  tft.setCursor   (0, 150);
  tft.println     ("TEMPO E/U ");
  tft.setTextColor(GREEN);
  tft.print       ("   ");tft.println (max_conta_eu);
  tft.setTextColor(WHITE);

  bottone (140,145,50,50,BLUE,"+",WHITE) ;
  bottone (190,145,50,50,BLUE,"-",WHITE) ;

  tft.setCursor    (0, 205); 
  tft.println      ("DURATA ALL");
  tft.setTextColor (GREEN);
  tft.print        ("   ");tft.println (max_conta_allarme);
  tft.setTextColor (WHITE);

  bottone (140,200,50,50,BLUE,"+",WHITE) ;
  bottone (190,200,50,50,BLUE,"-",WHITE) ;

  tft.drawRoundRect(5, 265, 230, 50, 20, RED);
  tft.fillRoundRect(6, 266, 228, 48, 20, BLUE);
  tft.setCursor(0, 275);
  tft.println ("  VEDI LOG EVENTI");
  
  }


//*************************************************************************************************
void aggiorna_ora()  {    // routine per la scrittura del box data ora
          
   leggi_ora_utp(); 
   tft.fillRect(0, 0, 240, 35, BLUE);    //  di seguito serve solo per il box 
   tft.setTextSize(2);
   tft.setTextColor(WHITE);
   tft.setCursor (0,0);
   tft.println (dataora_in_chiaro_R1);
   tft.println (dataora_in_chiaro_R2);
   }

//*************************************************************************************************
void pagina_meteo()  {    // pagina per il display delle informazioni meteo
  STATO=9; ALLARME=0;ARMATO=0; digitalWrite(25, LOW) ;
  
  String serverPath =  "http://api.openweathermap.org/data/2.5/forecast?lat=41.8947&lon=12.4839&appid="  + openWeatherMapApiKey+ "&units=metric&lang=it" ;   
  jsonBuffer = httpGETRequest(serverPath.c_str());
  //  JSON.typeof(jsonVar)                                        // can be used to get the type of the var
  JSONVar myObject = JSON.parse(jsonBuffer);
  // prelevo la data odierna ovvero dellla ricorrenza 0
  String da     =  myObject["list"][0]["dt_txt"];
  String dt     =  da.substring(8,10);
  String hh     =  da.substring(11,13);
  String descri =  myObject["list"][0]["weather"][0]["description"];

  tempchar =  JSON.stringify(myObject["list"][0]["main"]["temp"]);
  float se= tempchar.toFloat();
  se=se+0.5;
  temp = (int) se;
 // Serial.print  (tempchar);  Serial.print  ("xx");Serial.print  (se);  Serial.print  ("xx"); Serial.println (temp);
  
      tft.fillScreen   (NAVY);  tft.setCursor    (55, 2);
      tft.setTextColor (GREEN);    tft.setTextSize  (3); tft.println  ("METEO ");  
      tft.setTextColor (WHITE); 
      tft.setTextSize  (2); tft.print  ("Previsione delle "); tft.println (hh);
      tft.setTextSize  (3); tft.println ("");       
      tft.setTextSize  (2); tft.setTextColor (WHITE);  tft.print("Temperatura ");  
      tft.setTextSize  (5); tft.setTextColor (YELLOW);  tft.print(temp);
      tft.setTextSize  (2); tft.setTextColor (YELLOW);  tft.println("O");  

      tft.setTextSize  (5); tft.println  ("");
      tft.setTextSize  (3); tft.setTextColor (YELLOW);  tft.println(descri);       //la descrizione dello stato esempio nuvole sparse 
      tft.setTextSize  (1); tft.println  ("");
      tft.setTextSize  (2); tft.setTextColor (WHITE);  tft.print("Pressione  ");
      tft.setTextSize  (3); tft.setTextColor (YELLOW);  tft.println(myObject["list"][0]["main"]["pressure"]);
      tft.setTextSize  (1); tft.println  ("");
      tft.setTextSize  (2);tft.setTextColor (WHITE);  tft.print("Umidita %  ");
      tft.setTextSize  (3); tft.setTextColor (YELLOW);  tft.println(myObject["list"][0]["main"]["humidity"]);
      tft.setTextSize  (1); tft.println  ("");
      tft.setTextSize  (2);tft.setTextColor (WHITE);  tft.print("Vento mt/s ");
      tft.setTextSize  (3); tft.setTextColor (YELLOW);  tft.println(myObject["list"][0]["wind"]["speed"]);
      tft.setTextSize  (2);  tft.println  ("");  
      tft.setTextSize  (2);tft.setTextColor (MAGENTA);  tft.print("PREMI PER PREVISIONI");
 } 


//**************************************************************
  String httpGETRequest(const char* serverName) {
         WiFiClient client;
         HTTPClient http;
                                       
        http.begin(client, serverName);     // Your Domain name with URL path or IP address with path
        int httpResponseCode = http.GET();       // Send HTTP POST request
  
        String payload = "{}"; 
  
       if (httpResponseCode>0) {
      // Serial.print("HTTP Response code: ");
      // Serial.println(httpResponseCode);
       payload = http.getString();
        }
      else {
     // Serial.print("Error code: ");
     // Serial.println(httpResponseCode);
      }
 
     http.end();   // Free resources
     return payload;
   }
 
  

//*************************************************************************************************
void pagina_previsioni()  {    // pagina per il display delle previsioni meteo viene attivata dalla  pagina meteo

  STATO=8; ALLARME=0;ARMATO=0; digitalWrite(25, LOW) ;

  String serverPath =  "http://api.openweathermap.org/data/2.5/forecast?lat=41.8947&lon=12.4839&appid="  + openWeatherMapApiKey+ "&units=metric&lang=it" ;   
  jsonBuffer = httpGETRequest(serverPath.c_str());
  //  JSON.typeof(jsonVar)                                        // can be used to get the type of the var
  JSONVar myObject = JSON.parse(jsonBuffer);
 //  Serial.println (myObject);

  tft.fillScreen   (NAVY);  tft.setCursor    (35, 2);
  tft.setTextColor (GREEN); tft.setTextSize  (3);
  tft.println      ("PREVISIONI");
  tft.setTextSize  (1);  tft.println ("");

   oggi = giorno_del_mese_utc;   
   int righe=0;  
   int domani=0;
   tft.setTextColor (WHITE);    tft.setTextSize  (2); 
 
for (x=0; x<=16; x++) {
                        
  String  dax = (myObject["list"][x]["dt_txt"]);
  String  dix = dax.substring(8,10);
  String  hhx = dax.substring(11,13);
  giorno  = dix.toInt(); 
  ore     = hhx.toInt(); 
  tempchar =  JSON.stringify(myObject["list"][x]["main"]["temp"]);
  float se1= tempchar.toFloat();
  se1=se1+0.5;
  temp = (int) se1;
 
  if ( ore > 5 && ore < 23 && righe < 7)  {                   // evito di stampare le ore notturne 
       tft.setTextColor (WHITE);    tft.setTextSize  (2);        
           if (giorno==oggi) {tft.print       ("Oggi   ore:"); }
             else  {
                   domani++;
                    if (domani < 7) tft.print ("Domani ore:"); 
                    if (domani ==7) tft.print ("Dopo d.ore:");
                    }

       tft.setTextColor (GREEN); tft.print (hhx); tft.setTextColor (WHITE); 
       tft.print (" T= ");
       tft.setTextColor (YELLOW);       
       tft.println  (temp);

       tft.println(myObject["list"][x]["weather"][0]["description"]);
       tft.setTextSize  (1); tft.println("");
       righe ++;
      }

   // Serial.print("giorno" );  Serial.print(dix); Serial.print("alle" );  Serial.print(hhx);
   // Serial.print(" T " ); Serial.print    (myObject["list"][x]["main"]["temp"]);  
   // Serial.print(" U " ); Serial.print    (myObject["list"][x]["main"]["Pressure"]);  
   // Serial.print(" F " ); Serial.println  (myObject["list"][x]["weather"][0]["description"]);


       }   // end for x
   }
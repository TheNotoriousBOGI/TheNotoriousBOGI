// ######################################################################
// ###                                                                ###
// ###   Auswertesoftware fuer HAGER EHZ363... und EHZ163...          ###
// ###                                                                ###
// ###   Firmware-Version: 20210903                                   ###
// ###                                                                ###
// ###                                                                ###
// ######################################################################


//custom headers (include-folder)
#include "obis.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "string.h"
#include <SPIFFS.h>

#include <WiFi.h>
#include <WiFiSettings.h>
#include <HTTPClient.h>
#include <time.h>

//webserver for ota-update
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>;
AsyncWebServer server(80);

//webportal
String Serveraddress = "";
String Apikey = "";
String Authorization = "";
String Bearer = "Bearer ";
bool checkbox_server = true;
bool checkbox_apikey = false;
bool checkbox_authorization = false;

//WiFi info
//const char *ntpServer = "pool.ntp.org";
const char *ntpServer = "ptbtime1.ptb.de";  //changed for compatibility

//timestamp function
unsigned long timestamp;
unsigned long getTimestamp()
{
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return (0);
    }
    time(&now);
    return now;
}



//Variables
char ServerID_string[MSIZE];
char ServerID_string_formatted[MSIZE];
char eHZ_Message[MSIZE] = "";    // Zeichenpuffer der gelesenen Daten vom eHZ
char manufacturer[MSIZE] = "hager";
char printstring[MSIZE];         // String als Zwischenspeicher fuer die Ausgabe auf der Konsole
char Daten_Substring[MSIZE];     // Substring fuer Seriennummer
char eHZ_zeichen[4];             // Zeichen - als HEX gewandelt, ohne Trennzeichen - vom eHZ ueber die optische RS232
int eHZ_data = 0;                // Datenbyte ueber die optische RS232 vom eHZ eingelesen
int Escape_Zaehler = 0;          // Zaehler fuer die ESCAPE Zeichen (0x1b) am Message-Ende
int Escape = 0;                  // Flag, dass ESCAPE-Sequenz kam
long int Zaehlvariable = 0;      // Zaehlvariable fuer eine kontinuierliche Nummerierung der Schleifendurchlaeufe
long int PrintZaehlvariable = 0; // Zaehlvariable fuer die Durchnummerierung der ausgedruckten Zaehlerwerte
int Werte_Empfangen = 0;         // Flag, wird gesetzt, wenn wenigstens 1 Datensatz empfangen wurde ---> Ausgabe
int TL_Flag;                     // T/L Kenner des SML Protokolls
int TL_Offset;                   // Hilfsvariable um die Daten auszuwerten ja nach T/L Kenner

unsigned long int Zaehlernummer = 0; // Ausgelesene Werte aus dem Telegramm (Zaehlernummer)
long int Age_Zaehlernummer = 0;      // "Alter" der Daten
double Wirkleistung = 0;             // Ausgelesene Werte aus dem Telegramm (Wirkleistung)
long int Age_Wirkleistung = 0;       // "Alter" der Daten
double Wirkenergie = 0;              // Ausgelesene Werte aus dem Telegramm (Wirkenergie)
long int Age_Wirkenergie = 0;        // "Alter" der Daten
//NEU
double WirkenergieT1 = 0;
long int Age_WirkenergieT1 = 0;
double WirkenergieT2 = 0;
long int Age_WirkenergieT2 = 0;
double Blindleistung_L1 = 0;
long int Age_BlindleistungL1 = 0;
double Blindleistung_L2 = 0;
long int Age_BlindleistungL2 = 0;
double Blindleistung_L3 = 0;
long int Age_BlindleistungL3 = 0;
double PhasenabweichungStromSpannung_L1 = 0;
long int Age_PhasenabweichungStromSpannung_L1 = 0;
double PhasenabweichungStromSpannung_L2 = 0;
long int Age_PhasenabweichungStromSpannung_L2 = 0;
double PhasenabweichungStromSpannung_L3 = 0;
long int Age_PhasenabweichungStromSpannung_L3 = 0;
double PhasenabweichungSpannungen_L1L2 = 0;
long int Age_PhasenabweichungSpannungen_L1L2 = 0;
double PhasenabweichungSpannungen_L1L3 = 0;
long int Age_PhasenabweichungSpannungen_L1L3 = 0;
//
double Wirkleistung_L1 = 0;       // Ausgelesene Werte aus dem Telegramm (Wirkleistung L1)
long int Age_Wirkleistung_L1 = 0; // "Alter" der Daten
double Wirkleistung_L2 = 0;       // Ausgelesene Werte aus dem Telegramm (Wirkleistung L2)
long int Age_Wirkleistung_L2 = 0; // "Alter" der Daten
double Wirkleistung_L3 = 0;       // Ausgelesene Werte aus dem Telegramm (Wirkleistung L3)
long int Age_Wirkleistung_L3 = 0; // "Alter" der Daten
double Spannung_L1 = 0;           // Ausgelesene Werte aus dem Telegramm (Spannung L1)
long int Age_Spannung_L1 = 0;     // "Alter" der Daten
double Spannung_L2 = 0;           // Ausgelesene Werte aus dem Telegramm (Spannung L2)
long int Age_Spannung_L2 = 0;     // "Alter" der Daten
double Spannung_L3 = 0;           // Ausgelesene Werte aus dem Telegramm (Spannung L3)
long int Age_Spannung_L3 = 0;     // "Alter" der Daten
double Strom_L1 = 0;              // Ausgelesene Werte aus dem Telegramm (Strom L1)
long int Age_Strom_L1 = 0;        // "Alter" der Daten
double Strom_L2 = 0;              // Ausgelesene Werte aus dem Telegramm (Strom L2)
long int Age_Strom_L2 = 0;        // "Alter" der Daten
double Strom_L3 = 0;              // Ausgelesene Werte aus dem Telegramm (Strom L3)
long int Age_Strom_L3 = 0;        // "Alter" der Daten
double Spannung_Max = 0;          // Ausgelesene Werte aus dem Telegramm (Spannung MAX)
long int Age_Spannung_Max = 0;    // "Alter" der Daten
double Spannung_Min = 0;          // Ausgelesene Werte aus dem Telegramm (Spannung MIN)
long int Age_Spannung_Min = 0;    // "Alter" der Daten
double Chiptemp = 0;              // Ausgelesene Werte aus dem Telegramm (Spannung MIN)
long int Age_Chiptemp = 0;        // "Alter" der Daten
double Chiptemp_Min = 0;
long int Age_Chiptemp_Min = 0;
double Chiptemp_Max = 0;
long int Age_Chiptemp_Max = 0;
double Chiptemp_Avg = 0;
long int Age_Chiptemp_Avg = 0;

int i = 0;           // Zaehlvariable fuer Schleifen
long int Offset = 0; // Offset des Suchstrings
char *B_Daten;       // Pointer auf Datenbeginn des Auswerteteils
char *Beginn;        // Pointer auf Beginn der eHZ-Sendedaten


int connection_counter = 0;

//#########################################################################################################################################

//########################################################################################################################################
//define pins for serial input and output
#define RXD2 16
#define TXD2 17

//SETUP(START)############################################################################################################################
void setup()
{
    SPIFFS.begin(true);
    //open serial connection (console output)
    Serial.begin(115200);

    //Open optical serial connection
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

    //Print optical interface info
    Serial.println("Serielle Schnittstelle 2 (eHZ):");
    Serial.println("Serial Txd is on pin: " + String(TXD2));
    Serial.println("Serial Rxd is on pin: " + String(RXD2));
    Serial.println(" ");

    WiFiSettings.heading("Device SUITE_001");

    // Define custom settings saved by WifiSettings
    // These will return the default if nothing was set before
    String Server_dummy = WiFiSettings.string("Server address", "default.example.org:443");
    bool checkbox_server_dummy = WiFiSettings.checkbox("Server", true);
    String Apikey_dummy = WiFiSettings.string("API key", "dm38qd0ajd3803e0jd320asfafveafeafafa");
    bool checkbox_apikey_dummy = WiFiSettings.checkbox("APIkey");
    String Authorization_dummy = WiFiSettings.string("Authorization Bearer", "Ndufqa9d3qh8d320fh4379fh438904hf8430f9h4herf947fgPUEFF");
    bool checkbox_authorization_dummy = WiFiSettings.checkbox("Bearer");


    connection_counter = 0;
    // Connect to WiFi with a timeout of 30 seconds
    // Launches the portal if the connection failed
    WiFiSettings.connect(true, 30);
    
    Serveraddress = Server_dummy;
    Apikey = Apikey_dummy;
    Authorization = Authorization_dummy;
    Bearer += Authorization;
    checkbox_server = checkbox_server_dummy;
    checkbox_apikey = checkbox_apikey_dummy;
    checkbox_authorization = checkbox_authorization_dummy;

    //get time
    configTime(0, 0, ntpServer);

    AsyncElegantOTA.begin(&server);
    server.begin();
    Serial.println("HTTP server started");
}


//MAIN######################################################################################################################################################################

void loop()
{

    AsyncElegantOTA.loop();

    if (Serial2.available())
    { //if serial input is available+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        //read input from optical input as integer
        eHZ_data = Serial2.read();

        //check for escape sequence in SML protocol 0x1b 0x1b 0x1b 0x1b
        if (eHZ_data == 27)
        { //Escape-Zeichen 0x1b, 4x hintereinander
            Escape_Zaehler++;
        }
        else
        {
            Escape_Zaehler = 0;
        }

        //if escape sequence is complete => evaluate data
        if (Escape_Zaehler == 4)
        {
            Escape = 1; // Datentelegramm vollstaendig empfangen, Auswertung kann nun erfolgen
                        // ===================================================================
                        // ==
                        // ==
                        // == Ausgabe in der Sektion Werte_Empfangen == 1...
                        // ==
                        // ===================================================================

            // Werte auswerten
            Zaehlvariable++; // Schleifenzaehler hochzaehlen und Wert im Terminal ausgeben

            //debug info
            if ((DEBUG) < 0x8000)
            {
                Serial.println("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"); // Trennzeile ausgeben
                sprintf(printstring, "%09ld >>>>", Zaehlvariable);
                Serial.println(printstring);
                Serial.println("");
            }

            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Seriennummer des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);        // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_SERIENNUMMER); // Seriennummer-Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_SERIENNUMMER; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_SERIENNUMMER];
                }
                Daten_Substring[LENGHT_SERIENNUMMER] = '\0';

                Zaehlernummer = 0; // ... und Nutzdaten dieses Threads auswerten
                Zaehlernummer = 16 * Zaehlernummer + Chr_2_Number(Daten_Substring[4]);
                Zaehlernummer = 16 * Zaehlernummer + Chr_2_Number(Daten_Substring[5]);
                Zaehlernummer = 16 * Zaehlernummer + Chr_2_Number(Daten_Substring[6]);
                Zaehlernummer = 16 * Zaehlernummer + Chr_2_Number(Daten_Substring[7]);
                Zaehlernummer = 16 * Zaehlernummer + Chr_2_Number(Daten_Substring[8]);
                Zaehlernummer = 16 * Zaehlernummer + Chr_2_Number(Daten_Substring[9]);
                Zaehlernummer = 16 * Zaehlernummer + Chr_2_Number(Daten_Substring[10]);
                Zaehlernummer = 16 * Zaehlernummer + Chr_2_Number(Daten_Substring[11]);

                Age_Zaehlernummer = 0; // ganz aktuelle Daten
                Werte_Empfangen = 1;   // Ausgabe anregen

                if (((DEBUG)&0x01) != 0)
                {                                      // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_SERIENNUMMER); // Kennung des Threads ausgeben
                    Serial.println(OBIS_SERIENNUMMER);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("SNR+SS:  "); // Kennung ausgeben
                    Serial.println(KENN_SERIENNUMMER);
                    Serial.print("SNR_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("SNR-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("SNR#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08ld", Zaehlernummer);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Server-ID des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);    // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_SERVERID); // Seriennummer-Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_SERVERID; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    //Daten_Substring[i] = eHZ_Message[i+Offset+OFFSET_SERVERID];
                    ServerID_string[i] = eHZ_Message[i + Offset + OFFSET_SERVERID];
                }
                ServerID_string[LENGHT_SERVERID] = '\0';

                ServerID_string_formatted[0] = ServerID_string[0];
                ServerID_string_formatted[1] = ServerID_string[1];
                ServerID_string_formatted[2] = '-';
                ServerID_string_formatted[3] = ServerID_string[2];
                ServerID_string_formatted[4] = ServerID_string[3];
                ServerID_string_formatted[5] = '-';
                ServerID_string_formatted[6] = ServerID_string[4];
                ServerID_string_formatted[7] = ServerID_string[5];
                ServerID_string_formatted[8] = '-';
                ServerID_string_formatted[9] = ServerID_string[6];
                ServerID_string_formatted[10] = ServerID_string[7];
                ServerID_string_formatted[11] = '-';
                ServerID_string_formatted[12] = ServerID_string[8];
                ServerID_string_formatted[13] = ServerID_string[9];
                ServerID_string_formatted[14] = '-';
                ServerID_string_formatted[15] = ServerID_string[10];
                ServerID_string_formatted[16] = ServerID_string[11];
                ServerID_string_formatted[17] = '-';
                ServerID_string_formatted[18] = ServerID_string[12];
                ServerID_string_formatted[19] = ServerID_string[13];
                ServerID_string_formatted[20] = '-';
                ServerID_string_formatted[21] = ServerID_string[14];
                ServerID_string_formatted[22] = ServerID_string[15];
                ServerID_string_formatted[23] = '-';
                ServerID_string_formatted[24] = ServerID_string[16];
                ServerID_string_formatted[25] = ServerID_string[17];
                ServerID_string_formatted[26] = '-';
                ServerID_string_formatted[27] = ServerID_string[18];
                ServerID_string_formatted[28] = ServerID_string[19];
                ServerID_string_formatted[29] = '\0';

                //Serial.println("Server ID:");
                //Serial.println(ServerID_string);
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Wirkenergie des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);       // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_WIRKENERGIE); // Wirkenergie-Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_WIRKENERGIE; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_WIRKENERGIE];
                }
                Daten_Substring[LENGHT_WIRKENERGIE] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit
                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                Wirkenergie = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                {
                    Wirkenergie = 16.0 * Wirkenergie + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkenergie = 16.0 * Wirkenergie + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                }
                case 0x58:
                {
                    Wirkenergie = 16.0 * Wirkenergie + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkenergie = 16.0 * Wirkenergie + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                }
                case 0x57:
                {
                    Wirkenergie = 16.0 * Wirkenergie + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkenergie = 16.0 * Wirkenergie + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                }
                case 0x56:
                {
                    Wirkenergie = 16.0 * Wirkenergie + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkenergie = 16.0 * Wirkenergie + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                }
                case 0x55:
                {
                    Wirkenergie = 16.0 * Wirkenergie + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkenergie = 16.0 * Wirkenergie + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                }
                case 0x54:
                {
                    Wirkenergie = 16.0 * Wirkenergie + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkenergie = 16.0 * Wirkenergie + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                }
                case 0x53:
                {
                    Wirkenergie = 16.0 * Wirkenergie + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkenergie = 16.0 * Wirkenergie + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                }
                case 0x52:
                {
                    Wirkenergie = 16.0 * Wirkenergie + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkenergie = 16.0 * Wirkenergie + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                }
                default:
                    Wirkenergie = 99999999999999;
                }

                //Serial.println(TL_Offset);
                //Serial.println("HUHU");

                Wirkenergie = Wirkenergie / 10.0; // Einheit Wh, 1 Nachkommastelle
                Age_Wirkenergie = 0;              // ganz aktuelle Daten
                Werte_Empfangen = 1;              // Ausgabe anregen

                if (((DEBUG)&0x04) != 0)
                {                                     // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_WIRKENERGIE); // Kennung des Threads ausgeben
                    Serial.println(OBIS_WIRKENERGIE);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("Msg+SS:  "); // Kennung ausgeben
                    Serial.println(KENN_WIRKENERGIE);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.4f", Wirkenergie);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   WirkenergieT1 des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);         // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_WIRKENERGIET1); // Wirkenergie-Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_WIRKENERGIET1; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_WIRKENERGIET1];
                }
                Daten_Substring[LENGHT_WIRKENERGIET1] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit
                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                WirkenergieT1 = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                {
                    WirkenergieT1 = 16.0 * WirkenergieT1 + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    WirkenergieT1 = 16.0 * WirkenergieT1 + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                }
                case 0x58:
                {
                    WirkenergieT1 = 16.0 * WirkenergieT1 + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    WirkenergieT1 = 16.0 * WirkenergieT1 + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                }
                case 0x57:
                {
                    WirkenergieT1 = 16.0 * WirkenergieT1 + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    WirkenergieT1 = 16.0 * WirkenergieT1 + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                }
                case 0x56:
                {
                    WirkenergieT1 = 16.0 * WirkenergieT1 + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    WirkenergieT1 = 16.0 * WirkenergieT1 + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                }
                case 0x55:
                {
                    WirkenergieT1 = 16.0 * WirkenergieT1 + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    WirkenergieT1 = 16.0 * WirkenergieT1 + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                }
                case 0x54:
                {
                    WirkenergieT1 = 16.0 * WirkenergieT1 + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    WirkenergieT1 = 16.0 * WirkenergieT1 + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                }
                case 0x53:
                {
                    WirkenergieT1 = 16.0 * WirkenergieT1 + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    WirkenergieT1 = 16.0 * WirkenergieT1 + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                }
                case 0x52:
                {
                    WirkenergieT1 = 16.0 * WirkenergieT1 + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    WirkenergieT1 = 16.0 * WirkenergieT1 + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                }
                default:
                    WirkenergieT1 = 99999999999999;
                }

                //Serial.println(TL_Offset);
                //Serial.println("HUHU");

                WirkenergieT1 = WirkenergieT1 / 10.0; // Einheit Wh, 1 Nachkommastelle
                Age_WirkenergieT1 = 0;                // ganz aktuelle Daten
                Werte_Empfangen = 1;                  // Ausgabe anregen

                if (((DEBUG)&0x04) != 0)
                {                                       // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_WIRKENERGIET1); // Kennung des Threads ausgeben
                    Serial.println(OBIS_WIRKENERGIET1);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("Msg+SS:  "); // Kennung ausgeben
                    Serial.println(KENN_WIRKENERGIET1);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.4f", WirkenergieT1);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   WirkenergieT2 des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);         // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_WIRKENERGIET2); // Wirkenergie-Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_WIRKENERGIET2; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_WIRKENERGIET2];
                }
                Daten_Substring[LENGHT_WIRKENERGIET2] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit
                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                WirkenergieT2 = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                {
                    WirkenergieT2 = 16.0 * WirkenergieT2 + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    WirkenergieT2 = 16.0 * WirkenergieT2 + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                }
                case 0x58:
                {
                    WirkenergieT2 = 16.0 * WirkenergieT2 + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    WirkenergieT2 = 16.0 * WirkenergieT2 + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                }
                case 0x57:
                {
                    WirkenergieT2 = 16.0 * WirkenergieT2 + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    WirkenergieT2 = 16.0 * WirkenergieT2 + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                }
                case 0x56:
                {
                    WirkenergieT2 = 16.0 * WirkenergieT2 + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    WirkenergieT2 = 16.0 * WirkenergieT2 + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                }
                case 0x55:
                {
                    WirkenergieT2 = 16.0 * WirkenergieT2 + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    WirkenergieT2 = 16.0 * WirkenergieT2 + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                }
                case 0x54:
                {
                    WirkenergieT2 = 16.0 * WirkenergieT2 + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    WirkenergieT2 = 16.0 * WirkenergieT2 + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                }
                case 0x53:
                {
                    WirkenergieT2 = 16.0 * WirkenergieT2 + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    WirkenergieT2 = 16.0 * WirkenergieT2 + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                }
                case 0x52:
                {
                    WirkenergieT2 = 16.0 * WirkenergieT2 + 1.0 * Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    WirkenergieT2 = 16.0 * WirkenergieT2 + 1.0 * Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                }
                default:
                    WirkenergieT2 = 99999999999999;
                }

                //Serial.println(TL_Offset);
                //Serial.println("HUHU");

                WirkenergieT2 = WirkenergieT2 / 10.0; // Einheit Wh, 1 Nachkommastelle
                Age_WirkenergieT2 = 0;                // ganz aktuelle Daten
                Werte_Empfangen = 1;                  // Ausgabe anregen

                if (((DEBUG)&0x04) != 0)
                {                                       // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_WIRKENERGIET2); // Kennung des Threads ausgeben
                    Serial.println(OBIS_WIRKENERGIET2);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("Msg+SS:  "); // Kennung ausgeben
                    Serial.println(KENN_WIRKENERGIET2);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.4f", WirkenergieT2);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Wirkleistung des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);        // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_WIRKLEISTUNG); // Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_WIRKLEISTUNG; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_WIRKLEISTUNG];
                }
                Daten_Substring[LENGHT_WIRKLEISTUNG] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit
                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                Wirkleistung = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    Wirkleistung = 16.0 * Wirkleistung + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung = 16.0 * Wirkleistung + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    Wirkleistung = 16.0 * Wirkleistung + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung = 16.0 * Wirkleistung + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    Wirkleistung = 16.0 * Wirkleistung + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung = 16.0 * Wirkleistung + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    Wirkleistung = 16.0 * Wirkleistung + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung = 16.0 * Wirkleistung + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    Wirkleistung = 16.0 * Wirkleistung + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung = 16.0 * Wirkleistung + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    Wirkleistung = 16.0 * Wirkleistung + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung = 16.0 * Wirkleistung + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    Wirkleistung = 16.0 * Wirkleistung + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung = 16.0 * Wirkleistung + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    Wirkleistung = 16.0 * Wirkleistung + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung = 16.0 * Wirkleistung + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    Wirkleistung = 99999999999999;
                }

                Wirkleistung = Wirkleistung / 1.0; // Einheit W, 0 Nachkommastellen
                Age_Wirkleistung = 0;              // ganz aktuelle Daten
                Werte_Empfangen = 1;               // Ausgabe anregen

                if (((DEBUG)&0x02) != 0)
                {                                      // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_WIRKLEISTUNG); // Kennung des Threads ausgeben
                    Serial.println(OBIS_WIRKLEISTUNG);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_WIRKLEISTUNG);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.0f", Wirkleistung);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Wirkleistung L1 des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);           // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_WIRKLEISTUNG_L1); // Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_WIRKLEISTUNG_L1; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_WIRKLEISTUNG_L1];
                }
                Daten_Substring[LENGHT_WIRKLEISTUNG_L1] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit

                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                Wirkleistung_L1 = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    Wirkleistung_L1 = 16.0 * Wirkleistung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L1 = 16.0 * Wirkleistung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    Wirkleistung_L1 = 16.0 * Wirkleistung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L1 = 16.0 * Wirkleistung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    Wirkleistung_L1 = 16.0 * Wirkleistung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L1 = 16.0 * Wirkleistung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    Wirkleistung_L1 = 16.0 * Wirkleistung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L1 = 16.0 * Wirkleistung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    Wirkleistung_L1 = 16.0 * Wirkleistung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L1 = 16.0 * Wirkleistung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    Wirkleistung_L1 = 16.0 * Wirkleistung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L1 = 16.0 * Wirkleistung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    Wirkleistung_L1 = 16.0 * Wirkleistung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L1 = 16.0 * Wirkleistung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    Wirkleistung_L1 = 16.0 * Wirkleistung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L1 = 16.0 * Wirkleistung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    Wirkleistung_L1 = 99999999999999;
                }

                Wirkleistung_L1 = Wirkleistung_L1 / 10.0; // Einheit W, 1 Nachkommastelle
                Age_Wirkleistung_L1 = 0;                  // ganz aktuelle Daten
                Werte_Empfangen = 1;                      // Ausgabe anregen

                if (((DEBUG)&0x08) != 0)
                {                                         // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_WIRKLEISTUNG_L1); // Kennung des Threads ausgeben
                    Serial.println(OBIS_WIRKLEISTUNG_L1);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_WIRKLEISTUNG_L1);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.0f", Wirkleistung_L1);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Blindleistung L1 des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);            // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_BLINDLEISTUNG_L1); // Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_BLINDLEISTUNG_L1; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_BLINDLEISTUNG_L1];
                }
                Daten_Substring[LENGHT_BLINDLEISTUNG_L1] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit

                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                           // Offset um dynamisches Lesen der Leistung zu ermöglichen

                Blindleistung_L1 = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    Blindleistung_L1 = 16.0 * Blindleistung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L1 = 16.0 * Blindleistung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    Blindleistung_L1 = 16.0 * Blindleistung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L1 = 16.0 * Blindleistung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    Blindleistung_L1 = 16.0 * Blindleistung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L1 = 16.0 * Blindleistung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    Blindleistung_L1 = 16.0 * Blindleistung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L1 = 16.0 * Blindleistung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    Blindleistung_L1 = 16.0 * Blindleistung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L1 = 16.0 * Blindleistung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    Blindleistung_L1 = 16.0 * Blindleistung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L1 = 16.0 * Blindleistung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    Blindleistung_L1 = 16.0 * Blindleistung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L1 = 16.0 * Blindleistung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    Blindleistung_L1 = 16.0 * Blindleistung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L1 = 16.0 * Blindleistung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    Blindleistung_L1 = 99999999999999;
                }

                Blindleistung_L1 = Blindleistung_L1 / 10.0; // Einheit var, 1 Nachkommastelle
                Age_BlindleistungL1 = 0;                    // ganz aktuelle Daten
                Werte_Empfangen = 1;                        // Ausgabe anregen

                if (((DEBUG)&0x08) != 0)
                {                                          // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_BLINDLEISTUNG_L1); // Kennung des Threads ausgeben
                    Serial.println(OBIS_BLINDLEISTUNG_L1);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_BLINDLEISTUNG_L1);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.0f", Blindleistung_L1);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Strom L1 des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);    // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_STROM_L1); // Kennung suchen und dann auswerten
            Offset = B_Daten - Beginn;                    // Offset der Nutzdaten dieses Threads berechnen

            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_STROM_L1; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_STROM_L1];
                }
                Daten_Substring[LENGHT_STROM_L1] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit

                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                Strom_L1 = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    Strom_L1 = 16.0 * Strom_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L1 = 16.0 * Strom_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    Strom_L1 = 16.0 * Strom_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L1 = 16.0 * Strom_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    Strom_L1 = 16.0 * Strom_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L1 = 16.0 * Strom_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    Strom_L1 = 16.0 * Strom_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L1 = 16.0 * Strom_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    Strom_L1 = 16.0 * Strom_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L1 = 16.0 * Strom_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    Strom_L1 = 16.0 * Strom_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L1 = 16.0 * Strom_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    Strom_L1 = 16.0 * Strom_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L1 = 16.0 * Strom_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    Strom_L1 = 16.0 * Strom_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L1 = 16.0 * Strom_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default: 
                    Strom_L1 = 99999999999999;

                }

                Strom_L1 = Strom_L1 / 100.0; // Einheit A, 2 Nachkommastellen
                Age_Strom_L1 = 0;            // ganz aktuelle Daten
                Werte_Empfangen = 1;         // Ausgabe anregen

                if (((DEBUG)&0x200) != 0)
                {                                      // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_WIRKLEISTUNG); // Kennung des Threads ausgeben
                    Serial.println(OBIS_WIRKLEISTUNG);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_WIRKLEISTUNG);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.0f", Strom_L1);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Spannung L1 des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);       // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_SPANNUNG_L1); // Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_SPANNUNG_L1; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_SPANNUNG_L1];
                }
                Daten_Substring[LENGHT_SPANNUNG_L1] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit

                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                Spannung_L1 = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    Spannung_L1 = 16.0 * Spannung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L1 = 16.0 * Spannung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    Spannung_L1 = 16.0 * Spannung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L1 = 16.0 * Spannung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    Spannung_L1 = 16.0 * Spannung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L1 = 16.0 * Spannung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    Spannung_L1 = 16.0 * Spannung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L1 = 16.0 * Spannung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    Spannung_L1 = 16.0 * Spannung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L1 = 16.0 * Spannung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    Spannung_L1 = 16.0 * Spannung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L1 = 16.0 * Spannung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    Spannung_L1 = 16.0 * Spannung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L1 = 16.0 * Spannung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    Spannung_L1 = 16.0 * Spannung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L1 = 16.0 * Spannung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    Spannung_L1 = 99999999999999;
                }

                Spannung_L1 = Spannung_L1 / 100.0; // Einheit V, 2 Nachkommastellen
                Age_Spannung_L1 = 0;               // ganz aktuelle Daten
                Werte_Empfangen = 1;               // Ausgabe anregen

                if (((DEBUG)&0x40) != 0)
                {                                     // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_SPANNUNG_L1); // Kennung des Threads ausgeben
                    Serial.println(OBIS_SPANNUNG_L1);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_SPANNUNG_L1);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.0f", Spannung_L1);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Phasenabweichung Strom/Spannung L1 des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);                // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_PHASENABWEICHUNGSSL1); // Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_PHASENABWEICHUNGSSL1; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_PHASENABWEICHUNGSSL1];
                }
                Daten_Substring[LENGHT_PHASENABWEICHUNGSSL1] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit
                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                PhasenabweichungStromSpannung_L1 = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    PhasenabweichungStromSpannung_L1 = 16.0 * PhasenabweichungStromSpannung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L1 = 16.0 * PhasenabweichungStromSpannung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    PhasenabweichungStromSpannung_L1 = 16.0 * PhasenabweichungStromSpannung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L1 = 16.0 * PhasenabweichungStromSpannung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    PhasenabweichungStromSpannung_L1 = 16.0 * PhasenabweichungStromSpannung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L1 = 16.0 * PhasenabweichungStromSpannung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    PhasenabweichungStromSpannung_L1 = 16.0 * PhasenabweichungStromSpannung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L1 = 16.0 * PhasenabweichungStromSpannung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    PhasenabweichungStromSpannung_L1 = 16.0 * PhasenabweichungStromSpannung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L1 = 16.0 * PhasenabweichungStromSpannung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    PhasenabweichungStromSpannung_L1 = 16.0 * PhasenabweichungStromSpannung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L1 = 16.0 * PhasenabweichungStromSpannung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    PhasenabweichungStromSpannung_L1 = 16.0 * PhasenabweichungStromSpannung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L1 = 16.0 * PhasenabweichungStromSpannung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    PhasenabweichungStromSpannung_L1 = 16.0 * PhasenabweichungStromSpannung_L1 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L1 = 16.0 * PhasenabweichungStromSpannung_L1 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    PhasenabweichungStromSpannung_L1 = 99999999999999;
                }

                PhasenabweichungStromSpannung_L1 = acos(PhasenabweichungStromSpannung_L1 / 65535.0)*180/PI; // Einheit cos(phi), 0 Nachkommastellen
                Age_PhasenabweichungStromSpannung_L1 = 0;                                  // ganz aktuelle Daten
                Werte_Empfangen = 1;                                                       // Ausgabe anregen

                if (((DEBUG)&0x02) != 0)
                {                                              // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_PHASENABWEICHUNGSSL1); // Kennung des Threads ausgeben
                    Serial.println(OBIS_PHASENABWEICHUNGSSL1);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_PHASENABWEICHUNGSSL1);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.0f", PhasenabweichungStromSpannung_L1);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Wirkleistung L2 des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);           // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_WIRKLEISTUNG_L2); // Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_WIRKLEISTUNG_L2; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_WIRKLEISTUNG_L2];
                }
                Daten_Substring[LENGHT_WIRKLEISTUNG_L2] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit

                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                Wirkleistung_L2 = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    Wirkleistung_L2 = 16.0 * Wirkleistung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L2 = 16.0 * Wirkleistung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    Wirkleistung_L2 = 16.0 * Wirkleistung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L2 = 16.0 * Wirkleistung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    Wirkleistung_L2 = 16.0 * Wirkleistung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L2 = 16.0 * Wirkleistung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    Wirkleistung_L2 = 16.0 * Wirkleistung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L2 = 16.0 * Wirkleistung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    Wirkleistung_L2 = 16.0 * Wirkleistung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L2 = 16.0 * Wirkleistung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    Wirkleistung_L2 = 16.0 * Wirkleistung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L2 = 16.0 * Wirkleistung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    Wirkleistung_L2 = 16.0 * Wirkleistung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L2 = 16.0 * Wirkleistung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    Wirkleistung_L2 = 16.0 * Wirkleistung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L2 = 16.0 * Wirkleistung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    Wirkleistung_L2 = 99999999999999;
                }

                Wirkleistung_L2 = Wirkleistung_L2 / 10.0; // Einheit W, 1 Nachkommastellen
                Age_Wirkleistung_L2 = 0;                  // ganz aktuelle Daten
                Werte_Empfangen = 1;                      // Ausgabe anregen

                if (((DEBUG)&0x10) != 0)
                {                                         // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_WIRKLEISTUNG_L2); // Kennung des Threads ausgeben
                    Serial.println(OBIS_WIRKLEISTUNG_L2);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_WIRKLEISTUNG_L2);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.0f", Wirkleistung_L2);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Blindleistung L2 des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);            // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_BLINDLEISTUNG_L2); // Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_BLINDLEISTUNG_L2; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_BLINDLEISTUNG_L2];
                }
                Daten_Substring[LENGHT_BLINDLEISTUNG_L2] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit

                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                Blindleistung_L2 = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    Blindleistung_L2 = 16.0 * Blindleistung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L2 = 16.0 * Blindleistung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    Blindleistung_L2 = 16.0 * Blindleistung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L2 = 16.0 * Blindleistung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    Blindleistung_L2 = 16.0 * Blindleistung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L2 = 16.0 * Blindleistung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    Blindleistung_L2 = 16.0 * Blindleistung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L2 = 16.0 * Blindleistung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    Blindleistung_L2 = 16.0 * Blindleistung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L2 = 16.0 * Blindleistung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    Blindleistung_L2 = 16.0 * Blindleistung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L2 = 16.0 * Blindleistung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    Blindleistung_L2 = 16.0 * Blindleistung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L2 = 16.0 * Blindleistung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    Blindleistung_L2 = 16.0 * Blindleistung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L2 = 16.0 * Blindleistung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    Blindleistung_L2 = 99999999999999;
                }

                Blindleistung_L2 = Blindleistung_L2 / 10.0; // Einheit var, 1 Nachkommastellen
                Age_BlindleistungL2 = 0;                    // ganz aktuelle Daten
                Werte_Empfangen = 1;                        // Ausgabe anregen

                if (((DEBUG)&0x10) != 0)
                {                                          // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_BLINDLEISTUNG_L2); // Kennung des Threads ausgeben
                    Serial.println(OBIS_BLINDLEISTUNG_L2);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_BLINDLEISTUNG_L2);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.0f", Blindleistung_L2);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Strom L2 des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);    // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_STROM_L2); // Kennung suchen und dann auswerten
            Offset = B_Daten - Beginn;                    // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_STROM_L2; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_STROM_L2];
                }
                Daten_Substring[LENGHT_STROM_L2] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit

                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                Strom_L2 = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    Strom_L2 = 16.0 * Strom_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L2 = 16.0 * Strom_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    Strom_L2 = 16.0 * Strom_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L2 = 16.0 * Strom_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    Strom_L2 = 16.0 * Strom_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L2 = 16.0 * Strom_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    Strom_L2 = 16.0 * Strom_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L2 = 16.0 * Strom_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    Strom_L2 = 16.0 * Strom_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L2 = 16.0 * Strom_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    Strom_L2 = 16.0 * Strom_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L2 = 16.0 * Strom_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    Strom_L2 = 16.0 * Strom_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L2 = 16.0 * Strom_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    Strom_L2 = 16.0 * Strom_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L2 = 16.0 * Strom_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    Strom_L2 = 99999999999999;
                }

                Strom_L2 = Strom_L2 / 100.0; // Einheit A, 2 Nachkommastellen
                Age_Strom_L2 = 0;            // ganz aktuelle Daten
                Werte_Empfangen = 1;         // Ausgabe anregen

                if (((DEBUG)&0x400) != 0)
                {                                      // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_WIRKLEISTUNG); // Kennung des Threads ausgeben
                    Serial.println(OBIS_WIRKLEISTUNG);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_WIRKLEISTUNG);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.0f", Strom_L2);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Spannung L2 des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);       // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_SPANNUNG_L2); // Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_SPANNUNG_L2; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_SPANNUNG_L2];
                }
                Daten_Substring[LENGHT_SPANNUNG_L2] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit

                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                Spannung_L2 = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    Spannung_L2 = 16.0 * Spannung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L2 = 16.0 * Spannung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    Spannung_L2 = 16.0 * Spannung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L2 = 16.0 * Spannung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    Spannung_L2 = 16.0 * Spannung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L2 = 16.0 * Spannung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    Spannung_L2 = 16.0 * Spannung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L2 = 16.0 * Spannung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    Spannung_L2 = 16.0 * Spannung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L2 = 16.0 * Spannung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    Spannung_L2 = 16.0 * Spannung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L2 = 16.0 * Spannung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    Spannung_L2 = 16.0 * Spannung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L2 = 16.0 * Spannung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    Spannung_L2 = 16.0 * Spannung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L2 = 16.0 * Spannung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    Spannung_L2 = 99999999999999;
                }

                Spannung_L2 = Spannung_L2 / 100.0; // Einheit V, 2 Nachkommastellen
                Age_Spannung_L2 = 0;               // ganz aktuelle Daten
                Werte_Empfangen = 1;               // Ausgabe anregen

                if (((DEBUG)&0x80) != 0)
                {                                      // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_WIRKLEISTUNG); // Kennung des Threads ausgeben
                    Serial.println(OBIS_WIRKLEISTUNG);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_WIRKLEISTUNG);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.0f", Spannung_L2);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Phasenabweichung Strom/Spannung L2 des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);                // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_PHASENABWEICHUNGSSL2); // Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_PHASENABWEICHUNGSSL2; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_PHASENABWEICHUNGSSL2];
                }
                Daten_Substring[LENGHT_PHASENABWEICHUNGSSL2] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit
                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                PhasenabweichungStromSpannung_L2 = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    PhasenabweichungStromSpannung_L2 = 16.0 * PhasenabweichungStromSpannung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L2 = 16.0 * PhasenabweichungStromSpannung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    PhasenabweichungStromSpannung_L2 = 16.0 * PhasenabweichungStromSpannung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L2 = 16.0 * PhasenabweichungStromSpannung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    PhasenabweichungStromSpannung_L2 = 16.0 * PhasenabweichungStromSpannung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L2 = 16.0 * PhasenabweichungStromSpannung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    PhasenabweichungStromSpannung_L2 = 16.0 * PhasenabweichungStromSpannung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L2 = 16.0 * PhasenabweichungStromSpannung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    PhasenabweichungStromSpannung_L2 = 16.0 * PhasenabweichungStromSpannung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L2 = 16.0 * PhasenabweichungStromSpannung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    PhasenabweichungStromSpannung_L2 = 16.0 * PhasenabweichungStromSpannung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L2 = 16.0 * PhasenabweichungStromSpannung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    PhasenabweichungStromSpannung_L2 = 16.0 * PhasenabweichungStromSpannung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L2 = 16.0 * PhasenabweichungStromSpannung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    PhasenabweichungStromSpannung_L2 = 16.0 * PhasenabweichungStromSpannung_L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L2 = 16.0 * PhasenabweichungStromSpannung_L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    PhasenabweichungStromSpannung_L2 = 99999999999999;
                }

                PhasenabweichungStromSpannung_L2 = acos(PhasenabweichungStromSpannung_L2 / 65535.0)*180/PI; // Einheit cos(phi), 0 Nachkommastellen
                Age_PhasenabweichungStromSpannung_L2 = 0;                                  // ganz aktuelle Daten
                Werte_Empfangen = 1;                                                       // Ausgabe anregen

                if (((DEBUG)&0x02) != 0)
                {                                              // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_PHASENABWEICHUNGSSL2); // Kennung des Threads ausgeben
                    Serial.println(OBIS_PHASENABWEICHUNGSSL2);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_PHASENABWEICHUNGSSL2);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.0f", PhasenabweichungStromSpannung_L2);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Wirkleistung L3 des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);           // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_WIRKLEISTUNG_L3); // Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_WIRKLEISTUNG_L3; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_WIRKLEISTUNG_L3];
                }
                Daten_Substring[LENGHT_WIRKLEISTUNG_L3] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit

                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                Wirkleistung_L3 = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    Wirkleistung_L3 = 16.0 * Wirkleistung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L3 = 16.0 * Wirkleistung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    Wirkleistung_L3 = 16.0 * Wirkleistung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L3 = 16.0 * Wirkleistung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    Wirkleistung_L3 = 16.0 * Wirkleistung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L3 = 16.0 * Wirkleistung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    Wirkleistung_L3 = 16.0 * Wirkleistung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L3 = 16.0 * Wirkleistung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    Wirkleistung_L3 = 16.0 * Wirkleistung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L3 = 16.0 * Wirkleistung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    Wirkleistung_L3 = 16.0 * Wirkleistung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L3 = 16.0 * Wirkleistung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    Wirkleistung_L3 = 16.0 * Wirkleistung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L3 = 16.0 * Wirkleistung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    Wirkleistung_L3 = 16.0 * Wirkleistung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Wirkleistung_L3 = 16.0 * Wirkleistung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    Wirkleistung_L3 = 99999999999999;
                }

                Wirkleistung_L3 = Wirkleistung_L3 / 10.0; // Einheit W, 1 Nachkommastellen
                Age_Wirkleistung_L3 = 0;                  // ganz aktuelle Daten
                Werte_Empfangen = 1;                      // Ausgabe anregen

                if (((DEBUG)&0x20) != 0)
                {                                         // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_WIRKLEISTUNG_L3); // Kennung des Threads ausgeben
                    Serial.println(OBIS_WIRKLEISTUNG_L3);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_WIRKLEISTUNG_L3);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.0f", Wirkleistung_L3);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Blindleistung L3 des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);            // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_BLINDLEISTUNG_L3); // Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_BLINDLEISTUNG_L3; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_BLINDLEISTUNG_L3];
                }
                Daten_Substring[LENGHT_BLINDLEISTUNG_L3] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit

                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                Blindleistung_L3 = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    Blindleistung_L3 = 16.0 * Blindleistung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L3 = 16.0 * Blindleistung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    Blindleistung_L3 = 16.0 * Blindleistung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L3 = 16.0 * Blindleistung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    Blindleistung_L3 = 16.0 * Blindleistung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L3 = 16.0 * Blindleistung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    Blindleistung_L3 = 16.0 * Blindleistung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L3 = 16.0 * Blindleistung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    Blindleistung_L3 = 16.0 * Blindleistung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L3 = 16.0 * Blindleistung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    Blindleistung_L3 = 16.0 * Blindleistung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L3 = 16.0 * Blindleistung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    Blindleistung_L3 = 16.0 * Blindleistung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L3 = 16.0 * Blindleistung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    Blindleistung_L3 = 16.0 * Blindleistung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Blindleistung_L3 = 16.0 * Blindleistung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    Blindleistung_L3 = 99999999999999;
                }

                Blindleistung_L3 = Blindleistung_L3 / 10.0; // Einheit var, 1 Nachkommastellen
                Age_BlindleistungL3 = 0;                    // ganz aktuelle Daten
                Werte_Empfangen = 1;                        // Ausgabe anregen

                if (((DEBUG)&0x20) != 0)
                {                                          // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_BLINDLEISTUNG_L3); // Kennung des Threads ausgeben
                    Serial.println(OBIS_BLINDLEISTUNG_L3);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_BLINDLEISTUNG_L3);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.0f", Blindleistung_L3);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Strom L3 des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);    // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_STROM_L3); // Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_STROM_L3; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_STROM_L3];
                }
                Daten_Substring[LENGHT_STROM_L3] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit

                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                Strom_L3 = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    Strom_L3 = 16.0 * Strom_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L3 = 16.0 * Strom_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    Strom_L3 = 16.0 * Strom_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L3 = 16.0 * Strom_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    Strom_L3 = 16.0 * Strom_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L3 = 16.0 * Strom_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    Strom_L3 = 16.0 * Strom_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L3 = 16.0 * Strom_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    Strom_L3 = 16.0 * Strom_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L3 = 16.0 * Strom_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    Strom_L3 = 16.0 * Strom_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L3 = 16.0 * Strom_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    Strom_L3 = 16.0 * Strom_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L3 = 16.0 * Strom_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    Strom_L3 = 16.0 * Strom_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Strom_L3 = 16.0 * Strom_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    Strom_L3 = 99999999999999;
                }

                Strom_L3 = Strom_L3 / 100.0; // Einheit A, 2 Nachkommastellen
                Age_Strom_L3 = 0;            // ganz aktuelle Daten
                Werte_Empfangen = 1;         // Ausgabe anregen

                if (((DEBUG)&0x800) != 0)
                {                                  // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_STROM_L3); // Kennung des Threads ausgeben
                    Serial.println(OBIS_STROM_L3);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_STROM_L3);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.0f", Strom_L3);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Spannung L3 des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);       // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_SPANNUNG_L3); // Kennung suchen und dann auswerten
            Offset = B_Daten - Beginn;                       // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_SPANNUNG_L3; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_SPANNUNG_L3];
                }
                Daten_Substring[LENGHT_SPANNUNG_L3] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit

                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                Spannung_L3 = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    Spannung_L3 = 16.0 * Spannung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L3 = 16.0 * Spannung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    Spannung_L3 = 16.0 * Spannung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L3 = 16.0 * Spannung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    Spannung_L3 = 16.0 * Spannung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L3 = 16.0 * Spannung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    Spannung_L3 = 16.0 * Spannung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L3 = 16.0 * Spannung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    Spannung_L3 = 16.0 * Spannung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L3 = 16.0 * Spannung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    Spannung_L3 = 16.0 * Spannung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L3 = 16.0 * Spannung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    Spannung_L3 = 16.0 * Spannung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L3 = 16.0 * Spannung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    Spannung_L3 = 16.0 * Spannung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_L3 = 16.0 * Spannung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    Spannung_L3 = 99999999999999;
                }

                Spannung_L3 = Spannung_L3 / 100.0; // Einheit V, 2 Nachkommastellen
                Age_Spannung_L3 = 0;               // ganz aktuelle Daten
                Werte_Empfangen = 1;               // Ausgabe anregen

                if (((DEBUG)&0x100) != 0)
                {                                     // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_SPANNUNG_L3); // Kennung des Threads ausgeben
                    Serial.println(OBIS_SPANNUNG_L3);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_SPANNUNG_L3);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.0f", Spannung_L3);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Phasenabweichung Strom/Spannung L3 des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);                // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_PHASENABWEICHUNGSSL3); // Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_PHASENABWEICHUNGSSL3; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_PHASENABWEICHUNGSSL3];
                }
                Daten_Substring[LENGHT_PHASENABWEICHUNGSSL3] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit
                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                PhasenabweichungStromSpannung_L3 = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    PhasenabweichungStromSpannung_L3 = 16.0 * PhasenabweichungStromSpannung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L3 = 16.0 * PhasenabweichungStromSpannung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    PhasenabweichungStromSpannung_L3 = 16.0 * PhasenabweichungStromSpannung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L3 = 16.0 * PhasenabweichungStromSpannung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    PhasenabweichungStromSpannung_L3 = 16.0 * PhasenabweichungStromSpannung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L3 = 16.0 * PhasenabweichungStromSpannung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    PhasenabweichungStromSpannung_L3 = 16.0 * PhasenabweichungStromSpannung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L3 = 16.0 * PhasenabweichungStromSpannung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    PhasenabweichungStromSpannung_L3 = 16.0 * PhasenabweichungStromSpannung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L3 = 16.0 * PhasenabweichungStromSpannung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    PhasenabweichungStromSpannung_L3 = 16.0 * PhasenabweichungStromSpannung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L3 = 16.0 * PhasenabweichungStromSpannung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    PhasenabweichungStromSpannung_L3 = 16.0 * PhasenabweichungStromSpannung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L3 = 16.0 * PhasenabweichungStromSpannung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    PhasenabweichungStromSpannung_L3 = 16.0 * PhasenabweichungStromSpannung_L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungStromSpannung_L3 = 16.0 * PhasenabweichungStromSpannung_L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    PhasenabweichungStromSpannung_L3 = 99999999999999;
                }

                PhasenabweichungStromSpannung_L3 = acos(PhasenabweichungStromSpannung_L3 / 65535.0)*180/PI; // Einheit cos(phi), 0 Nachkommastellen
                Age_PhasenabweichungStromSpannung_L3 = 0;                                  // ganz aktuelle Daten
                Werte_Empfangen = 1;                                                       // Ausgabe anregen

                if (((DEBUG)&0x02) != 0)
                {                                              // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_PHASENABWEICHUNGSSL3); // Kennung des Threads ausgeben
                    Serial.println(OBIS_PHASENABWEICHUNGSSL3);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_PHASENABWEICHUNGSSL3);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.0f", PhasenabweichungStromSpannung_L3);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Phasenabweichung Spannungen L1/L2 des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);                // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_PHASENABWEICHUNGL1L2); // Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_PHASENABWEICHUNGL1L2; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_PHASENABWEICHUNGL1L2];
                }
                Daten_Substring[LENGHT_PHASENABWEICHUNGL1L2] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit
                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                PhasenabweichungSpannungen_L1L2 = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    PhasenabweichungSpannungen_L1L2 = 16.0 * PhasenabweichungSpannungen_L1L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungSpannungen_L1L2 = 16.0 * PhasenabweichungSpannungen_L1L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    PhasenabweichungSpannungen_L1L2 = 16.0 * PhasenabweichungSpannungen_L1L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungSpannungen_L1L2 = 16.0 * PhasenabweichungSpannungen_L1L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    PhasenabweichungSpannungen_L1L2 = 16.0 * PhasenabweichungSpannungen_L1L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungSpannungen_L1L2 = 16.0 * PhasenabweichungSpannungen_L1L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    PhasenabweichungSpannungen_L1L2 = 16.0 * PhasenabweichungSpannungen_L1L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungSpannungen_L1L2 = 16.0 * PhasenabweichungSpannungen_L1L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    PhasenabweichungSpannungen_L1L2 = 16.0 * PhasenabweichungSpannungen_L1L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungSpannungen_L1L2 = 16.0 * PhasenabweichungSpannungen_L1L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    PhasenabweichungSpannungen_L1L2 = 16.0 * PhasenabweichungSpannungen_L1L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungSpannungen_L1L2 = 16.0 * PhasenabweichungSpannungen_L1L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    PhasenabweichungSpannungen_L1L2 = 16.0 * PhasenabweichungSpannungen_L1L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungSpannungen_L1L2 = 16.0 * PhasenabweichungSpannungen_L1L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    PhasenabweichungSpannungen_L1L2 = 16.0 * PhasenabweichungSpannungen_L1L2 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungSpannungen_L1L2 = 16.0 * PhasenabweichungSpannungen_L1L2 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    PhasenabweichungSpannungen_L1L2 = 99999999999999;
                }

                PhasenabweichungSpannungen_L1L2 = acos(PhasenabweichungSpannungen_L1L2 / 65535.0)*180/PI; // Einheit cos(phi), 0 Nachkommastellen
                Age_PhasenabweichungSpannungen_L1L2 = 0;                                 // ganz aktuelle Daten
                Werte_Empfangen = 1;                                                     // Ausgabe anregen

                if (((DEBUG)&0x02) != 0)
                {                                              // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_PHASENABWEICHUNGL1L2); // Kennung des Threads ausgeben
                    Serial.println(OBIS_PHASENABWEICHUNGL1L2);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_PHASENABWEICHUNGL1L2);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.0f", PhasenabweichungSpannungen_L1L2);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Phasenabweichung Spannungen L1/L3 des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);                // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_PHASENABWEICHUNGL1L3); // Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_PHASENABWEICHUNGL1L3; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_PHASENABWEICHUNGL1L3];
                }
                Daten_Substring[LENGHT_PHASENABWEICHUNGL1L3] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit
                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                PhasenabweichungSpannungen_L1L3 = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    PhasenabweichungSpannungen_L1L3 = 16.0 * PhasenabweichungSpannungen_L1L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungSpannungen_L1L3 = 16.0 * PhasenabweichungSpannungen_L1L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    PhasenabweichungSpannungen_L1L3 = 16.0 * PhasenabweichungSpannungen_L1L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungSpannungen_L1L3 = 16.0 * PhasenabweichungSpannungen_L1L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    PhasenabweichungSpannungen_L1L3 = 16.0 * PhasenabweichungSpannungen_L1L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungSpannungen_L1L3 = 16.0 * PhasenabweichungSpannungen_L1L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    PhasenabweichungSpannungen_L1L3 = 16.0 * PhasenabweichungSpannungen_L1L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungSpannungen_L1L3 = 16.0 * PhasenabweichungSpannungen_L1L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    PhasenabweichungSpannungen_L1L3 = 16.0 * PhasenabweichungSpannungen_L1L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungSpannungen_L1L3 = 16.0 * PhasenabweichungSpannungen_L1L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    PhasenabweichungSpannungen_L1L3 = 16.0 * PhasenabweichungSpannungen_L1L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungSpannungen_L1L3 = 16.0 * PhasenabweichungSpannungen_L1L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    PhasenabweichungSpannungen_L1L3 = 16.0 * PhasenabweichungSpannungen_L1L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungSpannungen_L1L3 = 16.0 * PhasenabweichungSpannungen_L1L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    PhasenabweichungSpannungen_L1L3 = 16.0 * PhasenabweichungSpannungen_L1L3 + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    PhasenabweichungSpannungen_L1L3 = 16.0 * PhasenabweichungSpannungen_L1L3 + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    PhasenabweichungSpannungen_L1L3 = 99999999999999;
                }

                PhasenabweichungSpannungen_L1L3 = acos(PhasenabweichungSpannungen_L1L3 / 65535.0)*180/PI; // Einheit cos(phi), 0 Nachkommastellen
                Age_PhasenabweichungSpannungen_L1L3 = 0;                                 // ganz aktuelle Daten
                Werte_Empfangen = 1;                                                     // Ausgabe anregen

                if (((DEBUG)&0x02) != 0)
                {                                              // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_PHASENABWEICHUNGL1L3); // Kennung des Threads ausgeben
                    Serial.println(OBIS_PHASENABWEICHUNGL1L3);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_PHASENABWEICHUNGL1L3);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.0f", PhasenabweichungSpannungen_L1L3);
                    Serial.println(printstring);
                    Serial.println(""); 
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Chiptemperatur
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);    // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_CHIPTEMP); // Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_CHIPTEMP; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_CHIPTEMP];
                }
                Daten_Substring[LENGHT_CHIPTEMP] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit

                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit

                TL_Offset = 0; // Offset um dynamisches Lesen der Leistung zu ermöglichen

                Chiptemp = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    Chiptemp = 16.0 * Chiptemp + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp = 16.0 * Chiptemp + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    Chiptemp = 16.0 * Chiptemp + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp = 16.0 * Chiptemp + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    Chiptemp = 16.0 * Chiptemp + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp = 16.0 * Chiptemp + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    Chiptemp = 16.0 * Chiptemp + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp = 16.0 * Chiptemp + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    Chiptemp = 16.0 * Chiptemp + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp = 16.0 * Chiptemp + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    Chiptemp = 16.0 * Chiptemp + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp = 16.0 * Chiptemp + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    Chiptemp = 16.0 * Chiptemp + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp = 16.0 * Chiptemp + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    Chiptemp = 16.0 * Chiptemp + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp = 16.0 * Chiptemp + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    Chiptemp = 99999999999999;
                }

                Chiptemp = Chiptemp / 10.0; // Einheit °C, 1 Nachkommastellen
                Age_Chiptemp = 0;           // ganz aktuelle Daten
                Werte_Empfangen = 1;        // Ausgabe anregen

                if (((DEBUG)&0x4000) != 0)
                {                                  // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_CHIPTEMP); // Kennung des Threads ausgeben
                    Serial.println(OBIS_CHIPTEMP);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_CHIPTEMP);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.1f", Chiptemp);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Spannung MIN des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);        // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_SPANNUNG_MIN); // Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_SPANNUNG_MIN; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_SPANNUNG_MIN];
                }
                Daten_Substring[LENGHT_SPANNUNG_MIN] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit

                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit

                TL_Offset = 0; // Offset um dynamisches Lesen der Leistung zu ermöglichen

                Spannung_Min = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x69:
                    Spannung_Min = 16.0 * Spannung_Min + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_Min = 16.0 * Spannung_Min + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x68:
                    Spannung_Min = 16.0 * Spannung_Min + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_Min = 16.0 * Spannung_Min + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x67:
                    Spannung_Min = 16.0 * Spannung_Min + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_Min = 16.0 * Spannung_Min + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x66:
                    Spannung_Min = 16.0 * Spannung_Min + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_Min = 16.0 * Spannung_Min + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x65:
                    Spannung_Min = 16.0 * Spannung_Min + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_Min = 16.0 * Spannung_Min + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x64:
                    Spannung_Min = 16.0 * Spannung_Min + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_Min = 16.0 * Spannung_Min + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x63:
                    Spannung_Min = 16.0 * Spannung_Min + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_Min = 16.0 * Spannung_Min + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x62:
                    Spannung_Min = 16.0 * Spannung_Min + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_Min = 16.0 * Spannung_Min + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    Spannung_Min = 99999999999999;
                }

                Spannung_Min = Spannung_Min / 1.0; // Einheit V, 0 Nachkommastellen
                Age_Spannung_Min = 0;              // ganz aktuelle Daten
                Werte_Empfangen = 1;               // Ausgabe anregen

                if (((DEBUG)&0x2000) != 0)
                {                                      // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_SPANNUNG_MIN); // Kennung des Threads ausgeben
                    Serial.println(OBIS_SPANNUNG_MIN);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_SPANNUNG_MIN);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.0f", Spannung_Min);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Spannung MAX des Zaehlers auswerten
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);        // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_SPANNUNG_MAX); // Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_SPANNUNG_MAX; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_SPANNUNG_MAX];
                }
                Daten_Substring[LENGHT_SPANNUNG_MAX] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit

                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit

                TL_Offset = 0; // Offset um dynamisches Lesen der Leistung zu ermöglichen

                Spannung_Max = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x69:
                    Spannung_Max = 16.0 * Spannung_Max + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_Max = 16.0 * Spannung_Max + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x68:
                    Spannung_Max = 16.0 * Spannung_Max + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_Max = 16.0 * Spannung_Max + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x67:
                    Spannung_Max = 16.0 * Spannung_Max + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_Max = 16.0 * Spannung_Max + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x66:
                    Spannung_Max = 16.0 * Spannung_Max + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_Max = 16.0 * Spannung_Max + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x65:
                    Spannung_Max = 16.0 * Spannung_Max + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_Max = 16.0 * Spannung_Max + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x64:
                    Spannung_Max = 16.0 * Spannung_Max + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_Max = 16.0 * Spannung_Max + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x63:
                    Spannung_Max = 16.0 * Spannung_Max + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_Max = 16.0 * Spannung_Max + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x62:
                    Spannung_Max = 16.0 * Spannung_Max + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Spannung_Max = 16.0 * Spannung_Max + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    Spannung_Max = 99999999999999;
                }

                Spannung_Max = Spannung_Max / 1.0; // Einheit V, 0 Nachkommastellen
                Age_Spannung_Max = 0;              // ganz aktuelle Daten
                Werte_Empfangen = 1;               // Ausgabe anregen

                if (((DEBUG)&0x1000) != 0)
                {                                      // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_SPANNUNG_MAX); // Kennung des Threads ausgeben
                    Serial.println(OBIS_SPANNUNG_MAX);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_SPANNUNG_MAX);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.0f", Spannung_Max);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Minimale Chiptemperatur
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);       // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_CHIPTEMPMIN); // Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_CHIPTEMPMIN; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_CHIPTEMPMIN];
                }
                Daten_Substring[LENGHT_CHIPTEMPMIN] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit

                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit

                TL_Offset = 0; // Offset um dynamisches Lesen der Leistung zu ermöglichen

                Chiptemp_Min = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    Chiptemp_Min = 16.0 * Chiptemp_Min + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Min = 16.0 * Chiptemp_Min + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    Chiptemp_Min = 16.0 * Chiptemp_Min + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Min = 16.0 * Chiptemp_Min + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    Chiptemp_Min = 16.0 * Chiptemp_Min + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Min = 16.0 * Chiptemp_Min + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    Chiptemp_Min = 16.0 * Chiptemp_Min + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Min = 16.0 * Chiptemp_Min + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    Chiptemp_Min = 16.0 * Chiptemp_Min + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Min = 16.0 * Chiptemp_Min + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    Chiptemp_Min = 16.0 * Chiptemp_Min + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Min = 16.0 * Chiptemp_Min + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    Chiptemp_Min = 16.0 * Chiptemp_Min + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Min = 16.0 * Chiptemp_Min + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    Chiptemp_Min = 16.0 * Chiptemp_Min + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Min = 16.0 * Chiptemp_Min + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    Chiptemp_Min = 99999999999999;
                }

                Chiptemp_Min = Chiptemp_Min / 1.0; // Einheit °C, 1 Nachkommastellen
                Age_Chiptemp_Min = 0;              // ganz aktuelle Daten
                Werte_Empfangen = 1;               // Ausgabe anregen

                if (((DEBUG)&0x4000) != 0)
                {                                     // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_CHIPTEMPMIN); // Kennung des Threads ausgeben
                    Serial.println(OBIS_CHIPTEMPMIN);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_CHIPTEMPMIN);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.1f", Chiptemp_Min);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Maximale Chiptemperatur
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);       // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_CHIPTEMPMAX); // Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_CHIPTEMPMAX; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_CHIPTEMPMAX];
                }
                Daten_Substring[LENGHT_CHIPTEMPMAX] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit

                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit

                TL_Offset = 0; // Offset um dynamisches Lesen der Leistung zu ermöglichen

                Chiptemp_Max = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    Chiptemp_Max = 16.0 * Chiptemp_Max + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Max = 16.0 * Chiptemp_Max + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    Chiptemp_Max = 16.0 * Chiptemp_Max + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Max = 16.0 * Chiptemp_Max + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    Chiptemp_Max = 16.0 * Chiptemp_Max + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Max = 16.0 * Chiptemp_Max + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    Chiptemp_Max = 16.0 * Chiptemp_Max + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Max = 16.0 * Chiptemp_Max + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    Chiptemp_Max = 16.0 * Chiptemp_Max + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Max = 16.0 * Chiptemp_Max + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    Chiptemp_Max = 16.0 * Chiptemp_Max + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Max = 16.0 * Chiptemp_Max + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    Chiptemp_Max = 16.0 * Chiptemp_Max + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Max = 16.0 * Chiptemp_Max + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    Chiptemp_Max = 16.0 * Chiptemp_Max + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Max = 16.0 * Chiptemp_Max + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    Chiptemp_Max = 99999999999999;
                }

                Chiptemp_Max = Chiptemp_Max / 1.0; // Einheit °C, 1 Nachkommastellen
                Age_Chiptemp_Max = 0;              // ganz aktuelle Daten
                Werte_Empfangen = 1;               // Ausgabe anregen

                if (((DEBUG)&0x4000) != 0)
                {                                     // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_CHIPTEMPMAX); // Kennung des Threads ausgeben
                    Serial.println(OBIS_CHIPTEMPMAX);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_CHIPTEMPMAX);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.1f", Chiptemp_Max);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Gemittelte Chiptemperatur
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);       // Pointer auf den Anfang der Message
            B_Daten = strstr(eHZ_Message, KENN_CHIPTEMPAVG); // Kennung suchen und dann auswerten

            Offset = B_Daten - Beginn; // Offset der Nutzdaten dieses Threads berechnen
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_CHIPTEMPAVG; i++)
                { // ... und Nutzdaten dieses Threads kopieren
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_CHIPTEMPAVG];
                }
                Daten_Substring[LENGHT_CHIPTEMPAVG] = '\0';

                // T_L Wert auswerten um die nachfolgenden Daten korrekt zu interpretieren (Datenlänge 8/16/24.../56/64 bit

                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit

                TL_Offset = 0; // Offset um dynamisches Lesen der Leistung zu ermöglichen

                Chiptemp_Avg = 0.0; // ... und Nutzdaten dieses Threads auswerten
                switch (TL_Flag)
                { // dynamische Datenlaengenanpassung.... da waren Irre am Werk!!!
                case 0x59:
                    Chiptemp_Avg = 16.0 * Chiptemp_Avg + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Avg = 16.0 * Chiptemp_Avg + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x58:
                    Chiptemp_Avg = 16.0 * Chiptemp_Avg + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Avg = 16.0 * Chiptemp_Avg + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x57:
                    Chiptemp_Avg = 16.0 * Chiptemp_Avg + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Avg = 16.0 * Chiptemp_Avg + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x56:
                    Chiptemp_Avg = 16.0 * Chiptemp_Avg + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Avg = 16.0 * Chiptemp_Avg + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x55:
                    Chiptemp_Avg = 16.0 * Chiptemp_Avg + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Avg = 16.0 * Chiptemp_Avg + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x54:
                    Chiptemp_Avg = 16.0 * Chiptemp_Avg + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Avg = 16.0 * Chiptemp_Avg + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x53:
                    Chiptemp_Avg = 16.0 * Chiptemp_Avg + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Avg = 16.0 * Chiptemp_Avg + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    TL_Offset += 2;
                case 0x52:
                    Chiptemp_Avg = 16.0 * Chiptemp_Avg + Chr_2_Number(Daten_Substring[(2 + TL_Offset)]);
                    Chiptemp_Avg = 16.0 * Chiptemp_Avg + Chr_2_Number(Daten_Substring[(3 + TL_Offset)]);
                    break;
                default:
                    Chiptemp_Avg = 99999999999999;
                }

                Chiptemp_Avg = Chiptemp_Avg / 1.0; // Einheit °C, 1 Nachkommastellen
                Age_Chiptemp_Avg = 0;              // ganz aktuelle Daten
                Werte_Empfangen = 1;               // Ausgabe anregen

                if (((DEBUG)&0x4000) != 0)
                {                                     // Debug-Messages dieses Threads ausgeben
                    Serial.println(TEXT_CHIPTEMPAVG); // Kennung des Threads ausgeben
                    Serial.println(OBIS_CHIPTEMPAVG);
                    Serial.print("MSG:     "); // eHZ-Message ausgeben
                    Serial.println(eHZ_Message);
                    Serial.print("KENN  :  "); // Kennung ausgeben
                    Serial.println(KENN_CHIPTEMPAVG);
                    Serial.print("Msg_SS:  "); // Beginn der Daten mit Kennung ausgeben
                    Serial.println(B_Daten);
                    Serial.print("Msg-STR> "); // Daten Substring nach Entfernen der Kennung
                    Serial.println(Daten_Substring);
                    Serial.print("Msg#     "); // Nutzdaten ausgeben
                    sprintf(printstring, "%08.1f", Chiptemp_Avg);
                    Serial.println(printstring);
                    Serial.println("");
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ende
            // =
            // -------------------------------------------------------------------------------------------------------

            if ((DEBUG) < 0x800000)
            {
                Serial.println("------------------------------------------------------------------"); // Trennzeile ausgeben
            }

            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Gesammelte Werte können ab hier verarbeitet werden
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            if (((DEBUG)&0x800000) == 0)
            {
                Werte_Empfangen = 2; // Momentan Ausgabe unterdruecken
            }

            if (Werte_Empfangen == 1)
            {

                timestamp = getTimestamp();

                if (timestamp != 0)
                {
                    HTTPClient http;
                    //   http.begin("https://zuse.icas.fh-dortmund.de:2443/ict-gw/sml");
                    //   http.addHeader("x-api-key","lz7L445xjHLvBUvr5zmwJf4tmRTeIQCX");
                    //   http.addHeader("Authorization","Bearer ");
                    //   http.addHeader("x-host-override","sdc-service-api");
                    //   http.addHeader("Content-Type", "application/json");
                    http.begin(Serveraddress);
                    if (checkbox_apikey)
                    {
                        http.addHeader("x-api-key", Apikey);
                    }
                    if (checkbox_authorization)
                    {
                        http.addHeader("Authorization", Bearer);
                    }

                    http.addHeader("x-host-override", "sdc-service-api");
                    http.addHeader("Content-Type", "application/json");

                    String httpRequestData = "{\"serverID\": \"" + String(ServerID_string_formatted) + "\",\"timestamp\":" + String(timestamp) + "000,\"channel\": [{\"obis\": \"0100010800ff\",\"description\": \"Wirkenergie Bezug Gesamt\",\"value\":" + String(Wirkenergie) + " ,\"unit\": \"WATT_HOUR\"},{\"obis\": \"0100010801ff\",\"description\": \"Wirkenergie Bezug Tarif 1\",\"value\": " + String(WirkenergieT1) + " ,\"unit\": \"WATT_HOUR\"},{\"obis\": \"0100010802ff\",\"description\": \"Wirkenergie Bezug Tarif 2\",\"value\": " + String(WirkenergieT2) + " ,\"unit\": \"WATT_HOUR\"},{\"obis\": \"0100100700ff\",\"description\": \"Momentanleistung Gesamt\",\"value\": " + String(Wirkleistung) + " ,\"unit\": \"WATT\"},{\"obis\": \"0100240700ff\",\"description\": \"Wirkleistung L1\",\"value\": " + String(Wirkleistung_L1) + " ,\"unit\": \"WATT\"},{\"obis\": \"0100170700ff\",\"description\": \"Blindleistung L1\",\"value\": " + String(Blindleistung_L1) + " ,\"unit\": \"VAR\"},{\"obis\": \"01001f0700ff\",\"description\": \"Strom L1\",\"value\": " + String(Strom_L1) + " ,\"unit\": \"AMPERE\"},{\"obis\": \"0100200700ff\",\"description\": \"Spannung L1\",\"value\":" + String(Spannung_L1) + " ,\"unit\": \"VOLT\"},{\"obis\": \"0100510704ff\",\"description\": \"Phasenanweichung Strom/Spannung L1\",\"value\": " + String(PhasenabweichungStromSpannung_L1) + " ,\"unit\": \"DEGREE\"},{\"obis\": \"0100380700ff\",\"description\": \"Wirkleistung L2\",\"value\": " + String(Wirkleistung_L2) + " ,\"unit\": \"WATT\"},{\"obis\": \"01002b0700ff\",\"description\": \"Blindleistung L2\",\"value\": " + String(Blindleistung_L2) + " ,\"unit\": \"VAR\"},{\"obis\": \"0100330700ff\",\"description\": \"Strom L2\",\"value\": " + String(Strom_L2) + " ,\"unit\": \"AMPERE\"},{\"obis\": \"0100340700ff\",\"description\": \"Spannung L2\",\"value\":" + String(Spannung_L2) + " ,\"unit\": \"VOLT\"},{\"obis\": \"010051070fff\",\"description\": \"Phasenanweichung Strom/Spannung L2\",\"value\": " + String(PhasenabweichungStromSpannung_L2) + " ,\"unit\": \"DEGREE\"},{\"obis\": \"01004c0700ff\",\"description\": \"Wirkleistung L3\",\"value\": " + String(Wirkleistung_L3) + " ,\"unit\": \"WATT\"},{\"obis\": \"01003f0700ff\",\"description\": \"Blindleistung L3\",\"value\": " + String(Blindleistung_L3) + " ,\"unit\": \"VAR\"},{\"obis\": \"0100470700ff\",\"description\": \"Strom L3\",\"value\": " + String(Strom_L3) + " ,\"unit\": \"AMPERE\"},{\"obis\": \"0100480700ff\",\"description\": \"Spannung L3\",\"value\":" + String(Spannung_L3) + " ,\"unit\": \"VOLT\"},{\"obis\": \"010051071aff\",\"description\": \"Phasenanweichung Strom/Spannung L3\",\"value\": " + String(PhasenabweichungStromSpannung_L3) + " ,\"unit\": \"DEGREE\"},{\"obis\": \"0100510701ff\",\"description\": \"Phasenanweichung Spannungen L1/L2\",\"value\": " + String(PhasenabweichungSpannungen_L1L2) + " ,\"unit\": \"DEGREE\"},{\"obis\": \"0100510702ff\",\"description\": \"Phasenanweichung Spannungen L1/L3\",\"value\": " + String(PhasenabweichungSpannungen_L1L3) + " ,\"unit\": \"DEGREE\"},{\"obis\": \"010060320303\",\"description\": \"Spannungsminimum\",\"value\": " + String(Spannung_Min) + " ,\"unit\": \"VOLT\"},{\"obis\": \"010060320304\",\"description\": \"Spannungsmaximum\",\"value\": " + String(Spannung_Max) + " ,\"unit\": \"VOLT\"}],\"manufacturer\": \"" + String(manufacturer) + "\",\"smlPayload\": \"" + String(eHZ_Message) + "\"}";

//AUSGABE
                    Serial.println(httpRequestData);
                    int httpResponseCode = http.POST(httpRequestData);

//AUSGABE
                    Serial.print("HTTP Response code: ");
                    Serial.println(httpResponseCode);

                    http.end();
                }
                Werte_Empfangen = 0;
            }
            else
            {
                Age_Wirkleistung++; // Alter der Daten erhoehen
                Age_Wirkleistung_L1++;
                Age_Wirkleistung_L2++;
                Age_Wirkleistung_L3++;
                Age_Spannung_L1++;
                Age_Spannung_L2++;
                Age_Spannung_L3++;
                Age_Strom_L1++;
                Age_Strom_L2++;
                Age_Strom_L3++;
                Age_Wirkenergie++;
                Age_WirkenergieT1++;
                Age_WirkenergieT2++;
                Age_Zaehlernummer++;
                Age_Spannung_Max++;
                Age_Spannung_Min++;
                Age_Chiptemp++;

                Age_BlindleistungL1++;
                Age_BlindleistungL2++;
                Age_BlindleistungL3++;
                Age_PhasenabweichungStromSpannung_L1++;
                Age_PhasenabweichungStromSpannung_L2++;
                Age_PhasenabweichungStromSpannung_L3++;
                Age_PhasenabweichungSpannungen_L1L2++;
                Age_PhasenabweichungSpannungen_L1L3++;
                Age_Chiptemp_Min++;
                Age_Chiptemp_Max++;
                Age_Chiptemp_Avg++;

                if ((Werte_Empfangen == 0) && ((DEBUG) < 0x8000))
                {                                // keine Werte erkannt, dann wenigstens die Rohdaten ausgeben
                    Serial.println(eHZ_Message); // eHZ-Message ausgeben
                    Serial.println("");          // Trennzeile
                }
            }
            strcpy(eHZ_Message, ""); // Eingelesene Daten nun loeschen, auf neue warten

            if(WiFi.status() != WL_CONNECTED){
                connection_counter++;
                Serial.println(connection_counter);
                if(connection_counter>1000000){
                    ESP.restart();
                }
            } else {
                connection_counter = 0;
            }
        }
        else
        {
            Escape = 0;                             // neue Daten...
            sprintf(eHZ_zeichen, "%02X", eHZ_data); // gelesene Daten in HEX umwandeln
            strcat(eHZ_Message, eHZ_zeichen);       // und der empfangenen Message hinzufuegen

            if(WiFi.status() != WL_CONNECTED){
                connection_counter++;
                Serial.println(connection_counter);
                if(connection_counter>1000000){
                    ESP.restart();
                }
            } else {
                connection_counter = 0;
            }
        }
    } //if serial input is available+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    if(WiFi.status() != WL_CONNECTED){
        connection_counter++;
        Serial.println(connection_counter);
        if(connection_counter>1000000){
            ESP.restart();
        }
    } else {
        connection_counter = 0;
    }
}

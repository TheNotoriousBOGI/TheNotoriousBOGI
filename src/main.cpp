// ########################################################################
// ###                                                                  ###
// ###   Reading software for HAGER EHZ363, EHZ163 and similar devices  ###
// ###                                                                  ###
// ###   Firmware-Version: 20220214                                     ###
// ###                                                                  ###
// ###                                                                  ###
// ########################################################################


//#define SERIALPRINT             //uncomment if serial output is wished

//custom headers (in include-folder)
#include "obis.h"
#include "webportal.h"
#include "timestamp.h"
#include "deviceinfo.h"
#include "filefunctions.h"
#include "sml.h"

//sml analysis helpers
char printstring[MSIZE];         // string buffer for console output
char Daten_Substring[MSIZE];     // substring serial number

//counters
int i = 0;
int connection_counter = 0;

//define pins for serial input and output
#define RXD2 16
#define TXD2 17


//SETUP(START)############################################################################################################################
void setup()
{
    //start SPIFFS
    SPIFFS.begin(true);

    //create logfile
    writeFile(SPIFFS,"/log.txt","LOG\n");
 
    //open serial connection (console output)
    Serial.begin(115200);

    //open optical serial connection
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

    #ifdef SERIALPRINT
        //print optical interface info
        Serial.println("Serielle Schnittstelle 2 (eHZ):");
        Serial.println("Serial Txd is on pin: " + String(TXD2));
        Serial.println("Serial Rxd is on pin: " + String(RXD2));
        Serial.println(" ");
    #endif

    //display device info on config page
    WiFiSettings.hostname = device_ID;
    WiFiSettings.heading("Sensor ID:");
    WiFiSettings.heading(device_ID);

    //Define custom settings saved by WifiSettings
    //these will return the default if nothing was set before
    //bool checkbox_useconfig_dummy = WiFiSettings.checkbox("Use config file");
    String Server_dummy = WiFiSettings.string("Server address", "default.example.org:443");
    bool checkbox_server_dummy = WiFiSettings.checkbox("Server", true);
    String Apikey_dummy = WiFiSettings.string("API key", "dm38qd0ajd3803e0jd320asfafveafeafafa");
    bool checkbox_apikey_dummy = WiFiSettings.checkbox("APIkey");
    String Authorization_dummy = WiFiSettings.string("Authorization Bearer", "Ndufqa9d3qh8d320fh4379fh438904hf8430f9h4herf947fgPUEFF");
    bool checkbox_authorization_dummy = WiFiSettings.checkbox("Bearer");
    String Server_dummy_meta = WiFiSettings.string("Metadata Server address", "default.example.org:443");
    bool checkbox_meta_dummy = WiFiSettings.checkbox("Medadata transfer");

    //reset connection counter
    connection_counter = 0;

    // try to connect to WiFi with a timeout of 30 seconds - launch portal if connection fails
    WiFiSettings.connect(true, 30);
    
    //checkbox_useconfig = checkbox_useconfig_dummy;
    Serveraddress   = Server_dummy;
    Serveraddress_m = Server_dummy_meta;
    Apikey          = Apikey_dummy;
    Authorization   = Authorization_dummy;
    Bearer         += Authorization;
    checkbox_server = checkbox_server_dummy;
    checkbox_apikey = checkbox_apikey_dummy;
    checkbox_meta   = checkbox_meta_dummy;
    checkbox_authorization = checkbox_authorization_dummy;

    //get time
    configTime(0, 0, ntpServer);

    //start update-functionality
    AsyncElegantOTA.begin(&server);
    server.begin();

    #ifdef SERIALPRINT
        Serial.println("HTTP update server started");
    #endif

    //set initial timestamp for calculation of uptime
    initial_timestamp = getTimestamp();

    //start SD card
    if(!SD.begin()){
        #ifdef SERIALPRINT
            Serial.println("Card Mount Failed");
        #endif
        return;
    }

    uint8_t cardType = SD.cardType();

    #ifdef SERIALPRINT
        Serial.print("SD Card Type: ");
        if(cardType == CARD_NONE){
            Serial.println("No SD card attached");
            return;
        } else if(cardType == CARD_MMC){
            Serial.println("MMC");
        } else if(cardType == CARD_SD){
            Serial.println("SDSC");
        } else if(cardType == CARD_SDHC){
            Serial.println("SDHC");
        } else {
            Serial.println("UNKNOWN");
        }
    #endif

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);

    #ifdef SERIALPRINT
        Serial.printf("SD Card Size: %lluMB\n", cardSize);
    #endif

    writeFile(SD, "/backlog.txt","{");
}


//MAIN######################################################################################################################################################################
void loop()
{
    //run update server
    AsyncElegantOTA.loop();

    if (Serial2.available())
    {
        //read input from optical input as integer
        eHZ_data = Serial2.read();

        //check for escape sequence in SML protocol 0x1b 0x1b 0x1b 0x1b
        if (eHZ_data == 27)
        { 
            //Escape-sequence four times 0x1b
            Escape_Zaehler++;
        }
        else
        {
            Escape_Zaehler = 0;
        }

        //if escape sequence is complete => evaluate data
        if (Escape_Zaehler == 4)
        {
            Escape = 1; // data telegram complete, start evaluation
                        // ===================================================================
                        // ==
                        // == Ouptut in the section Werte_Empfangen == 1...
                        // ==
                        // ===================================================================

            // evaluation
            Zaehlvariable++; // loop counter

            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Serial number of eHZ
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);        // pointer to beginning of sml-message
            B_Daten = strstr(eHZ_Message, KENN_SERIENNUMMER); // search serial number obis

            Offset = B_Daten - Beginn; // calculate offset
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_SERIENNUMMER; i++)
                { //copy data of this thread
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_SERIENNUMMER];
                }
                Daten_Substring[LENGHT_SERIENNUMMER] = '\0';

                Zaehlernummer = 0; //evaluate data of the thread
                Zaehlernummer = 16 * Zaehlernummer + Chr_2_Number(Daten_Substring[4]);
                Zaehlernummer = 16 * Zaehlernummer + Chr_2_Number(Daten_Substring[5]);
                Zaehlernummer = 16 * Zaehlernummer + Chr_2_Number(Daten_Substring[6]);
                Zaehlernummer = 16 * Zaehlernummer + Chr_2_Number(Daten_Substring[7]);
                Zaehlernummer = 16 * Zaehlernummer + Chr_2_Number(Daten_Substring[8]);
                Zaehlernummer = 16 * Zaehlernummer + Chr_2_Number(Daten_Substring[9]);
                Zaehlernummer = 16 * Zaehlernummer + Chr_2_Number(Daten_Substring[10]);
                Zaehlernummer = 16 * Zaehlernummer + Chr_2_Number(Daten_Substring[11]);

                Age_Zaehlernummer = 0; // most recent data
                Werte_Empfangen = 1;   // trigger output
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Server-ID
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);
            B_Daten = strstr(eHZ_Message, KENN_SERVERID);

            Offset = B_Daten - Beginn; // Calculate offset
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_SERVERID; i++)
                {
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Wirkenergie
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);       // Pointer to beginning of sml message
            B_Daten = strstr(eHZ_Message, KENN_WIRKENERGIE); // search Wirkenergie-Kennung

            Offset = B_Daten - Beginn; // Calculate Offset
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_WIRKENERGIE; i++)
                {
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_WIRKENERGIE];
                }
                Daten_Substring[LENGHT_WIRKENERGIE] = '\0';

                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                Wirkenergie = 0.0;
                switch (TL_Flag)
                { 
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

                Wirkenergie = Wirkenergie / 10.0; // Einheit Wh, 1 Nachkommastelle
                Age_Wirkenergie = 0;
                Werte_Empfangen = 1;              // trigger output
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   WirkenergieT1
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            Beginn = strstr(eHZ_Message, eHZ_Message);         // pointer to beginning of sml-message
            B_Daten = strstr(eHZ_Message, KENN_WIRKENERGIET1); // search WirkenergieT1-Kennung

            Offset = B_Daten - Beginn;
            if (Offset > 0)
            {
                for (i = 0; i < LENGHT_WIRKENERGIET1; i++)
                {
                    Daten_Substring[i] = eHZ_Message[i + Offset + OFFSET_WIRKENERGIET1];
                }
                Daten_Substring[LENGHT_WIRKENERGIET1] = '\0';

                TL_Flag = 16 * Chr_2_Number(Daten_Substring[0]) + Chr_2_Number(Daten_Substring[1]); // Laenge der Daten erkennen
                                                                                                    // 0x52 =>  8 bit
                                                                                                    // 0x53 => 16 bit
                                                                                                    // 0x54 => 24 bit (Irrsinn!!!)
                                                                                                    // ...
                                                                                                    // 0x59 => 64 bit
                TL_Offset = 0;                                                                      // Offset um dynamisches Lesen der Leistung zu ermöglichen

                WirkenergieT1 = 0.0; 
                switch (TL_Flag)
                {
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

                WirkenergieT1 = WirkenergieT1 / 10.0; // Einheit Wh, 1 Nachkommastelle
                Age_WirkenergieT1 = 0;                // ganz aktuelle Daten
                Werte_Empfangen = 1;                  // Ausgabe anregen
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   WirkenergieT2
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

                WirkenergieT2 = WirkenergieT2 / 10.0; // Einheit Wh, 1 Nachkommastelle
                Age_WirkenergieT2 = 0;                // ganz aktuelle Daten
                Werte_Empfangen = 1;                  // Ausgabe anregen
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Wirkleistung
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Wirkleistung L1
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Blindleistung L1
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Strom L1
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Spannung L1
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Phasenabweichung Strom/Spannung L1
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Wirkleistung L2
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Blindleistung L2 
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Strom L2 
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Spannung L2 
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Phasenabweichung Strom/Spannung L2 
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Wirkleistung L3 
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Blindleistung L3 
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Strom L3 
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Spannung L3 
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Phasenabweichung Strom/Spannung L3 
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Phasenabweichung Spannungen L1/L2 
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Phasenabweichung Spannungen L1/L3 
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Spannung MIN 
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Spannung MAX 
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
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
            }
            // -------------------------------------------------------------------------------------------------------
            // =
            // =   Thread Ending
            // =
            // -------------------------------------------------------------------------------------------------------

            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            // =
            // =   Gesammelte Werte können ab hier verarbeitet werden
            // =
            // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            if (Werte_Empfangen == 1)
            {
                inputSML++;
                timestamp = getTimestamp();

                //Werte an Server senden
                if (timestamp != 0)
                {
                    HTTPClient http;

                    http.begin(Serveraddress+Serveraddress_SML);
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

#ifdef SERIALPRINT
                    Serial.println("-----------------------------------------------");
                    Serial.println("-----------------------------------------------");
                    Serial.println("-----------------------------------------------");
                    Serial.println("-----------------------------------------------");
                    Serial.println("-----------------------------------------------");
                    Serial.println("SML Message:");
                    Serial.println(httpRequestData);
#endif
                    //PUT JSON-STRING
                    int httpResponseCode = http.PUT(httpRequestData);

                    //count send attempts
                    sendAttempts++;

                    //count successful sends and backlog
                    if(httpResponseCode == 200){
                        sendAttemptsSuccessful++;
                    } else if(httpResponseCode == 201){
                        sendAttemptsSuccessful++;
                    } else{
                        sendAttemptsFailed++;
                        appendFile(SD, "/backlog.txt", httpRequestData.c_str());
                        appendFile(SD, "/backlog.txt", ",");
                        backlog++;
                    }

                    
                    if(httpResponseCode == 200 && sendAttemptsFailed > 0){
                        appendFile(SD, "/backlogg.txt", "}");
                        readFile(SD, "/backlog.txt");
                        File file = SD.open("/backlog.txt");
                        String sml_array = file.readString();
                    }


#ifdef SERIALPRINT
                    Serial.println("HTTP Response code: ");
                    Serial.println(httpResponseCode);
#endif

                    http.end();
                }

                if (checkbox_meta && sendAttempts%1000 == 0)
                {

                    //Metadaten an Server senden
                    if (timestamp != 0)
                    {
                    HTTPClient http;
                    
                    http.begin(Serveraddress_m+Serveraddress_META);
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

                    upTime = (timestamp - initial_timestamp)*1000;
                    String metadataMessage = "{\"serverID\": \"" + String(ServerID_string_formatted) + "\",\"timestamp\":" + String(timestamp) + "000,\"identification\":" + String(ServerID_string) + ",\"manufacturer\": \"" + String(manufacturer) +"\", \"metadata\": [{\"id\": \"identification\",\"description\": \"Individualkennung des Geräts\", \"value\":" + String(device_ID) + " , \"unit\": \"Undefined\"},{\"id\": \"upTime\",\"description\": \"wie lange läuft das Gerät\", \"value\": " + String(upTime) + " ,\"unit\": \"MILLISECOND\"},{\"id\": \"inputSML\",\"description\": \"Anzahl empfangener SML-Protokolle\",\"value\": " + String(inputSML) + " ,\"unit\": \"COUNT\"},{\"id\": \"sendAttempts\",\"description\": \"Anzahl Versuche Datensätze zu übertragen\",\"value\": " + String(sendAttempts) + " ,\"unit\": \"COUNT\"},{\"id\": \"sendAttemptsSuccessful\",\"description\": \"Anzahl erfolgreicher Übertragungsversuche\",\"value\": " + String(sendAttemptsSuccessful) + " ,\"unit\": \"COUNT\"},{\"id\": \"sendAttemptsFailed\",\"description\": \"Anzahl gescheiterter Übertragungsversuche\",\"value\": " + String(sendAttemptsFailed) + " ,\"unit\": \"COUNT\"},{\"id\": \"backlog\",\"description\": \"Anzahl noch zu übertragender Datensätze\",\"value\": " + String(backlog) + " ,\"unit\": \"COUNT\"},{\"id\": \"firmwareVersion\",\"description\": \"Aktuell installierte Firmware\",\"value\":" + String(firmwareVersion) + " ,\"unit\": \"Undefined\"},{\"id\": \"freeSpaceSD\",\"description\": \"Speicherplatz auf der SD-Karte\",\"value\": " + String(freeSpaceSD) + " ,\"unit\": \"COUNT\"},{\"id\": \"ramMaxLoad\",\"description\": \"maximale Arbeitsspeicherauslastung\",\"value\": " + String(ramMaxLoad) + " ,\"unit\": \"COUNT\"},{\"id\": \"signalStrengthWifi\",\"description\": \"aktuelle Signalstärke des WiFi-Netzes\",\"value\": " + String(signalStrengthWifi) + " ,\"unit\": \"PERCENTAGE\"}]}";

#ifdef SERIALPRINT
                    Serial.println("-----------------------------------------------");
                    Serial.println("-----------------------------------------------");
                    Serial.println("-----------------------------------------------");
                    Serial.println("-----------------------------------------------");
                    Serial.println("-----------------------------------------------");
                    Serial.println("METADATA Message:");
                    Serial.println(metadataMessage);
#endif
                    //PUT metadata message
                    int httpResponseCodeMeta = http.PUT(metadataMessage);

#ifdef SERIALPRINT
                    Serial.println("HTTP Response code: ");
                    Serial.println(httpResponseCodeMeta);
#endif

                    http.end();
                    }
                }

                //reset activation
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
            }
            strcpy(eHZ_Message, ""); // Eingelesene Daten nun loeschen, auf neue warten

            if(WiFi.status() != WL_CONNECTED){
                connection_counter++;
#ifdef SERIALPRINT
                Serial.println(connection_counter);
#endif
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
#ifdef SERIALPRINT
                Serial.println(connection_counter);
#endif
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
#ifdef SERIALPRINT
        Serial.println(connection_counter);
#endif
        if(connection_counter>1000000){
            ESP.restart();
        }
    } else {
        connection_counter = 0;
    }
}
#ifndef SML_HEADER
#define SML_HEADER

char eHZ_zeichen[4];             // ehz data (HEX) without delimiters
int eHZ_data = 0;                // databyte from eHZ RS232
int Escape_Zaehler = 0;          // ESCAPE counter (0x1b) at the end of the message
int Escape = 0;                  // ESCAPE flag
long int Zaehlvariable = 0;      // variable for numeration of loop-runs
long int PrintZaehlvariable = 0; // variable for numeration of printed meter values
int Werte_Empfangen = 0;         // flag for received values (at least one set received)
int TL_Flag;                     // T/L flag of SML protocol
int TL_Offset;                   // helper variable to evaluate data according to T/L
long int Offset = 0;             // offset of search string
char *B_Daten;                   // pointer to data start
char *Beginn;                    // pointer to start of eHZ data

//sml metadata
long int inputSML = 0;
long int sendAttempts = 0;
long int sendAttemptsSuccessful = 0;
long int sendAttemptsFailed = 0;
long int backlog = 0;

//device metadata
long int upTime = 0;
long int freeSpaceSD = 0;
long int ramMaxLoad = 0;
long int signalStrengthWifi = 0;

//measured variables and helpers
unsigned long int Zaehlernummer = 0;
long int Age_Zaehlernummer = 0;
double Wirkleistung = 0;
long int Age_Wirkleistung = 0;
double Wirkenergie = 0;
long int Age_Wirkenergie = 0;
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
double Wirkleistung_L1 = 0;       
long int Age_Wirkleistung_L1 = 0; 
double Wirkleistung_L2 = 0;       
long int Age_Wirkleistung_L2 = 0; 
double Wirkleistung_L3 = 0;       
long int Age_Wirkleistung_L3 = 0; 
double Spannung_L1 = 0;           
long int Age_Spannung_L1 = 0;     
double Spannung_L2 = 0;           
long int Age_Spannung_L2 = 0;     
double Spannung_L3 = 0;           
long int Age_Spannung_L3 = 0;     
double Strom_L1 = 0;              
long int Age_Strom_L1 = 0;        
double Strom_L2 = 0;              
long int Age_Strom_L2 = 0;        
double Strom_L3 = 0;              
long int Age_Strom_L3 = 0;        
double Spannung_Max = 0;          
long int Age_Spannung_Max = 0;    
double Spannung_Min = 0;          
long int Age_Spannung_Min = 0;    
double Chiptemp = 0;              
long int Age_Chiptemp = 0;        
double Chiptemp_Min = 0;
long int Age_Chiptemp_Min = 0;
double Chiptemp_Max = 0;
long int Age_Chiptemp_Max = 0;
double Chiptemp_Avg = 0;
long int Age_Chiptemp_Avg = 0;

#endif
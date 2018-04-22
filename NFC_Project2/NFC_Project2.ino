#include <Wire.h>
#include "Adafruit_RGBLCDShield.h"
#include "Adafruit_PN532.h"

#define PN532_IRQ   (2)
#define PN532_RESET (3)
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

// create the LCD and NFC objects
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
//Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
Adafruit_PN532 nfc(13, 11, 12, 10);
void setup() {

  // initiate NFC and LCD shields
  nfc.begin();
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setBacklight(BLUE);

  // open serial for debugging
  Serial.begin(115200);

  // opening screen
  lcd.print("NFC Read/Writter");
  lcd.setCursor(0,1);
  lcd.print("By ShadowLurker");
  delay(2000);

  // get NFC firmware version data
  uint32_t versiondata = nfc.getFirmwareVersion();

  // error reading card, and show it. Halt system.
  if (! versiondata) {
    lcd.clear();
    lcd.print("Dind't Find PN532 board.");
    lcd.setBacklight(RED);
    while (1); // halt
  }
  
  // display card, firmware, and software version data
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Chip Version");
  lcd.setCursor(0,1);
  lcd.print("PN5");lcd.print((versiondata>>24) & 0xFF, HEX);
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Firmware Version");
  lcd.setCursor(0,1);
  lcd.print((versiondata>>16) & 0xFF, DEC); lcd.print('.'); lcd.print((versiondata>>8) & 0xFF, DEC);
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Software Version");
  lcd.setCursor(0,1);
  lcd.print("0.1");
  delay(2000);
  
  // configure board to read RFID tags
  nfc.SAMConfig();

  // show opening menu
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Select an Option");
  lcd.setCursor(0,1);
  lcd.print("<Read");
  lcd.setCursor(10,1);
  lcd.print("Write>");
    
}

void screenReset() {

  // initiate NFC and LCD shields
  nfc.begin();
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setBacklight(BLUE);

  
  // show opening menu
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Select an Option");
  lcd.setCursor(0,1);
  lcd.print("<Read");
  lcd.setCursor(10,1);
  lcd.print("Write>");
    
}

void loop(void) {

  uint8_t buttons = lcd.readButtons();

  // main menu
  if (buttons) {
    if (buttons & BUTTON_LEFT) {
      readNFCcard();
      delay(2000);
      screenReset();
    }
    if (buttons & BUTTON_RIGHT) {
      writeNFCcard();
    }
  }
}

void readNFCcard() {

  uint8_t buttons = lcd.readButtons();
  
  uint8_t success={0};
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength={0};                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  uint8_t data16[16] = {0};


  // promt for card to be scanned.
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Scan Card");
  lcd.setBacklight(BLUE);

  // tell the user to press select to return to menu loop
  lcd.setCursor(6,1);
  lcd.print("Menu:(sel)");

  // TODO: Get this working
  // if select button pressed return to menu
  if (buttons) {
    if (buttons & BUTTON_SELECT) {
      return;
    }
  }
  
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

    
  if (success) {
    // Display some basic information about the card
    Serial.print("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");

    
    if (uidLength == 4) {
      // We probably have a Mifare Classic card ... 
      Serial.println("Seems to be a Mifare Classic card (4 byte UID)");
    
      // Now we need to try to authenticate it for read/write access
      // Try with the factory default KeyA: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
      Serial.println("Trying to authenticate block 4 with default KEYA value");
      uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    
      // Start with block 4 (the first block of sector 1) since sector 0
      // contains the manufacturer data and it's probably better just
      // to leave it alone unless you know what you're doing
      success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya);
    
      if (success) {
        Serial.println("Sector 1 (Blocks 4..7) has been authenticated");

        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("UID:");
        lcd.setCursor(0,1);
        for(int i = 0; i <sizeof(uid); i++)
        {
          lcd.print(uid[i], HEX);
        }
        lcd.setBacklight(GREEN);
        
    
        // If you want to write something to block 4 to test with, uncomment
        // the following line and this text should be read back in a minute
        //memcpy(data, (const uint8_t[]){ 'a', 'd', 'a', 'f', 'r', 'u', 'i', 't', '.', 'c', 'o', 'm', 0, 0, 0, 0 }, sizeof data);
        // success = nfc.mifareclassic_WriteDataBlock (4, data);

        // Try to read the contents of block 4
        /*success = nfc.mifareclassic_ReadDataBlock(4, data16);
    
        if (success) {
          // Data seems to have been read ... spit it out
          Serial.println("Reading Block 4:");
          nfc.PrintHexChar(data16, 16);
          Serial.println("");
      
          // Wait a bit before reading the card again
          delay(1000);
        }
        else {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Card Denied");
          lcd.setBacklight(RED);
          Serial.println("Ooops ... unable to read the requested block.  Try another key?");
        }*/
      }
      else {
        Serial.println("Ooops ... authentication failed: Try another key?");
        Serial.println("Reading Block 4:");
        nfc.PrintHexChar(data16, 16);
        Serial.println("");

        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("UID:");
        lcd.setCursor(0,1);
        for(int i = 0; i <sizeof(uid); i++)
        {
          lcd.print(uid[i], HEX);
        }
      
          // Wait a bit before reading the card again
          delay(1000);
      }
    }
    
    if (uidLength == 7) {
      // We probably have a Mifare Ultralight card ...
      Serial.println("Seems to be a Mifare Ultralight tag (7 byte UID)");
    
      // Try to read the first general-purpose user page (#4)
      Serial.println("Reading page 4");
      success = nfc.mifareultralight_ReadPage (4, data16);
      if (success) {
        // Data seems to have been read ... spit it out
        nfc.PrintHexChar(data16, 4);
        Serial.println("");
    
        // Wait a bit before reading the card again
        delay(1000);
      }
      else {
        Serial.println("Ooops ... unable to read the requested page!?");
      }
    }
  }

}

void writeNFCcard() {
    /*
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Card to write");
    lcd.setBacklight(BLUE);

    Serial.println("\nPlace an NFC Tag that you want to Record these Messages on!"); // Command for the Serial Monitor
    if (nfc.tagPresent()) {
        NdefMessage message = NdefMessage();
        message.addTextRecord("My First NFC Tag Write"); // Text Message you want to Record
        message.addUriRecord("http://allaboutcircuits.com"); // Website you want to Record
        message.addTextRecord("Way to Go, It Worked!");  // Ednding Message for you to Record
        boolean success = nfc.write(message);
        if (success) {
            Serial.println("Good Job, now read it with your phone!"); // if it works you will see this message 
        } else {
            Serial.println("Write failed"); // If the the rewrite failed you will see this message
        }
    }
    delay(10000);
    */
    
}

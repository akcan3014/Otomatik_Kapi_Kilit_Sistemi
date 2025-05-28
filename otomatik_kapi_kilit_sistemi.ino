#include <Wire.h>
#include <Adafruit_PN532.h>

#define LOCK_PIN 6            
#define UNLOCK_DURATION 3000  

#define PN532_IRQ (2)
#define PN532_RESET (3)

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
const uint8_t yetkiliKart1_UID[] = { 0x03, 0x22, 0xCB, 0x26 };  
const uint8_t yetkiliKart1_UID_Length = sizeof(yetkiliKart1_UID);
const uint8_t yetkiliKart2_UID[] = { 0x86, 0x2D, 0xB7, 0x01 };  
const uint8_t yetkiliKart2_UID_Length = sizeof(yetkiliKart2_UID);
struct YetkiliKart {
  const uint8_t* uid;
  const uint8_t length;
};
YetkiliKart yetkiliKartlar[] = {
  { yetkiliKart1_UID, yetkiliKart1_UID_Length },
  { yetkiliKart2_UID, yetkiliKart2_UID_Length }
};
const int yetkiliKartSayisi = sizeof(yetkiliKartlar) / sizeof(yetkiliKartlar[0]);
uint8_t lastReadUID[7];  
uint8_t lastReadUIDLength = 0;
unsigned long lastReadTime = 0;
unsigned long unlockTime = 0;
bool isUnlocked = false;
void setup() {
  Serial.begin(115200);
  pinMode(LOCK_PIN, OUTPUT);
  digitalWrite(LOCK_PIN, LOW);  
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("PN532 modülü bulunamadı");
    while (1)
      ;
  }
  nfc.SAMConfig();
  Serial.println("Yetkili kartları tanımlamak için kartlarınızı okutun:");
  Serial.println("------------------------------------");
  Serial.println("Kart okutulmasını bekliyor...");
}
void loop() {
  uint8_t uid[7];
  uint8_t uidLength;
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 50)) {
    if (!isUnlocked) {  
      bool sameAsLastRead = true;
      if (uidLength != lastReadUIDLength) {
        sameAsLastRead = false;
      } else {
        for (uint8_t i = 0; i < uidLength; i++) {
          if (uid[i] != lastReadUID[i]) {
            sameAsLastRead = false;
            break;
          }
        }
      }

      if (!sameAsLastRead || (millis() - lastReadTime > 3000)) {
        Serial.print("Kart okundu. UID: ");
        printUID(uid, uidLength);

        if (isCardAuthorized(uid, uidLength)) {
          Serial.println("Yetkili kart! Kilit açılıyor...");
          unlockLock();  
        } else {
          Serial.println("Geçiş hakkı yok!");
        }
        memcpy(lastReadUID, uid, uidLength);
        lastReadUIDLength = uidLength;
        lastReadTime = millis();
      }
    } else {
    }
  }
  if (isUnlocked && (millis() - unlockTime >= UNLOCK_DURATION)) {
    Serial.println("DEBUG: Kapatma koşulu sağlandı. Kilit kapatılıyor.");
    lockLock();
  }
}
bool isCardAuthorized(uint8_t* uid, uint8_t uidLength) {
  for (int i = 0; i < yetkiliKartSayisi; i++) {
    if (uidLength == yetkiliKartlar[i].length) {  
      bool match = true;
      for (uint8_t j = 0; j < uidLength; j++) {
        if (uid[j] != yetkiliKartlar[i].uid[j]) {
          match = false;
          break;
        }
      }
      if (match) {
        return true; 
      }
    }
  }
  return false;  
}
void unlockLock() {
  digitalWrite(LOCK_PIN, HIGH);  
  Serial.println("Kilit açıldı!");
  unlockTime = millis();  
  isUnlocked = true;
}
void lockLock() {
  digitalWrite(LOCK_PIN, LOW);  
  Serial.println("Kilit kapandı.");
  isUnlocked = false;
}
void printUID(uint8_t* uid, uint8_t length) {
  for (uint8_t i = 0; i < length; i++) {
    Serial.print(uid[i] < 0x10 ? "0" : "");  
    Serial.print(uid[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}
/*
 * RFID-RC522 MIFARE Classic 1K Cloner v6
 * Auth + Read in single pass. Card removal aborts immediately.
 */

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN  10
#define RST_PIN  9

MFRC522 mfrc522(SS_PIN, RST_PIN);

const byte commonKeys[][6] = {
  {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},{0xA0,0xA1,0xA2,0xA3,0xA4,0xA5},
  {0xB0,0xB1,0xB2,0xB3,0xB4,0xB5},{0x4D,0x3A,0x99,0xC3,0x51,0xDD},
  {0x1A,0x98,0x2C,0x7E,0x45,0x9A},{0x00,0x00,0x00,0x00,0x00,0x00},
  {0xD3,0xF7,0xD3,0xF7,0xD3,0xF7},{0xAA,0xBB,0xCC,0xDD,0xEE,0xFF},
  {0x71,0x4C,0x5C,0x88,0x6E,0x97},{0x58,0x7E,0xE5,0xF9,0x35,0x0F},
  {0xA0,0x47,0x8C,0xC3,0x90,0x91},{0x53,0x3C,0xB6,0xC7,0x23,0xF6},
  {0x8F,0xD0,0xA4,0xF2,0x56,0xE9},{0x4F,0x0E,0x5C,0xD1,0x47,0x5B},
  {0x2A,0xA0,0x5E,0xD1,0x85,0x6F},{0xAB,0xCD,0xEF,0x12,0x34,0x56},
  {0x12,0x34,0x56,0x78,0x9A,0xBC},{0xFE,0xDC,0xBA,0x98,0x76,0x54},
  {0x00,0x11,0x22,0x33,0x44,0x55},{0xAA,0xAA,0xAA,0xAA,0xAA,0xAA},
  {0xBB,0xBB,0xBB,0xBB,0xBB,0xBB},{0x0F,0x0F,0x0F,0x0F,0x0F,0x0F},
  {0xF0,0xF0,0xF0,0xF0,0xF0,0xF0},{0x19,0x84,0x07,0x15,0x76,0xFE},
  {0xE8,0x68,0x52,0xD8,0xD2,0xA0},{0xDE,0xAD,0xBE,0xEF,0xDE,0xAD},
  {0x48,0x45,0x4C,0x4C,0x4F,0x21},{0x43,0x41,0x46,0x45,0x42,0x41},
  {0x85,0xE3,0x5D,0x2F,0x74,0xDC},{0x57,0x0B,0xDE,0x11,0x3F,0x2E},
  {0xE3,0x51,0x73,0x49,0x4A,0x81},{0xCB,0x9D,0xF2,0xA5,0x17,0x79},
  {0x12,0x84,0xE6,0x1C,0x59,0xF3},{0x45,0x04,0x29,0xE3,0xC0,0x9D},
  {0xE2,0x9C,0xA8,0x4F,0x1D,0xB9},{0x0F,0xF5,0x37,0x72,0x21,0x7E},
  {0x63,0xA8,0x45,0xF0,0xE2,0x7C},{0x5A,0xCF,0x55,0xC1,0x3B,0x36},
  {0x95,0x92,0x7D,0xFF,0x69,0x25},{0x85,0x3E,0x80,0x31,0x26,0x2A},
  {0xC4,0xD5,0x1A,0x4A,0xD4,0x71},{0x6B,0x65,0x54,0xAE,0xD0,0xAC},
  {0x65,0xB2,0xB8,0x14,0x31,0xA5},{0xD2,0xFC,0x15,0xF1,0xAF,0xA1},
  {0x94,0x0E,0xCE,0x88,0x82,0x6C},{0x47,0x92,0x1C,0x48,0x01,0xF5},
  {0x63,0xE8,0xAC,0x45,0x67,0x3C},{0x6A,0x47,0x0D,0x54,0x12,0x7D},
  {0x20,0x0F,0x79,0x40,0x63,0x24},{0x3A,0x42,0xB3,0x3D,0x66,0x67},
};
const byte KEY_COUNT = sizeof(commonKeys) / 6;

byte dumpData[16][4][16];
MFRC522::MIFARE_Key dumpKeys[16];
bool dumpSectorOK[16];
byte dumpSectorCount, dumpUID[10], dumpUIDSize;
MFRC522::PICC_Type dumpType;
bool dumpValid, oddCard = true;

byte getTrailer(byte sec) { return (sec < 32) ? (sec*4+3) : (128+(sec-32)*16+15); }
byte getFirst(byte sec)   { return (sec < 32) ? (sec*4)   : (128+(sec-32)*16); }
byte getNum(byte sec)     { return (sec < 32) ? 4 : 16; }

bool cardGone() { return !mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial(); }

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  byte v = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.print(F("\n=== Cloner v6 | RC522 0x"));
  Serial.print(v, HEX); Serial.print(F(" | ")); Serial.print(KEY_COUNT);
  Serial.println(F(" keys\n[1] PLACE SOURCE CARD..."));
}

// ===================== DUMP SOURCE CARD =====================
void dumpCard(MFRC522::PICC_Type pt) {
  Serial.print(F("\n=== DUMP | "));
  Serial.print(mfrc522.PICC_GetTypeName(pt));
  Serial.print(F(" | UID: "));
  dumpUIDSize = mfrc522.uid.size;
  memcpy(dumpUID, mfrc522.uid.uidByte, dumpUIDSize);
  dumpType = pt;
  for (byte i = 0; i < dumpUIDSize; i++) {
    if (dumpUID[i] < 0x10) Serial.print("0");
    Serial.print(dumpUID[i], HEX); Serial.print(" ");
  }
  Serial.println();

  dumpSectorCount = (pt == MFRC522::PICC_TYPE_MIFARE_1K)  ? 16 :
                    (pt == MFRC522::PICC_TYPE_MIFARE_4K)  ? 40 :
                    (pt == MFRC522::PICC_TYPE_MIFARE_MINI) ? 5  : 16;
  byte found = 0, blocks = 0;
  memset(dumpSectorOK, 0, sizeof(dumpSectorOK));
  memset(dumpData, 0, sizeof(dumpData));

  // Auth + Read 在同一个循环 (v2 模式)
  Serial.println(F("Sec | Key               | Status"));
  Serial.println(F("----|-------------------|------"));
  for (byte s = 0; s < dumpSectorCount; s++) {
    byte trailer = getTrailer(s);
    bool authed = false;

    // Brute-force KeyA
    for (byte k = 0; k < KEY_COUNT && !authed; k++) {
      memcpy(dumpKeys[s].keyByte, commonKeys[k], 6);
      if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
          trailer, &dumpKeys[s], &(mfrc522.uid)) == MFRC522::STATUS_OK) authed = true;
    }
    // Brute-force KeyB
    for (byte k = 0; k < KEY_COUNT && !authed; k++) {
      memcpy(dumpKeys[s].keyByte, commonKeys[k], 6);
      if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B,
          trailer, &dumpKeys[s], &(mfrc522.uid)) == MFRC522::STATUS_OK) authed = true;
    }

    Serial.print(F("  "));
    Serial.print(s/10); Serial.print(s%10);
    Serial.print(F("  | "));

    if (authed) {
      dumpSectorOK[s] = true; found++;
      for (byte i = 0; i < 6; i++) {
        if (dumpKeys[s].keyByte[i] < 0x10) Serial.print("0");
        Serial.print(dumpKeys[s].keyByte[i], HEX);
      }
      Serial.print(F(" | OK"));

      // 立即读数据
      byte fb = getFirst(s), nb = getNum(s);
      for (byte b = 0; b < nb; b++) {
        byte buf[18], sz = 18;
        if (mfrc522.MIFARE_Read(fb+b, buf, &sz) == MFRC522::STATUS_OK) {
          memcpy(dumpData[s][b], buf, 16); blocks++;
        }
      }
      Serial.print(F(" ("));
      Serial.print(blocks);
      Serial.println(F(" blk)"));
    } else {
      memset(dumpKeys[s].keyByte, 0, 6);
      Serial.println(F("LOCKED          | --"));
    }

    if (cardGone()) { Serial.println(F("\n** CARD REMOVED - ABORT **")); goto done; }
  }

  // 第 2 轮: 用已知密钥尝试未知扇区
  if (found > 0 && found < dumpSectorCount) {
    Serial.println(F("\nR2: Propagation..."));
    for (byte s = 0; s < dumpSectorCount; s++) {
      if (dumpSectorOK[s]) continue;
      byte trailer = getTrailer(s);
      bool ok = false;
      for (byte ks = 0; ks < dumpSectorCount && !ok; ks++) {
        if (!dumpSectorOK[ks]) continue;
        MFRC522::MIFARE_Key tk;
        memcpy(tk.keyByte, dumpKeys[ks].keyByte, 6);
        if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
            trailer, &tk, &(mfrc522.uid)) == MFRC522::STATUS_OK) ok = true;
        if (!ok && mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B,
            trailer, &tk, &(mfrc522.uid)) == MFRC522::STATUS_OK) ok = true;
      }
      if (ok) {
        dumpSectorOK[s] = true; found++;
        memcpy(dumpKeys[s].keyByte, dumpKeys[s].keyByte, 6); // will be fixed below
        // Actually need to store the found key. Let me re-auth and read.
        for (byte ks = 0; ks < dumpSectorCount; ks++) {
          if (!dumpSectorOK[ks]) continue;
          MFRC522::MIFARE_Key tk;
          memcpy(tk.keyByte, dumpKeys[ks].keyByte, 6);
          if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
              trailer, &tk, &(mfrc522.uid)) == MFRC522::STATUS_OK) {
            memcpy(dumpKeys[s].keyByte, tk.keyByte, 6); break;
          }
          if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B,
              trailer, &tk, &(mfrc522.uid)) == MFRC522::STATUS_OK) {
            memcpy(dumpKeys[s].keyByte, tk.keyByte, 6); break;
          }
        }
        byte fb = getFirst(s), nb = getNum(s);
        for (byte b = 0; b < nb; b++) {
          byte buf[18], sz = 18;
          if (mfrc522.MIFARE_Read(fb+b, buf, &sz) == MFRC522::STATUS_OK) {
            memcpy(dumpData[s][b], buf, 16); blocks++;
          }
        }
        Serial.print(F("  ")); Serial.print(s/10); Serial.print(s%10);
        Serial.println(F("  | propagated      | OK"));
      }
    }
  }

done:
  mfrc522.PICC_HaltA();
  dumpValid = (found > 0);
  oddCard = false;

  Serial.print(F("\nResult: ")); Serial.print(found); Serial.print(F("/"));
  Serial.print(dumpSectorCount); Serial.print(F(" sectors (")); Serial.print(blocks);
  Serial.println(F(" blk)"));
  if (found < dumpSectorCount) Serial.println(F("DUMP INCOMPLETE"));
  Serial.println(F("[2] PLACE TARGET CARD..."));
}

// ===================== WRITE TARGET CARD =====================
void writeCard(MFRC522::PICC_Type pt) {
  Serial.print(F("\n=== WRITE | UID: "));
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) Serial.print("0");
    Serial.print(mfrc522.uid.uidByte[i], HEX); Serial.print(" ");
  }
  Serial.print(F("| ")); Serial.print(mfrc522.PICC_GetTypeName(pt));
  Serial.println();

  if (!dumpValid) { Serial.println(F("No dump!")); oddCard = true; mfrc522.PICC_HaltA(); return; }

  byte tc = (pt == MFRC522::PICC_TYPE_MIFARE_1K)  ? 16 :
            (pt == MFRC522::PICC_TYPE_MIFARE_4K)  ? 40 :
            (pt == MFRC522::PICC_TYPE_MIFARE_MINI) ? 5  : 16;

  // 破解目标卡密钥
  Serial.println(F("Cracking target keys..."));
  MFRC522::MIFARE_Key targetKeys[16];
  for (byte s = 0; s < tc && s < 16; s++) {
    byte trailer = getTrailer(s);
    bool ok = false;
    for (byte k = 0; k < KEY_COUNT && !ok; k++) {
      memcpy(targetKeys[s].keyByte, commonKeys[k], 6);
      if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
          trailer, &targetKeys[s], &(mfrc522.uid)) == MFRC522::STATUS_OK) ok = true;
      if (!ok && mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B,
          trailer, &targetKeys[s], &(mfrc522.uid)) == MFRC522::STATUS_OK) ok = true;
    }
    if (!ok) memset(targetKeys[s].keyByte, 0, 6);
  }

  // 可写性检测
  bool writable = false;
  for (byte k = 0; k < KEY_COUNT; k++) {
    MFRC522::MIFARE_Key k0;
    memcpy(k0.keyByte, commonKeys[k], 6);
    if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
        getTrailer(0), &k0, &(mfrc522.uid)) == MFRC522::STATUS_OK) {
      byte orig[18], sz = 18;
      byte tst[16] = {0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,0,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99};
      if (mfrc522.MIFARE_Read(0, orig, &sz) == MFRC522::STATUS_OK) {
        mfrc522.MIFARE_Write(0, tst, 16);
        sz = 18; byte vrf[18];
        if (mfrc522.MIFARE_Read(0, vrf, &sz) == MFRC522::STATUS_OK)
          writable = (memcmp(vrf, tst, 16) == 0);
        if (writable) mfrc522.MIFARE_Write(0, orig, 16);
      }
      break;
    }
  }
  Serial.println(writable ? F("Magic card!") : F("Standard card"));

  // 写入
  byte w = 0;
  byte maxSec = min(dumpSectorCount, tc);
  for (byte s = 0; s < maxSec && s < 16; s++) {
    if (!dumpSectorOK[s] || targetKeys[s].keyByte[0] == 0) continue;
    byte trailer = getTrailer(s);
    if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
        trailer, &targetKeys[s], &(mfrc522.uid)) != MFRC522::STATUS_OK) continue;
    byte fb = getFirst(s), nb = getNum(s);
    byte bs = (s == 0 && !writable) ? 1 : 0;
    for (byte b = bs; b < nb; b++)
      if (mfrc522.MIFARE_Write(fb+b, dumpData[s][b], 16) == MFRC522::STATUS_OK) w++;
    if (cardGone()) { Serial.println(F("Card removed - abort")); break; }
  }
  mfrc522.PICC_HaltA();
  oddCard = true;
  Serial.print(F("Done: ")); Serial.print(w); Serial.println(F(" blk\n[1] PLACE NEXT SOURCE CARD..."));
}

// ===================== LOOP =====================
void loop() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

  MFRC522::PICC_Type pt = mfrc522.PICC_GetType(mfrc522.uid.sak);
  if (pt != MFRC522::PICC_TYPE_MIFARE_1K && pt != MFRC522::PICC_TYPE_MIFARE_4K
      && pt != MFRC522::PICC_TYPE_MIFARE_MINI) {
    Serial.print(F("Skip: ")); Serial.println(mfrc522.PICC_GetTypeName(pt));
    mfrc522.PICC_HaltA(); delay(2000); return;
  }

  if (oddCard) dumpCard(pt);
  else writeCard(pt);

  delay(1500);
  mfrc522.PCD_Init();
}

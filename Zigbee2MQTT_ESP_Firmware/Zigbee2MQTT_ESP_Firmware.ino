#define   BAUD_RATE   115200
#define   RX_PIN      13      // Connected to P0_3 CC2530 TX_PIN
#define   TX_PIN      15      // Connected to P0_2 CC2530 RX_PIN

#define   CC2530_RESET_PIN    2   // Connected to CC2530 RESET_N
#define   CC2530_CT_PIN       12  // Connected to CC2530 P0_4
#define   CC2530_RT_PIN       14  // Connected to CC2530 P0_5
#define   CC2530_CFG1_PIN     10  // Connected to CC2530 P2_0

uint8_t calcFCS(uint8_t *pMsg, uint8_t len);
void serialSwap(unsigned long ms);

uint8_t incomingByte = 0;
uint8_t receivedPackets[255];
uint8_t packetIndex = 0;

uint8_t max_len = 255;

uint8_t fcs;
uint8_t flag = 0;

void setup() 
{
  pinMode(CC2530_RESET_PIN, OUTPUT);
  pinMode(CC2530_CT_PIN, OUTPUT);
  pinMode(CC2530_RT_PIN, INPUT_PULLUP);
  pinMode(CC2530_CFG1_PIN, OUTPUT);

  // Initialize Serial for communication
  Serial.begin(BAUD_RATE);
  while (!Serial);  // Wait for Serial to initialize

  Serial.println("Serial Successfully Initialized!");
  Serial.println("Swap Serial to Communicate with CC2530!");

  serialSwap(10);

  // CC2530 Power Up Procedure
  digitalWrite(CC2530_RESET_PIN, LOW);
  digitalWrite(CC2530_CFG1_PIN, LOW);
  digitalWrite(CC2530_CT_PIN, LOW);
  delay(1000);
  digitalWrite(CC2530_RESET_PIN, HIGH);
  delay(5000);

  packetIndex = 0;
  while(1)
  {
    if(Serial.available())
    {
      digitalWrite(CC2530_CT_PIN, HIGH);
      incomingByte = Serial.read();
      if (!((!packetIndex) && (incomingByte != 0xFE)))
      {
        receivedPackets[packetIndex] = incomingByte;
        packetIndex++;
      }
      delay(10);
      digitalWrite(CC2530_CT_PIN, LOW);
    }

    if(packetIndex >= 11)
    {
      digitalWrite(CC2530_CT_PIN, HIGH);
      break;
    }
  }

  serialSwap(10);

  Serial.println();
  for(int i=0; i<packetIndex; i++)
  {
    Serial.print(receivedPackets[i], HEX);
    Serial.print(" ");
  }
  
  serialSwap(10);
  
  uint8_t general[] = {0x00, 0x26, 0x00};
  sendTxPacket(general);

  digitalWrite(CC2530_CT_PIN, LOW);
  packetIndex = 0;
  max_len = 255;

  while(1)
  {
    if(Serial.available())
    {
      digitalWrite(CC2530_CT_PIN, HIGH);
      incomingByte = Serial.read();
      if (!((!packetIndex) && (incomingByte != 0xFE)))
      {
        receivedPackets[packetIndex] = incomingByte;

        if (packetIndex == 1)
        {
          max_len = receivedPackets[1] + 5;
        }

        packetIndex++;
      }
      delay(10);
      digitalWrite(CC2530_CT_PIN, LOW);
    }

    if(packetIndex >= max_len)
    {
      digitalWrite(CC2530_CT_PIN, HIGH);

      serialSwap(10);

      Serial.println();
      for(int i=0; i<packetIndex; i++)
      {
        Serial.print(receivedPackets[i], HEX);
        Serial.print(" ");
      }
      
      serialSwap(10);

      break;
    }
  }

  uint8_t general1[] = {0x00, 0x27, 0x00};
  sendTxPacket(general1);

  digitalWrite(CC2530_CT_PIN, LOW);
  packetIndex = 0;
  max_len = 255;

  while(1)
  {
    if(Serial.available())
    {
      digitalWrite(CC2530_CT_PIN, HIGH);
      incomingByte = Serial.read();
      if (!((!packetIndex) && (incomingByte != 0xFE)))
      {
        receivedPackets[packetIndex] = incomingByte;

        if (packetIndex == 1)
        {
          max_len = receivedPackets[1] + 5;
        }

        packetIndex++;
      }
      delay(10);
      digitalWrite(CC2530_CT_PIN, LOW);
    }

    if(packetIndex >= max_len)
    {
      digitalWrite(CC2530_CT_PIN, HIGH);

      serialSwap(10);

      Serial.println();
      for(int i=0; i<packetIndex; i++)
      {
        Serial.print(receivedPackets[i], HEX);
        Serial.print(" ");
      }
      
      serialSwap(10);

      break;
    }
  }

  uint8_t general2[] = {0x01, 0x25, 0x40, 0x05};
  sendTxPacket(general2);

  digitalWrite(CC2530_CT_PIN, LOW);
  packetIndex = 0;
  max_len = 255;
}

void loop()
{
  if(Serial.available())
  {
    digitalWrite(CC2530_CT_PIN, HIGH);
    incomingByte = Serial.read();
    if (!((!packetIndex) && (incomingByte != 0xFE)))
    {
      receivedPackets[packetIndex] = incomingByte;

      if (packetIndex == 1)
      {
        max_len = receivedPackets[1] + 5;
      }

      packetIndex++;
    }
    delay(10);
    digitalWrite(CC2530_CT_PIN, LOW);
  }

  if(packetIndex >= max_len)
  {
    digitalWrite(CC2530_CT_PIN, HIGH);

    if(receivedPackets[2] == 0x45 && receivedPackets[3] == 0xC0 && receivedPackets[4] == 9)
    {
      flag = 1;
    }

    serialSwap(10);

    Serial.println();
    for(int i=0; i<packetIndex; i++)
    {
      Serial.print(receivedPackets[i], HEX);
      Serial.print(" ");
    }
    
    serialSwap(10);

    digitalWrite(CC2530_CT_PIN, LOW);
    packetIndex = 0;
    max_len = 255;
  }

  if(flag)
  {
    flag = 0;
    uint8_t general3[] = {0x03, 0x26, 0x08, 0xFC, 0xFF, 0xFF};
    sendTxPacket(general3);
  }
}

uint8_t calcFCS(uint8_t *pMsg, uint8_t len) 
{ 
  uint8_t result = 0; 
  while (len--) 
  { 
    result ^= *pMsg++; 
  } 
  
  return result; 
} 

void sendTxPacket(uint8_t *pMsg)
{
  uint8_t txPacketLen = pMsg[0] + 3;
  uint8_t FCS = calcFCS(pMsg, txPacketLen);

  Serial.write(0xFE);

  for(unsigned int i = 0; i < txPacketLen; i++)
    Serial.write(pMsg[i]);

  Serial.write(FCS);
}

void serialSwap(unsigned long ms)
{
  Serial.flush();
  Serial.swap();
  delay(ms);
}
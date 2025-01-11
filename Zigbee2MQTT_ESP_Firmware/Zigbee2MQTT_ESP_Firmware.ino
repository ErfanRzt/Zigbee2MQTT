#include <String.h>

#define   BAUD_RATE   115200
#define   RX_PIN      13      // Connected to P0_3 CC2530 TX_PIN
#define   TX_PIN      15      // Connected to P0_2 CC2530 RX_PIN

#define   CC2530_RESET_PIN    2   // Connected to CC2530 RESET_N
#define   CC2530_CT_PIN       12  // Connected to CC2530 P0_4
#define   CC2530_RT_PIN       14  // Connected to CC2530 P0_5
#define   CC2530_CFG1_PIN     10  // Connected to CC2530 P2_0

uint8_t calcFCS(uint8_t *pMsg, uint8_t len);
void ZNP_ZB_START_REQUEST(void);
void ZNP_UTIL_GET_DEVICE_INFO(void);
void ZNP_ZDO_STARTUP_FROM_APP(void);
void serialMonitorPacket(String log, uint8_t *pMsg, uint8_t len);
void serialMonitorString(String log);
void serialSwap(unsigned long ms);

uint8_t incomingByte = 0;
uint8_t receivedPackets[255];
uint8_t packetIndex = 0;
uint8_t max_len = 255;

uint8_t coord_short_add[2];
uint8_t coord_IEEE_add[8];

uint8_t *connectedDev;

uint8_t flag = 1;
uint8_t flag_req = 0;
uint8_t flag_act = 0;
uint8_t flag_seq = 0;

uint8_t flag_setup = 0;
uint8_t flag_coord = 0;
uint8_t flag_disply_attemp = 1;

unsigned long previousMillis;

void setup() 
{
  pinMode(CC2530_RESET_PIN, OUTPUT);
  pinMode(CC2530_CT_PIN, OUTPUT);
  pinMode(CC2530_RT_PIN, INPUT_PULLUP);
  pinMode(CC2530_CFG1_PIN, OUTPUT);

  // Initialize Serial for communication
  Serial.begin(BAUD_RATE);
  while (!Serial);  // Wait for Serial to initialize

  serialSwap(10);

  // CC2530 Power Up Procedure
  digitalWrite(CC2530_RESET_PIN, LOW);
  digitalWrite(CC2530_CFG1_PIN, LOW);
  digitalWrite(CC2530_CT_PIN, LOW);
  delay(1000);
  digitalWrite(CC2530_RESET_PIN, HIGH);
  delay(5000);

  while(1)
  {
    max_len = receivedPackets[1] + 5;
    if(Serial.available())
    {
      digitalWrite(CC2530_CT_PIN, HIGH);  // NOT Clear to Receive Serial Data

      incomingByte = Serial.read();
      if (!((!packetIndex) && (incomingByte != 0xFE)))
      {
        receivedPackets[packetIndex] = incomingByte;
        packetIndex++;
      }

      delay(10);
      digitalWrite(CC2530_CT_PIN, LOW);
    }
    if(packetIndex >= max_len)
    {
      digitalWrite(CC2530_CT_PIN, HIGH);

      /* STARTUP PROCEDURE */
      if(receivedPackets[0] == 0xFE)
      {
        if(receivedPackets[2] == 0x41 && receivedPackets[3] == 0x80)
        {
          serialMonitorPacket("SYS_RESET_IND: ", receivedPackets, packetIndex);
        }
        else if(receivedPackets[2] == 0x45 && receivedPackets[3] == 0xC0)
        {
          serialMonitorPacket("ZDO_STATE_CHANGE_IND: ", receivedPackets, packetIndex);
          if(receivedPackets[4] == 0x08 && flag_disply_attemp)
          {
            serialMonitorString("Starting as Zigbee Coordinator!");

            flag_coord = 0;
            flag_disply_attemp = 0;
          }
          else if(receivedPackets[4] == 0x09 && (!flag_coord))
          {
            serialMonitorString("Started as Zigbee Coordinator!");

            flag_coord = 1;
            flag_disply_attemp = 1;
          }
          else
          {
            flag_coord = 0;
            flag_disply_attemp = 1;
          }
        }
        else if(receivedPackets[2] == 0x66 && receivedPackets[3] == 0x00)
        {
          serialMonitorPacket("ZB_START_REQUEST: ", receivedPackets, packetIndex);
        }
        else if(receivedPackets[2] == 0x67 && receivedPackets[3] == 0x00)
        {
          if(receivedPackets[4] == 0x00)  // Success
          {
            for(unsigned int i = 0; i < 8; i++)
            {
              coord_IEEE_add[i] = receivedPackets[i + 5];
            }
            for(unsigned int i = 0; i < 2; i++)
            {
              coord_short_add[i] = receivedPackets[i + 13];
            }
            serialMonitorPacket("UTIL_GET_DEVICE_INFO: ", receivedPackets, packetIndex);
            serialMonitorPacket("COORD_IEEE_ADD:  ", coord_IEEE_add, 8);
            serialMonitorPacket("COORD_SHORT_ADD: ", coord_short_add, 2);

            break;
          }
        }
      }

      packetIndex = 0;
      max_len = 255;

      digitalWrite(CC2530_CT_PIN, LOW);
    }

    if(flag_coord)
    {
      if(!flag_setup)
      {
        flag_setup = 1;

        // ZNP_ZB_START_REQUEST();
        ZNP_UTIL_GET_DEVICE_INFO(); 
        // ZNP_ZDO_STARTUP_FROM_APP();
      }
    }
  }

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

    // if(receivedPackets[2] == 0x45 && receivedPackets[3] == 0xC0 && receivedPackets[4] == 9)
    // {
    //   flag = 1;
    // }
    if(receivedPackets[2] == 0x45 && receivedPackets[3] == 0xC1)
    {
      connectedDev = receivedPackets;
      flag_req = 1;
    }
    else if(receivedPackets[2] == 0x65 && receivedPackets[3] == 0x04 && receivedPackets[4] == 0x00)
    {
      flag_act = 1;
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

  if(flag_req)
  {
    flag_req = 0;
    uint8_t general4[] = {0x05, 0x25, 0x04, connectedDev[4], connectedDev[5], connectedDev[6], connectedDev[7], 0xFF}; // ZB_PERMIT_JOINING_REQUEST
    sendTxPacket(general4); // SRSP: 0x01 0x66 0x08 STATUS
  }
  if(flag_act)
  {
    flag_act = 0;
    flag_seq = 1;
    uint8_t general5[] = {0x04, 0x25, 0x05, connectedDev[4], connectedDev[5], connectedDev[6], connectedDev[7]}; // ZB_PERMIT_JOINING_REQUEST
    sendTxPacket(general5); // SRSP: 0x01 0x66 0x08 STATUS
  }
  if(flag_seq)
  {
    flag_seq = 0;
    uint8_t general6[] = {0x09, 0x24, 0x00, 0x01, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // ZB_PERMIT_JOINING_REQUEST
    sendTxPacket(general6); // SRSP: 0x01 0x66 0x08 STATUS
  }
  if(flag)
  {
    flag = 0;
    uint8_t general3[] = {0x03, 0x26, 0x08, 0xFC, 0xFF, 0xFF}; // ZB_PERMIT_JOINING_REQUEST
    sendTxPacket(general3); // SRSP: 0x01 0x66 0x08 STATUS
  }
}

uint8_t calcFCS(uint8_t *pMsg, uint8_t len) 
{ 
  uint8_t fcs = 0; 
  while (len--) 
    fcs ^= *pMsg++; 
  
  return fcs; 
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

void ZNP_ZB_START_REQUEST(void)
{
  uint8_t pMsg[] = {0x00, 0x26, 0x00};
  sendTxPacket(pMsg);
}

void ZNP_UTIL_GET_DEVICE_INFO(void)
{
  uint8_t pMsg[] = {0x00, 0x27, 0x00};
  sendTxPacket(pMsg);
}

void ZNP_ZDO_STARTUP_FROM_APP(void)
{
  uint8_t pMsg[] = {0x01, 0x25, 0x40, 0x05};
  sendTxPacket(pMsg);
}

void serialMonitorString(String log)
{
  serialSwap(10);
  Serial.println(log);
  serialSwap(10);
}

void serialMonitorPacket(String log, uint8_t *pMsg, uint8_t len)
{
  serialSwap(10);

  Serial.println();
  Serial.println(log);
  for(unsigned int i = 0; i < len; i++)
  {
    Serial.print(pMsg[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  serialSwap(10);
}

void serialSwap(unsigned long ms)
{
  Serial.flush();
  Serial.swap();
  delay(ms);
}
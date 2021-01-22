#include <SoftwareSerial.h>
SoftwareSerial Serial1(2, 3);
String systemID = "";
String url = "";
String ip = "\"cloudsocket.hologram.io\"";
int port = 9999;
String SendCmd = "AT+CIPSEND=";
String Start = "AT+CIPSTART=\"TCP\"";
String msg = "";
int i = 0, temp = 0;
int Vo;
float logR2, R2, T, Pr;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
int pumptick = 0;
bool systemrun = false;
bool failure = false;
bool pumpfailure = false;
bool remotelock = true;
int webtick = 0;
void setup() {
  Serial1.begin(9600);
  delay(4000);
  initGSM();
  initGPRS();
  delay(2000);
}

void loop() {
  if (webtick == 5)
  {
    remotelock = SystemCheck();
    webtick = 0;
  }
  else
  {
    webtick++;
  }
  if (remotelock == false)
  {
    Pump(true);
    Vo = analogRead(0);
    Pr = analogRead(1);
    R2 = 10000 * (1023.0 / (float)Vo - 1.0);
    logR2 = log(R2);
    T = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2));
    T = T - 273.15;
    T = (T * 9.0) / 5.0 + 32.0;
    if (T > 200)
    {
      Heat(false);
      failure = true;
      Pump(true);
    }
    if (pumpfailure == false)
    {
      if (failure == false)
      {
        if (T < 100) {
          pumptick = 0;
          if (systemrun == false)
          {
            Pump(true);
            delay(10000);
            Heat(true);
            systemrun = true;
            pumptick = 0;
          }
          if (analogRead(1) < 10)
          {
            failure = true;
            systemrun = false;
            pumpfailure = true;
            Heat(false);
            Pump(false);
          }
        }
        else
        {
          if (systemrun == true)
          {
            Heat(false);
            if (pumptick < 10)
            {
              pumptick++;
            }
            else
            {
              systemrun = false;
            }
          }
        }
      }
      else
      {
        Pump(true);
        Heat(false);
      }
    }
    else
    {
      Pump(false);
      Heat(false);
    }
  }
  delay(1000);

}


void Heat(bool tf)
{
  if (tf == true)
  {
    digitalWrite(2, HIGH);
  }
  else
  {
    digitalWrite(2, LOW);
  }
}
void Pump(bool tf)
{
  if (tf == true)
  {
    digitalWrite(3, HIGH);
  }
  else
  {
    digitalWrite(3, LOW);
  }
}

bool SystemCheck()
{
  String statusmsg = "";
  if (pumpfailure == false)
  {
    if (failure == false)
    {
      if (systemrun == false)
      {
        statusmsg = "0";
      }
      else
      {
        statusmsg = "1";
      }
    }
    else
    {
      statusmsg = "2";
    }
  }
  else
  {
    statusmsg = "3";
  }
  url = GenerateURL("STATUS", statusmsg);
  String svr = Start + "," + ip + "," + port;
  delay(1000);
  connectGSM(svr, "CONNECT");
  delay(1000);
  int len = url.length();
  String str = "";
  str = SendCmd + len;
  sendToServer(str);
  Serial1.print(url);
  delay(1000);
  Serial1.write(0x1A);

  return false;
}

void sendToServer(String str)
{
  Serial1.println(str);
  delay(1000);
}
void initGSM()
{
  connectGSM("AT", "OK");
  connectGSM("ATE1", "OK");
  connectGSM("AT+CPIN?", "READY");
}
void initGPRS()
{
  connectGSM("AT+CIPSHUT", "OK");
  connectGSM("AT+CGATT=1", "OK");
  connectGSM("AT+CSTT=\"hologram\",\"\",\"\"", "OK");
  connectGSM("AT+CIICR", "OK");
  delay(1000);
  Serial1.println("AT+CIFSR");
  delay(1000);
}

String GenerateURL(String topic, String body)
{
  String x = "";
  x = "{\"k\":\"" + systemID;
  x = x + "\",\"d\":\"";
  x = x + body;
  x = x + "\", \"t\":\"";
  x = x + topic;
  x = x + "\"}";
  return x;
}

void connectGSM (String cmd, String data)
{
  while (1)
  {
    Serial.println(cmd);
    Serial1.println(cmd);
    delay(500);
    char res[data.length()+1];
    data.toCharArray(res, data.length()+1);
    while (Serial1.available() > 0)
    {
      if (Serial1.find(res))
      {
        delay(1000);
        return;
      }
    }
    delay(1000);
  }
}

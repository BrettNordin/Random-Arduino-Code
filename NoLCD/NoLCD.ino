#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
int i = 0, temp = 0;
int Vo;
float caladjtemp = 0.00;
float logR2, R2, T, Pr;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
int pumptick = 0;
bool systemrun = false;
bool failure = false;
bool pumpfailure = false;
bool failsafe = false;
int menu = 0;
int lastmenu = 0;
int menumax = 1;
int tick = 0;
byte buttons[] = {10, 11, 12, 13};
const byte nrButtons = 4;
int setpoint = 104;
int inadjmode = false;
int currenttemp = 0;
String stat = "OFF";
int temptemp = 0;
bool failsafelockout = false;
int t;
int firsttime = 0;
byte customChar1[8] = {
  0b00100,
  0b01110,
  0b11111,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100
};
byte customChar2[8] = {
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b11111,
  0b01110,
  0b00100
};
byte customChar3[8] = {
  0b10000,
  0b01000,
  0b00100,
  0b00010,
  0b00010,
  0b00100,
  0b01000,
  0b10000
};
byte customChar4[8] = {
  0b00001,
  0b00010,
  0b00100,
  0b01000,
  0b01000,
  0b00100,
  0b00010,
  0b00001
};

void setup()
{
  firsttime = EEPROM.read(3);
  if (firsttime == 1)
  {
    updateMenu();
    setpoint = EEPROM.read(0);
    EEPROM.get(1, caladjtemp);
    pinMode(10, INPUT_PULLUP);
    pinMode(11, INPUT_PULLUP);
    pinMode(12, INPUT_PULLUP);
    pinMode(13, INPUT_PULLUP);
    pinMode(2, OUTPUT);
    pinMode(3, OUTPUT);
    Serial.begin(9600);
    updateMenu();
  }
  else
  {
    firstTime();
  }
}

void firstTime()
{
  EEPROM.put(0, 104);
  EEPROM.put(1, 1.00);
  EEPROM.put(3, 1);
}

void updateMenu()
{
  switch (menu) {
    case 0:
      Pl("Menu 0");
      Serial.println(String(currenttemp) + " Degrees");

      break;
    case 1:
      Pl("Menu 1");
      Serial.println(setpoint);
      inadjmode = false;
      break;
    case 2:
      Pl("Menu 2");
      Serial.println(caladjtemp);
      inadjmode = false;
      break;
    case 10:
      inadjmode = true;
      Pl("Menu 10");
      Serial.println(setpoint);
      Serial.println(inadjmode);
      break;
    case 20:
      Pl("Menu 20");
      Serial.println(caladjtemp);
      inadjmode = true;
    case 50:
      Pl("Menu 50");
      Pl("Failsafe");
      break;
    case 70:
      Pl("Menu 70");
      Pl("Failsafe");
      break;
  }
}

void Pl(String x)
{
  Serial.println(x);
}

void saveSetpoint()
{
  EEPROM.write(0, setpoint);
  EEPROM.put(1, caladjtemp);
  Pl("Updated EEPROM");
}

byte checkButtonPress() {
  byte bP = 0;
  byte rBp = 0;
  for (t = 0; t < nrButtons; t++) {
    if (digitalRead(buttons[t]) == 0) {
      bP = (t + 1);
    }
  }
  rBp = bP;
  while (bP != 0) {
    bP = 0;
    for (t = 0; t < nrButtons; t++) {
      if (digitalRead(buttons[t]) == 0) {
        bP = (t + 1);
      }
    }
  }
  return rBp;
}

void loop() {
  sys();
  if (failsafe)
  {
    if (failsafelockout)
    {
    }
    else
    {
      updateMenu();
      failsafelockout = true;
    }
  }
  else
  {

    if (temptemp == currenttemp) {} else
    {
      if (inRange(currenttemp, temptemp - 1, temptemp + 1))
      {
      }
      else
      {

        temptemp = currenttemp;
        if (menu == 0) {
          menu = 0;
          updateMenu();
        }
      }
    }
    menusys();
  }
}

bool inRange(int val, int minimum, int maximum)
{
  return ((minimum <= val) && (val <= maximum));
}

void Heat(bool tf)
{
  if (tf == true)
  {
    //Pl("HEAT RUN");
    digitalWrite(2, HIGH);
  }
  else
  {
    //Pl("HEAT OFF");
    digitalWrite(2, LOW);
  }
}
void Pump(bool tf)
{
  if (tf == true)
  {
    //Pl("PUMP RUN");
    digitalWrite(3, HIGH);
  }
  else
  {
    //Pl("PUMP OFF");
    digitalWrite(3, LOW);
  }
}

void menusys()
{
  int pressedButton = checkButtonPress();
  int tempval = menu;
  if (pressedButton != 0) {
    switch (pressedButton) {
      case 1://up
        Pl("Up Pressed");
        if (inadjmode)
        {
          setpoint++;
        }
        else
        {

          tempval++;
          lastmenu = menu;
          if (tempval > menumax)
          {
            menu = 0;
          }
          else
          {
            menu = tempval;
          }
        }
        break;
      case 2:
        Pl("Down Pressed");
        if (inadjmode)
        {
          setpoint--;
        }
        else
        {
          tempval--;
          lastmenu = menu;
          if (tempval < 0)
          {
            menu = menumax;
          }
          else
          {
            menu = tempval;
          }
        }
        break;
      case 3:
        Pl("Enter Pressed");
        if (inadjmode)
        {
          menu = lastmenu;
          saveSetpoint();
        }
        else
        {
          lastmenu = menu;
          menu = tempval * 10;
        }
        break;
      case 4:
        Pl("Back Pressed");
        menu = lastmenu;
        break;
    }
    updateMenu();
  }
}

void sys()
{
  Pump(true);
  Vo = analogRead(0);
  Pr = analogRead(1);
  R2 = 10000 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2));
  T = T - 273.15;
  T = (T * 9.0) / 5.0 + 32.0;
  T = (T*(9.0/5.0))+28.0; //4 degress for error
  T = T + caladjtemp;
  currenttemp = T;
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
      if (currenttemp < setpoint) {
        if (systemrun == false)
        {
          stat = "RUN";
          Pump(true);
          delay(1000);
          Heat(true);
          systemrun = true;
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
          stat = "OFF";
          Heat(false);
          systemrun = false;
        }
      }
    }
    else
    {
      Pl("FAILSAFE - Heat Error: " + String(T));
      Pump(true);
      Heat(false);
      menu = 50;
      failsafe = true;
    }
  }
  else
  {
    Pl("FAILSAFE - Pump Pressure Error: " + String(analogRead(1)));
    Pump(false);
    Heat(false);
    menu = 70;
    failsafe = true;
  }
}

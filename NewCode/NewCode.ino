#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
int i = 0, temp = 0;
int Vo;
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
  lcd.init();
  lcd.backlight();
  updateMenu();
  int setpoint = EEPROM.read(0);
  lcd.createChar(0, customChar1);
  lcd.createChar(1, customChar2);
  lcd.createChar(2, customChar3);
  lcd.createChar(3, customChar4);
}

void updateMenu()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  switch (menu) {
    case 0:
      lcd.print(stat + "   " + String(currenttemp));
      printcontrol(0);
      break;
    case 1:
      inadjmode = false;
      lcd.print("Setpoint " + String(setpoint));
      printcontrol(1);
      break;
    case 10:
      inadjmode = true;
      lcd.print("Set Temp " + String(setpoint));
      printcontrol(2);
    case 50:
      lcd.print("FAILSAFE..HEAT");
      printcontrol(3);
      break;
    case 70:
      lcd.print("FAILSAFE..PUMP");
      printcontrol(3);
      break;
  }
}

void printcontrol(int x)
{
  switch (x) {
    case 0:
      lcd.setCursor(0, 5);
      lcd.print((byte)1);
      lcd.setCursor(0, 10);
      lcd.print((byte)0);
      break;
    case 1:
      lcd.setCursor(0, 5);
      lcd.print((byte)1);
      lcd.setCursor(0, 10);
      lcd.print((byte)0);
      lcd.setCursor(0, 15);
      lcd.print((byte)3);
      break;
    case 2:
      lcd.setCursor(0, 1);
      lcd.print((byte)4);
      lcd.setCursor(0, 5);
      lcd.print((byte)1);
      lcd.setCursor(0, 10);
      lcd.print((byte)0);
      lcd.setCursor(0, 15);
      lcd.print((byte)3);
      break;
    case 3:
      lcd.setCursor(0, 1);
      lcd.print(String(Vo) + "." + String(Pr));
      break;
  }
}
void saveSetpoint()
{
  EEPROM.write(0, setpoint);
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
    if (failsafelockout == false)
    {
      updateMenu();
      failsafelockout = true;
    }
  }
  else
  {
    if (temptemp == currenttemp) {} else
    { temptemp = currenttemp; if (menu == 0) {
        menu = 0;
        updateMenu();
      }
    }
    menusys();
  }
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

void menusys()
{
  int pressedButton = checkButtonPress();
  int tempval = menu;
  if (pressedButton != 0) {
    switch (pressedButton) {
      case 1://up
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
      if (T < setpoint) {
        if (systemrun == false)
        {
          stat = "RUN";
          Pump(true);
          delay(2000);
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
      Pump(true);
      Heat(false);
      menu = 50;
      failsafe = true;
    }
  }
  else
  {
    Pump(false);
    Heat(false);
    menu = 70;
    failsafe = true;
  }
}

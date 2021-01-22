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
int menumax = 2;
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
unsigned long previousMillis = 0;
String laststate = "OFF";

byte customChar1[8] = {
  0b00100,
  0b01110,
  0b10101,
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
  0b10101,
  0b01110,
  0b00100
};

byte customChar3[8] = {
  0b00000,
  0b00100,
  0b00010,
  0b11111,
  0b11111,
  0b00010,
  0b00100,
  0b00000
};

byte customChar4[8] = {
  0b00000,
  0b00100,
  0b01000,
  0b11111,
  0b11111,
  0b01000,
  0b00100,
  0b00000
};

void setup()
{
  firsttime = EEPROM.read(3);
  if (firsttime == 1)
  {
    lcd.init();
    lcd.backlight();
    updateMenu();
    setpoint = EEPROM.read(0);
    EEPROM.get(1, caladjtemp);
    lcd.createChar(0, customChar1);
    lcd.createChar(1, customChar2);
    lcd.createChar(2, customChar3);
    lcd.createChar(3, customChar4);
    pinMode(10, INPUT_PULLUP);
    pinMode(11, INPUT_PULLUP);
    pinMode(12, INPUT_PULLUP);
    pinMode(13, INPUT_PULLUP);
    pinMode(2, OUTPUT);
    pinMode(3, OUTPUT);
    Serial.begin(9600);
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
  delay(5000);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("EEPROM Loaded");
  lcd.setCursor(0, 1);
  lcd.print("Please Reset");
  while (true)
  {
    lcd.backlight();
    delay(2000);
  }
}

void updateMenu()
{
  lcd.setCursor(0, 0);
  switch (menu) {
    case 0:
      Pl("Menu 0");
      Serial.println(stat + "   " + String(currenttemp));
      lcd.print(stat + "   " + String(currenttemp));
      printcontrol(0);
      break;
    case 1:
      Pl("Menu 1");
      Serial.println("Setpoint " + String(setpoint));
      inadjmode = false;
      lcd.print("Setpoint " + String(setpoint));
      printcontrol(1);
      break;

    case 2:
      Pl("Menu 2");
      Serial.println("TempCal " + String(caladjtemp));
      inadjmode = false;
      lcd.print("TempCal " + String(caladjtemp));
      printcontrol(1);
      break;
    case 10:
      inadjmode = true;
      Pl("Menu 10");
      Serial.println("Set Temp " + String(setpoint));
      lcd.print("Set Temp " + String(setpoint));
      printcontrol(2);
      break;
    case 20:
      Pl("Menu 20");
      Serial.println("Set Cal " + String(caladjtemp));
      inadjmode = true;
      lcd.print("Set Cal " + String(caladjtemp));
      printcontrol(2);
      break;
    case 50:
      Pl("Menu 50");
      lcd.print("FAILSAFE..HEAT");
      printcontrol(3);
      break;
    case 70:
      Pl("Menu 70");
      lcd.print("FAILSAFE..PUMP");
      printcontrol(3);
      break;
  }
}

void Pl(String x)
{
  Serial.println(x);
}

void printcontrol(int x)
{
  Serial.println("Print Requested");
  switch (x) {
    case 0:
      lcd.setCursor(5, 1);
      lcd.write(1);
      lcd.setCursor(10, 1);
      lcd.write(0);
      break;
    case 1:
      lcd.setCursor(5, 1);
      lcd.write(1);
      lcd.setCursor(10, 1);
      lcd.write(0);
      lcd.setCursor(15, 1);
      lcd.write(2);
      break;
    case 2:
      lcd.setCursor(1, 1);
      lcd.write(3);
      lcd.setCursor(5, 1);
      lcd.write(1);
      lcd.setCursor(10, 1);
      lcd.write(0);
      lcd.setCursor(15, 1);
      lcd.write(2);
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
      lcd.backlight();
      delay(1000);
    }
    else
    {
      updateMenu();
      failsafelockout = true;
    }
  }
  else
  {
    if (menu == 0) {
      if (temptemp == currenttemp) {
        if (laststate == stat) {}
        else
        {
          laststate = stat;
          menu = 0;
          updateMenu();
        }
      } else
      {
        if (inRange(currenttemp, temptemp - 1, temptemp + 1))
        {
          if (laststate == stat) {}
          else
          {
            laststate = stat;
            menu = 0;
            updateMenu();
          }
        }
        else
        {
          temptemp = currenttemp;
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
    digitalWrite(2, HIGH);
  }
  else
  {
    digitalWrite(2, LOW);
  }
}

bool ticksys(long interval)
{
  unsigned long currentMillis = millis();
  return (currentMillis - previousMillis >= interval);
}

void newtick()
{
  previousMillis = millis();
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
          lcd.clear();
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
          lcd.clear();
        }
        break;
      case 3:
        Pl("Enter Pressed");
        if (inadjmode)
        {
          menu = lastmenu;
          saveSetpoint();
          lcd.clear();
        }
        else
        {
          lastmenu = menu;
          menu = tempval * 10;
          lcd.clear();
        }
        break;
      case 4:
        Pl("Back Pressed");
        menu = lastmenu;
        lcd.clear();
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
  T = (T * (9.0 / 5.0)) + 20.0; //4 degress for error
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
      if (currenttemp <= setpoint) {
        if (systemrun == false)
        {
          if (ticksys(10000))
          {
            newtick();
            stat = "RUN";
            Pump(true);
            delay(1000);
            Heat(true);
            systemrun = true;
          }
        }
        else
        {
          if (analogRead(1) < 1000)
          {

            failure = true;
            systemrun = false;
            pumpfailure = true;
            Heat(false);
            Pump(false);
          }
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

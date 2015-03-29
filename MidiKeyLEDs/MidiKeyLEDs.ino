//This example implements a simple sliding On/Off button. The example
// demonstrates drawing and touch operations.
//
//Thanks to Adafruit forums member Asteroid for the original sketch!
//
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

#define STMPE_CS 8
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);
#define TFT_CS 10
#define TFT_DC 9
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
#define CHAR_HEIGHT 16
#define CHAR_WIDTH 12
#define BIGCHAR_HEIGHT 24
#define BIGCHAR_WIDTH 18

/*
VARIABLES
*/
File root;
File song;
char filenames[16][13];
typedef struct {
  char keycode;
  char length;
} note;
int songlength;

void listFiles(void)
{
  File temp;
  char row = 0;
  char name[256];
  char nameln;
  int i;
  tft.setTextSize(3);
  tft.setCursor(1,1);
  tft.print("Select a Song");
  tft.setCursor(3,25);
  tft.setTextSize(2);
  while(temp = root.openNextFile())
  {
    if(!temp.isDirectory())
    {
      tft.setCursor(16,30+row*30);
      strcpy(filenames[row], temp.name());
      nameln = temp.read();
      for(i=0; i<nameln;i++)
      {
        name[i] = temp.read();
      }
      name[i]=0;
      tft.print(name);
    } 
    row++;
  }
  tft.setTextSize(2);
  while(1)
  {
    if(!ts.bufferEmpty())
    {
      TS_Point p = ts.getPoint();
      p.x = map(p.x, TS_MINY, TS_MAXY, 0, tft.height());
      p.y = map(p.y, TS_MINX, TS_MAXX, 0, tft.width());
      int y = tft.height() - p.x;
      int x = p.y;
      int choice = (y - 30)/30;
      if(choice < row)
      {
        song = SD.open(filenames[choice]);
        break;
      }
    } 
  }
}

void resetDisplay()
{
   //Setup Screen
  tft.fillScreen(ILI9341_WHITE);
  // origin = left,top landscape (USB left upper)
  tft.setRotation(1); 
  tft.setCursor(10,10);
  tft.setTextColor(0x000, 0xFFFF);
  tft.setTextSize(2);
}

void setup(void)
{
  //Init serial
  Serial.begin(9600);
  Serial1.begin(9600);
  
  //Init tft and touchscreen
  tft.begin();
  if (!ts.begin()) { 
    Serial.println("Unable to start touchscreen.");
  } 
  else { 
    Serial.println("Touchscreen started."); 
  }

  resetDisplay();
  int i;
  for(i=20; i<=53; i++)
  {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  //Init SD card
  pinMode(10,OUTPUT);
  if(!SD.begin(4))
  {
    tft.setTextSize(3);
    tft.print("ERROR: No SD Card");
    while(1){};
  }
}

void clearBuffers()
{
  while(Serial1.available())
  {
    Serial1.read(); 
  }
  while(!ts.bufferEmpty())
  {
    ts.getPoint();
  }
}

void chooseFile()
{
  resetDisplay();
  pinMode(10,OUTPUT);
  SD.begin(4);
  root = SD.open("/");
  root.rewindDirectory();
  listFiles();
}

void keyCodeToNote(char *str, byte code)
{
  byte i = 0;
  switch(code % 12)
  {
    case 0: str[0] = 'C'; break;
    case 1: str[++i] = 'b'; case 2: str[0] = 'D'; break;
    case 3: str[++i] = 'b'; case 4: str[0] = 'E'; break;
    case 5: str[0] = 'F'; break;
    case 6: str[++i] = 'b'; case 7: str[0] = 'G'; break;
    case 8: str[++i] = 'b'; case 9: str[0] = 'A'; break;
    case 10: str[++i] = 'b'; case 11: str[0] = 'B'; break;
  }
  str[++i] = 47 + code / 12;
  str[++i] = 0;
}

void ledSet(byte *code, byte *hit, byte num)
{
  byte i;
  for(i=20; i<=53; i++)
  {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  pinMode(16,OUTPUT); digitalWrite(16, LOW);
  pinMode(17,OUTPUT); digitalWrite(17,LOW);
  for(i=0; i<num; i++){
    if(code[i] - 28 == 50 || code[i] - 28 == 51)
    {
      digitalWrite(code[i] - 62, hit[i] ? LOW : HIGH);
    }
    else
      digitalWrite(code[i] - 28, hit[i] ? LOW : HIGH);
  }
}

void readNext(byte *code, byte *hit, byte *num)
{
  byte tmp = song.read();
  if(tmp == 0)
  {//chord
    *num = song.read();
    for(;tmp< *num; tmp++)
    {
      code[tmp] = song.read();
      hit[tmp] = 0;
    }
  }
  else
  {//single note
    code[0] = tmp;
    hit[0] = 0;
    *num = 1;
  }
  song.read(); song.read(); //length
}

//If returns 1, all pieces are hit.
// else, not all are hit
byte checkCode(byte code, byte *codes, byte *hit, byte num)
{
  byte i;
  for(i=0; i<num; i++)
  {
    if(code == codes[i])
    {
      hit[i] = 1;
    }
  }
  ledSet(codes, hit, num);
  byte all = 1;
  for(i=0;i<num;i++)
  {
    if(!hit[i])
      all = 0;
  }
  return all;
}

void playSong()
{
  if(!song)
  {
    return;
  }
  resetDisplay();
  clearBuffers();
  tft.setCursor(1,1);
  tft.print("Now playing: ");
  tft.print(song.name());
  tft.setCursor(1,CHAR_HEIGHT+2);
  
  //Print nice name
  int i = 0;
  char name[127];
  char noteBuf[4];
  char code;
  int nameln = song.read();
  for(i=0; i<nameln;i++)
  {
    name[i] = song.read();
  }
  name[i]=0;
  tft.print(name);
  tft.setCursor(1,CHAR_HEIGHT*2+2);
  short int notes = song.read() << 8 | song.read();
  tft.print("Song has "); tft.print(notes);
  tft.print(" notes.");
  tft.drawRect(10,200,300, 35, 0);
  tft.setTextSize(3);
  tft.setCursor(20,205);
  tft.print("Return to List");
  
  i = 0;
  byte codes[8];
  byte hit[8];
  byte num;
  
  readNext(codes, hit, &num);
  ledSet(codes, hit, num);
  while(i < notes)
  {
    if(Serial1.available())
    {
      Serial.print(i);
      code = Serial1.read();
      if(code == 36)
      {
        return;
      }
      if(checkCode(code, codes, hit, num))
      {
        if(i++ == notes) return;
        readNext(codes,hit,&num);
        ledSet(codes,hit,num);
      }
    }
    if (!ts.bufferEmpty())
    {
      TS_Point p = ts.getPoint();
      p.x = map(p.x, TS_MINY, TS_MAXY, 0, tft.height());
      p.y = map(p.y, TS_MINX, TS_MAXX, 0, tft.width());
      int y = tft.height() - p.x;
      int x = p.y;
      if(y > 200)
      {
        return;
      }
    }
  }
}

void loop()
{
  clearBuffers();
  chooseFile();
  clearBuffers();
  playSong();
}

void getKey()
{
  // See if there's any  touch data for us
  if (!ts.bufferEmpty())
  {
    // Retrieve a point  
    TS_Point p = ts.getPoint(); 
    // Scale using the calibration #'s
    // and rotate coordinate system
    p.x = map(p.x, TS_MINY, TS_MAXY, 0, tft.height());
    p.y = map(p.y, TS_MINX, TS_MAXX, 0, tft.width());
    int y = tft.height() - p.x;
    int x = p.y;
    
  }
  
}




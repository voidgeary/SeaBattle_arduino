#include <ArduinoJson.h>
#include <Process.h>
#define load 6
#define write_bit 5

StaticJsonDocument<150> doc;
String result;
bool LED_twinkling[16][8] = {};
bool LED_matrix[16][8] = {};
bool game = 1;

void setup() {
  // put your setup code here, to run once:
  Bridge.begin();   // Initialize the Bridge
  Serial.begin(9600);
  while(!Serial);
  for(byte i=2;i<=11;i++){
    pinMode(i, OUTPUT);
    digitalWrite(i, 0);
  }
  printLed();
  reset();
  start("Sente/");
  start("Gote/");
  //start("Onlooker/");
}
void loop() {
  // put your main code here, to run repeatedly:
  battle();
}
void reset(){
  for(byte y=0;y<8;y++)
    for(byte x=0;x<16;x++){
      setLed(0, x, y, 1);
      delay(10);
      setLed(1, x, y, 1);
      delay(10);
      setLed(0, x, y, 1);
      delay(10);
    }
}
void start(String name){
  String prince = "https://seabattlenutn.firebaseio.com/State/";
  prince += name;
  String ship[5] = {"BB/", "CA/", "CL/", "CV/", "DD/"};
  for(byte i = 0; i < 5; i++){
    download(prince + ship[i] + "num.json");
    byte num = result[1] - '0';
    result = "";
    Serial.println(num);
    for(byte j = 0;j<num;j++){
      String n = "";
      n += j;
      n += ".json";
      download(prince + ship[i] + n);
      DeserializationError err = deserializeJson(doc, (String)result);
      result = "";
      //debug
      if(err){
        Serial.print("ERROR: ");
        Serial.println(err.c_str());
        return;
      }
      byte pos1X = doc["Pos1"]["X"];
      byte pos1Y = doc["Pos1"]["Y"];
      byte pos2X = doc["Pos2"]["X"];
      byte pos2Y = doc["Pos2"]["Y"];
      bool player;
      if(name == "Sente/"/*|| name == "Onlooker/"*/)
        player = 1;
      else
        player = 0;
      shipLed(pos1X, pos1Y, pos2X, pos2Y, ship[i], player);
    }
    printLed();
  }
}
void battle(){
  for(byte n = 1;game == 1;){
    twinkling();
    Serial.println(n);
    result = "";
    String prince = "https://seabattlenutn.firebaseio.com/Battle/";
    /*if(n<10)
      prince += "00";
    else if(n < 100)
      prince += "0";*/          //test data
    prince += n;
    prince += ".json";
    download(prince);
    if(result == "null")
      delay(100);
    else{
      DeserializationError err = deserializeJson(doc, (String)result);
      result = "";
      //debug
      if(err){
        Serial.print("ERROR: ");
        Serial.println(err.c_str());
        return;
      }
      for(byte c=0;c<15;c++){
        String t = "";
        t += (char)('A'+c);
        String tt = doc[t];
        Serial.println(tt);
        if(tt != "null")
          action(t, tt, n%2);
      }
      n++;
    }
  }
}
void action(String t, String tt, bool player){
  if(t == "A"||t == "B"){
    byte y = tt[0] - 'A';
    byte x1 = tt[1] - 'A';
    byte x2 = tt[2] - 'A';
    FL(y, x1, x2, player);
  }
  if(t == "C"||t == "D"||t == "E"||t == "F"){
    byte x = tt[0] - 'A';
    byte y = tt[1] - 'A';
    SH(x, y, player);
  }
  if(t == "G"||t == "H"||t == "I"||t == "J"){
    byte x = tt[0] - 'A';
    byte y = tt[1] - 'A';
    DF(x, y, player, 1);
  }
  if(t == "K"||t == "L"||t == "M"||t == "N"){
    char type = tt[0];
    byte x1 = tt[1] - 'A';
    byte y1 = tt[2] - 'A';
    byte x2 = tt[3] - 'A';
    byte y2 = tt[4] - 'A';
    DE(type, x1, y1, x2, y2, player);
    
  }
  if(t == "O"){
    char winner = tt[0];
    ED(winner);
  }
}
void FL(byte y, byte x1, byte x2, bool player){
  setLed(1, x1, y, player);
  for(byte x = x1+1; x < x2;x++){
    setLed(1, x, y, player);
    delay(500);
    if(checkMatrix(x-1, y, player)==0)
      setLed(0, x-1, y, player);
    twinkling();
  }
  delay(500);
  setLed(0, x2-1, y, player);
}
void SH(byte x, byte y, bool player){
  setLed(1, x, y, player);
  delay(500);
  setLed(0, x, y, player);
}
void DE(char type, byte x1, byte y1, byte x2, byte y2, bool player){
  if(type == 'V'){
    for(byte posX = x1;;){
      for(byte posY = y1;;){
        setLed(1, posX, posY, player);    //at check time, we need change 1 to 0.
        setMatrix(0, posX, posY, player);
        DF(posX, posY, player, 0);
        if(posY == y2)
          break;
        posY += ((y2 - y1) > 0) - ((y2 - y1) < 0);
      }
      if(posX == x2)
        break;
      posX += ((x2 - x1) > 0) - ((x2 - x1) < 0);
    }
  }
  else{
    for(byte posX = x1, posY = y1;;){
      setLed(0, posX, posY, player);    //at check time, we need change 1 to 0.
      setMatrix(0, posX, posY, player);
      DF(posX, posY, player, 0);
      if(posX == x2 && posY == y2)
        break;
      posX += ((x2 - x1) > 0) - ((x2 - x1) < 0);
      posY += ((y2 - y1) > 0) - ((y2 - y1) < 0);
    }
  }
}
void DF(byte x, byte y, bool player, bool sw){
  byte posX = x;
  byte posY = y;
  if(player == 0){
    posX = 15 - posX;
    posY = 7 - posY;
  }
  LED_twinkling[posX][posY] = sw;
}

void twinkling(){
  for(byte i=0;i<16;i++){
    for(byte j=0;j<8;j++)
      if(LED_twinkling[i][j] == 1)
        setLed(0, i, j, 1);
  }
  delay(100);
  for(byte i=0;i<16;i++){
    for(byte j=0;j<8;j++)
      if(LED_twinkling[i][j] == 1)
        setLed(1, i, j, 1);
  }
}
void ED(char winner){
  game = 0;
  delay(5000);
  reset();
  if(winner == 'S')
    Serial.println("Sente is winner!");
  else
    Serial.println("Gote is winner!");
}
void download(String prince){
  Serial.println("download start!");
  Process getDataWithFirebase;
  getDataWithFirebase.begin("curl");
  getDataWithFirebase.addParameter("-k");
  getDataWithFirebase.addParameter(prince);
  getDataWithFirebase.run();
  while(getDataWithFirebase.running());  
  while (getDataWithFirebase.available()) {
      char word = getDataWithFirebase.read();
      result +=word;
  }
  Serial.println("String result : ");
  Serial.println(result);
  Serial.println("download end!");
}
void shipLed(byte pos1X, byte pos1Y, byte pos2X, byte pos2Y, String ship, bool player){
  Serial.println(pos1X);
  Serial.println(pos1Y);
  Serial.println(pos2X);
  Serial.println(pos2Y);
  if(ship == "DD/"){
    setLed(1, pos1X, pos1Y, player); 
    setMatrix(1, pos1X, pos1Y, player);
  }
  else if(ship == "CV/"){
    for(byte posX = pos1X;;){
      for(byte posY = pos1Y;;){
        setLed(1, posX, posY, player); 
        setMatrix(1, posX, posY, player);
        if(posY == pos2Y)
          break;
        posY += ((pos2Y - pos1Y) > 0) - ((pos2Y - pos1Y) < 0);
      }
      if(posX == pos2X)
        break;
      posX += ((pos2X - pos1X) > 0) - ((pos2X - pos1X) < 0);
  }  }
  else{
    for(byte posX = pos1X, posY = pos1Y;;){
      setLed(1, posX, posY, player);    
      setMatrix(1, posX, posY, player);
      if(posX == pos2X && posY == pos2Y)
        break;
      posX += ((pos2X - pos1X) > 0) - ((pos2X - pos1X) < 0);
      posY += ((pos2Y - pos1Y) > 0) - ((pos2Y - pos1Y) < 0);
}  	}	}
void setLed(bool a, byte posX, byte posY, bool player){
  posY = 7 - posY;
  if(player == 0){
    posX = 15 - posX;
    posY = 7 - posY;
  }
  //LED_twinkling[posX][posY] = a;
  Serial.print(posX);
  Serial.print(" ");
  Serial.println(posY);
  setAddr(posX, posY);
  digitalWrite(write_bit, a);
  digitalWrite(load, 1);
  delay(1);
  digitalWrite(load, 0);
}
void setMatrix(bool a, byte posX, byte posY, bool player){
  posY = 7 - posY;
  if(player == 0){
    posX = 15 - posX;
    posY = 7 - posY;
  }
  LED_matrix[posX][posY] = a;
}
bool checkMatrix(byte posX, byte posY, bool player){
  posY = 7 - posY;
  if(player == 0){
    posX = 15 - posX;
    posY = 7 - posY;
  }
  if(LED_matrix[posX][posY] == 1)
    return 1;
  return 0;
}
void setAddr(byte col, byte row){
  if(col >= 8)
    digitalWrite(11, 1);
  else
    digitalWrite(11, 0);
  bool write_addr[3] = {};
  for(byte i=0;i<3;i++){
    write_addr[2 - i] = row % 2, row /= 2;
    digitalWrite(10 - i, write_addr[2 - i]);
  }
  for(byte i=0;i<3;i++){
    write_addr[2 - i] = col % 2, col /= 2;
    digitalWrite(4 - i, write_addr[2 - i]);
  }
}
void printLed(){
  Serial.println();
  for(short j=7;j>=0;j--){
    for(byte i=0;i<16;i++){
      Serial.print(LED_twinkling[i][j]);
      Serial.print(" ");
    }
      Serial.println();
  }
}

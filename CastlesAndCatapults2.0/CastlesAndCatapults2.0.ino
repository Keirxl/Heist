
#define PALE makeColorHSB(200,50,70)
#define FORT makeColorHSB(90,255,255)
#define dustblue makeColorHSB(115,255,255)
#define burntyellow makeColorHSB(35,255,255)
#define burntorange makeColorHSB(25,200,255)
#define purple makeColorHSB(200,255,255)
#define pink makeColorHSB(10,210,255)
#define teal makeColorHSB(100,255,255)
#define mint makeColorHSB(90,255,255)
#define pastelpurple makeColorHSB(190,255,255)


#define dimness 100
#define DAMAGE_DURATION 200

enum signalStates {INERT,RESET,RESOLVE};
byte signalState = INERT;
enum blinkModes {CASTLE,CATAPULT,DEAD};
byte blinkMode=CASTLE;
byte team=0;
//team at [A], signalState at [C][D], blinkMode at [E][F]
byte sendData= (team << 4)+(signalState << 2)+(blinkMode);

Color teamColor[4]={dustblue, pastelpurple, burntorange, teal};
byte ignoredFaces[6]={0,0,0,0,0,0};
byte connectedFaces[6]={0,0,0,0,0,0}; //1 if fort nearby. then show pale if 1
byte hp=4;
byte lastConnectedTeam=0;
bool isDead=false;
bool isTroops=false;
Timer damageTimer;

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  switch (signalState) {
    case INERT:
      inertLoop();
      break;
    case RESET:
      resetLoop();
      break;
    case RESOLVE:
      resolveLoop();
      break;
  }

  
  if(blinkMode==DEAD){
    captureDisplay();
  }else if(blinkMode==CATAPULT){
    teamSet();
  }else{
    if(!damageTimer.isExpired()){
      damageDisplay();
    }else{
       castleDisplay();
    }
  }
  
  byte sendData= (team << 4)+(signalState << 2)+(blinkMode);
  setValueSentOnAllFaces(sendData);
}


//all the gameplay happens here
void inertLoop() {
  
  //set myself to RESET
  if (buttonMultiClicked()) {
    byte clicks=buttonClickCount();
    if(clicks==3){
      signalState = RESET;
    }
  }

  // Turn me into a catapult
  if(buttonLongPressed()){
    blinkMode=CATAPULT;
    team=0;
    teamSet();
  }

  if(blinkMode==CATAPULT){
    if(buttonDoubleClicked()){
        team++;
        if(team==4){
          team=0;
        }
        teamSet();
     }
  }

  // Look for Attackers Here
  FOREACH_FACE(f){
    if(!isValueReceivedOnFaceExpired(f)){//am i connected?
       if(getBlinkMode(getLastValueReceivedOnFace(f))==CATAPULT){// to a catapult?
          if(ignoredFaces[f]==0){//not an ignored face
            hp--;
            damageTimer.set(DAMAGE_DURATION);
            lastConnectedTeam=getTeam(getLastValueReceivedOnFace(f));
            if(hp<1){
              blinkMode=DEAD;
            }
          }
       }
       if(getBlinkMode(getLastValueReceivedOnFace(f))!=DEAD){
        ignoredFaces[f]=1;
       }
    }else{
      if(getBlinkMode(getLastValueReceivedOnFace(f))!=DEAD){
        ignoredFaces[f]=0;
       }
    }
  }
  

  //listen for neighbors in RESET
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getSignalState(getLastValueReceivedOnFace(f)) == RESET) {//a neighbor saying RESET!
        signalState = RESET;
      }
    }
  }
}

void resetLoop() {
  signalState = RESOLVE;//I default to this at the start of the loop. Only if I see a problem does this not happen

  blinkMode=CASTLE;
  hp=4;
  team=0;

  //look for neighbors who have not heard the RESET news
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getSignalState(getLastValueReceivedOnFace(f)) == INERT) {//This neighbor doesn't know it's RESET time. Stay in RESET
        signalState = RESET;
      }
    }
  }
}

void resolveLoop() {
  signalState = INERT;//I default to this at the start of the loop. Only if I see a problem does this not happen

  //look for neighbors who have not moved to RESOLVE
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getSignalState(getLastValueReceivedOnFace(f)) == RESET) {//This neighbor isn't in RESOLVE. Stay in RESOLVE
        signalState = RESOLVE;
      }
    }
  }
}

void captureDisplay(){
  setColor(teamColor[lastConnectedTeam]);
}

void castleDisplay(){
  FOREACH_FACE(f){
    if(!isValueReceivedOnFaceExpired(f)){
      if(getBlinkMode(getLastValueReceivedOnFace(f))==CASTLE){
        setColorOnFace(PALE,f);
      }else{
        setColorOnFace(FORT,f);
      }
    }else{
      setColorOnFace(FORT,f);
    }
  }
}

void damageDisplay(){
  setColor(RED);
}

void teamSet(){
  setColor(teamColor[team]);
  setColorOnFace(dim(teamColor[team],dimness),0);
  setColorOnFace(dim(teamColor[team],dimness),2);
  setColorOnFace(dim(teamColor[team],dimness),4);
}

//team at [A], signalState at [C][D], blinkMode at [E][F]
byte getBlinkMode(byte data) {
  return (data & 3);
}

byte getSignalState(byte data) {
  return ((data >> 2) & 3);
}

byte getTeam(byte data) {
  return (data >> 4);
}

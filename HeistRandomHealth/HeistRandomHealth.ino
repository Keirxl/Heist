//  __Heist__
//  by Keir Williams with help by Move38's Dan King
//  

#define VAULT makeColorHSB(200,50,70) //walls
#define GOLD makeColorHSB(200,50,70) //interior
#define GOLDEN makeColorHSB(37,240,255)  //gold pieces
#define dustblue makeColorHSB(115,255,255)
#define burntorange makeColorHSB(25,200,255)
#define purple makeColorHSB(200,255,255)
#define pink makeColorHSB(240,235,255)
#define teal makeColorHSB(100,255,255)
#define mint makeColorHSB(80,220,255)
#define pastelpurple makeColorHSB(190,255,255)
#define yellowy makeColorHSB(43,240,255)
#define shimmer 43
#define PALE makeColorHSB(200,50,70)



#define DAMAGE_DURATION 500 //time to flash on a hit
#define DEAD_DURATION 120 //time to finish spin
#define DEAD_WAIT 1500 //pause between spins
#define FINAL_DAMAGE_DURATION 1500
#define WOBBLE_DURATION 100
#define BREATH_DURATION 200
#define VICTORY_LAP  50 //time for team to lap the gold piece
#define SPARKLE_DURATION 1000
#define SPARKLE_FADE 150
#define PULSE_LENGTH 2000
#define HEALTH 6

enum signalStates {INERT,RESET,RESOLVE};
byte signalState = INERT;
enum blinkModes {BANK,THEIF,DEAD};
byte blinkMode=BANK;
byte team=0;
//team at [A], signalState at [C][D], blinkMode at [E][F]
byte sendData= (team << 4)+(signalState << 2)+(blinkMode);

Color teamColor[4]={teal,WHITE,mint,burntorange};
byte ignoredFaces[6]={0,0,0,0,0,0};
byte connectedFaces[6]={0,0,0,0,0,0}; //1 if attached
byte hp=HEALTH;
byte randomHealth;
byte maxHp;
byte lastConnectedTeam=0;
bool isDead=false;
bool isDecrease=false;
bool isTroops=false;
byte damageDim;
byte deadFace=0;
byte sparkleFace;
byte sparkleSat;
byte victoryFace=0;
byte dimness;
bool flipDirection=false;
Timer damageTimer;
Timer deadTimer; //for Dead Display
Timer deadWaitTimer; //for Dead Display
Timer finalDamageTimer;
Timer victoryTimer;
Timer wobbleTimer;
Timer sparkleTimer;
Timer sparkleFadeTimer;




void setup() {
  // put your setup code here, to run once:
  randomize();
  
  //makes 4 hp way more likely
  randomHealth=random(100);
  if(randomHealth<75){
    maxHp=4;
  }else{
    maxHp=6;
  }
  hp=maxHp;
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

  
 // this controls what is displayed
  switch(blinkMode){
    case DEAD:
      for(byte i=0;i<6;i++){
        ignoredFaces[i]=1;
      }
      if(!finalDamageTimer.isExpired()){
        finalDamageDisplay();
      }else{
        victoryFace=0;
        deadDisplay();
        //deadSparkle();
        //setColor(GOLDEN);
      }
      break;
    case THEIF:
      teamSet();
      break;
    case BANK:
      if(!damageTimer.isExpired()){
       damageDisplay();
      }else if(hp<=(maxHp/2)){
       wobbleDisplay();
      }else{
       BANKDisplay();
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

  // Turn me into a THEIF
  if(buttonLongPressed()){
    if(isAlone()){
      blinkMode=THEIF;
      team=0;
      teamSet();
    }
  }

  //Change Teams
  if(blinkMode==THEIF){
    if(buttonDoubleClicked()){
      if(isAlone()){
          team++;
          if(team==4){
            team=0;
          }
        teamSet();
       }
    }
  }


  // Look for Attackers Here
  if(blinkMode==BANK){
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){//am i connected?
         if(getBlinkMode(getLastValueReceivedOnFace(f))==THEIF){// to a THEIF?
            if(ignoredFaces[f]==0){//not an ignored face
              hp--;
              damageTimer.set(DAMAGE_DURATION);
              lastConnectedTeam=getTeam(getLastValueReceivedOnFace(f));
              if(hp<1){
                finalDamageTimer.set(FINAL_DAMAGE_DURATION);
                setColor(VAULT);
                blinkMode=DEAD;
              }
            }
         }
         if(getBlinkMode(getLastValueReceivedOnFace(f))!=DEAD){
          ignoredFaces[f]=1;
         }
      }else{
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

  blinkMode=BANK;
  //hp=HEALTH;
  //makes 4 hp way more likely
  randomHealth=random(100);
  if(randomHealth<75){
    maxHp=4;
  }else{
    maxHp=6;
  }
  hp=maxHp;

  //resets team to 0
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


void finalDamageDisplay(){
    if(victoryTimer.isExpired()){
      victoryFace++;
      victoryTimer.set(VICTORY_LAP);
    }
  setColor(dim(teamColor[lastConnectedTeam],70));
  setColorOnFace(teamColor[lastConnectedTeam],victoryFace%6);
}

//captured pieces spin
void deadDisplay(){
  if(deadWaitTimer.isExpired()){
    if(deadTimer.isExpired()){
      setColor(GOLDEN);
      setColorOnFace(teamColor[lastConnectedTeam],deadFace%6);
      deadFace++;
        if(deadFace==13){
          deadWaitTimer.set(DEAD_WAIT);
          deadFace=0;
        }
      deadTimer.set(DEAD_DURATION);
    }
  }else{
    setColor(GOLDEN);
  }
}

//makes dead pieces sparkle
void deadSparkle(){
  if(sparkleTimer.isExpired()){
    sparkleFace=random(5)+0;
    sparkleSat=random(70)+150;
    sparkleTimer.set(SPARKLE_DURATION);
  }
  if(sparkleFadeTimer.isExpired()){
     sparkleSat+=10;
     if(sparkleSat>244){
        sparkleSat=random(70)+150;
     }
     sparkleFadeTimer.set(SPARKLE_FADE);
  }
    setColor(GOLDEN);
    setColorOnFace(dim(teamColor[lastConnectedTeam],sparkleSat),sparkleFace);

}


//same as BANKdisplay but it wobbles to show its at half health
void wobbleDisplay(){
  if(sparkleTimer.isExpired()){
    sparkleFace=random(5)+1;
    sparkleSat=random(50)+130;
    sparkleTimer.set(SPARKLE_DURATION);
  }
  if(sparkleFadeTimer.isExpired()){
     sparkleSat+=10;
     if(sparkleSat>244){
        sparkleSat=random(50)+130;
     }
     sparkleFadeTimer.set(SPARKLE_FADE);
  }

  breathe(180,90,WOBBLE_DURATION);
    

  if(isAlone()){
    setColor(GOLDEN);
  }else{
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        if(getBlinkMode(getLastValueReceivedOnFace(f))==BANK){
            connectedFaces[f]=1;
            if(sparkleFace==f){
                setColorOnFace(makeColorHSB(43,200,sparkleSat),sparkleFace);
            }else{
              setColorOnFace(GOLDEN,f);
            }
        }else{
          connectedFaces[f]=0;
          setColorOnFace(makeColorHSB(200,50,dimness),f);
        }
      }else{
        connectedFaces[f]=0;
        setColorOnFace(makeColorHSB(200,50,dimness),f);
      }
    }
  }
}


void BANKDisplay(){
  if(sparkleTimer.isExpired()){
    sparkleFace=random(5)+1;
    sparkleSat=random(50)+130;
    sparkleTimer.set(SPARKLE_DURATION);
  }
  if(sparkleFadeTimer.isExpired()){
     sparkleSat+=10;
     if(sparkleSat>255){
        sparkleSat=random(55)+140;
     }
     sparkleFadeTimer.set(SPARKLE_FADE);
  }
    

  if(isAlone()){
    setColor(GOLDEN);
  }else{
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        if(getBlinkMode(getLastValueReceivedOnFace(f))==BANK){
            connectedFaces[f]=1;
            if(sparkleFace==f){
                setColorOnFace(makeColorHSB(43,sparkleSat,255),sparkleFace);
            }else{
              setColorOnFace(GOLDEN,f);
            }
        }else{
          connectedFaces[f]=0;
          setColorOnFace(VAULT,f);
        }
      }else{
        connectedFaces[f]=0;
        setColorOnFace(VAULT,f);
      }
    }
  }
}

void damageDisplay(){
  damageDim=damageTimer.getRemaining();
  setColor(dim(teamColor[lastConnectedTeam],damageDim));
}

void teamSet(){
  breathe(249,200,BREATH_DURATION);
  setColor(dim(teamColor[team],dimness));
}

void breathe(byte high, byte low, byte duration){
  if(wobbleTimer.isExpired()){
    if(!isDecrease){
      dimness+=10;
    }else{
      dimness-=10;
    }
    if(dimness>high){
      dimness=high;
      isDecrease=true;
    }else if(dimness<low){
      dimness=low;
      isDecrease=false;
    }
    wobbleTimer.set(duration);
  }
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

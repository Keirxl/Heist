//________________________________________________________________________________
//   Heist
//         by Keir Williams
//            With help from Move38 (Thanks to Dan King!)
//
//                                                        I.. kill.. the bus driver
//__________________________________________________________________________________

#define IMPACT_OFFSET 100
#define IMPACT_FADE 650

//COLORS
#define VAULT makeColorHSB(200,50,70) //walls
#define GOLDEN makeColorHSB(37,240,255)  //gold pieces
#define GOLDHUE 37
#define burntorange makeColorHSB(25,200,255)
#define teal makeColorHSB(100,255,255)
#define mint makeColorHSB(75,220,255)
#define PALE makeColorHSB(200,50,70)


//TOO MANY VARIABLES
#define DAMAGE_DURATION 500 //time to flash on a hit
#define DEAD_DURATION 120 //time to finish spin
#define DEAD_WAIT 1500 //pause between spins
#define FINAL_DAMAGE_DURATION 2500
#define WOBBLE_DURATION 75
#define VICTORY_LAP  50 //time for team to lap the gold piece
#define SPARKLE_DURATION 1000
#define SPARKLE_FADE 150
#define PULSE_LENGTH 2000
#define HEALTH 4
#define PERIOD 2500

//SIGNALS AND ENUMS AND STATE CHANGE OH MY!
enum signalStates {INERT, RESET, RESOLVE};
byte signalState = INERT;
enum blinkModes {BANK, THEIF, DEAD};
byte blinkMode = BANK;
byte team = 0;


//BYTES (and colors)
Color teamColor[4] = {teal, WHITE, mint, burntorange};
byte ignoredFaces[6] = {0, 0, 0, 0, 0, 0};
byte connectedFaces[6] = {0, 0, 0, 0, 0, 0}; //1 if attached
byte theifOffFace = 0;
byte hp = HEALTH;
byte lastConnectedTeam = 0;
byte damageDim;
byte deadFace = 0;
byte sparkleFace;
byte sparkleSat;
byte victoryFace = 2;
byte dimness;
byte impactFace;
byte nextFace = 0;
byte complimentFace = 4;

//TIMERS
Timer damageTimer;
Timer deadTimer; //for Dead Display
Timer deadWaitTimer; //for Dead Display
Timer finalDamageTimer;
Timer victoryTimer;
Timer wobbleTimer;
Timer sparkleTimer;
Timer sparkleFadeTimer;
Timer impactTimer;

//BOOLEANS BABY!
bool isDead = false;
bool isDecrease = false;
bool isTroops = false;
bool flipDirection = false;




void setup() {
  // put your setup code here, to run once:
  randomize();
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


  if (blinkMode == DEAD) {
    for (byte i = 0; i < 6; i++) {
      ignoredFaces[i] = 1;
    }
    if (!finalDamageTimer.isExpired()) {
      finalDamageDisplay();
    } else {
      victoryFace = 2;
      deadDisplay();
      //deadSparkle();
      //setColor(GOLDEN);
    }
  } else if (blinkMode == THEIF) {
    teamSet();
  } else {
    if (!impactTimer.isExpired()) {
      impactDisplay();
      //damageDisplay();
    } else if (hp <= HEALTH / 2) {
      wobbleDisplay();
    } else {
      BANKDisplay();
    }
  }

  byte sendData = (team << 4) + (signalState << 2) + (blinkMode);
  setValueSentOnAllFaces(sendData);

  buttonLongPressed();
  buttonDoubleClicked();
}


//all the gameplay happens here
void inertLoop() {

  //set myself to RESET
  if (buttonMultiClicked()) {
    byte clicks = buttonClickCount();
    if (clicks == 3) {
      signalState = RESET;
    }
  }

  // Turn me into a THEIF
  if (blinkMode != THEIF) {
    if (buttonLongPressed()) {
      if (isAlone()) {
        blinkMode = THEIF;
        team = 0;
        teamSet();
      }
    }
  }

  //Change Teams
  if (blinkMode == THEIF) {
    if (buttonSingleClicked()) {
      theifOffFace++;
      if (theifOffFace > 5) {
        theifOffFace = 0;
      }
    }
    if (buttonDoubleClicked()) {
      if (isAlone()) {
        team++;
        if (team == 4) {
          team = 0;
        }
        teamSet();
      }
    }
  }
  if (blinkMode == THEIF) {
    if (buttonLongPressed()) {
      if (isAlone()) {
        blinkMode = BANK;
        hp = HEALTH;
      }
    }
  }



  // Look for Attackers Here
  if (blinkMode == BANK) {
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) { //am i connected?
        if (getBlinkMode(getLastValueReceivedOnFace(f)) == THEIF) { // to a THEIF?
          if (ignoredFaces[f] == 0) { //not an ignored face
            hp--;
            //damageTimer.set(DAMAGE_DURATION);
            impactTimer.set((IMPACT_OFFSET * 3) + IMPACT_FADE);
            impactFace = f;
            nextFace = 0;
            complimentFace = 4;
            lastConnectedTeam = getTeam(getLastValueReceivedOnFace(f));
            if (hp < 1) {
              finalDamageTimer.set(FINAL_DAMAGE_DURATION);
              setColor(VAULT);
              blinkMode = DEAD;
            }
          }
        }
        if (getBlinkMode(getLastValueReceivedOnFace(f)) != DEAD) {
          ignoredFaces[f] = 1;
        }
      } else {
        ignoredFaces[f] = 0;
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

  blinkMode = BANK;
  hp = HEALTH;
  //hp=random(3)+3;
  team = 0;

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


//same as BANKdisplay but it wobbles to show its at half health
void wobbleDisplay() {
  if (sparkleTimer.isExpired()) {
    sparkleFace = random(60) % 6;
    sparkleSat = random(40) + 100;
    sparkleTimer.set(random(300) + 900);
  }
  if (sparkleFadeTimer.isExpired()) {
    sparkleSat += 10;
    if (sparkleSat > 244) {
      sparkleSat = random(40) + 100;
    }
    sparkleFadeTimer.set(SPARKLE_FADE);
  }

  breathe(70, 180);


  if (noBanksAround()) {
    setColor(makeColorHSB(GOLDHUE, 240, 255));
    setColorOnFace(makeColorHSB(GOLDHUE, sparkleSat, 255), sparkleFace);
  } else {
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        if (getBlinkMode(getLastValueReceivedOnFace(f)) == BANK) {
          connectedFaces[f] = 1;
          if (sparkleFace == f) {
            setColorOnFace(makeColorHSB(GOLDHUE, sparkleSat, 255), sparkleFace);
          } else {
            setColorOnFace(GOLDEN, f);
          }
        } else {
          connectedFaces[f] = 0;
          setColorOnFace(makeColorHSB(200, 50, random(180 - dimness) + dimness), f);
        }
      } else {
        connectedFaces[f] = 0;
        setColorOnFace(makeColorHSB(200, 50, random(180 - dimness) + dimness), f);
      }
    }
  }
}


void BANKDisplay() {
  if (sparkleTimer.isExpired()) {
    sparkleFace = random(60) % 6;
    sparkleSat = random(40) + 100;
    sparkleTimer.set(random(300) + 900);
  }
  if (sparkleFadeTimer.isExpired()) {
    sparkleSat += 10;
    if (sparkleSat > 255) {
      sparkleSat = random(40) + 100;
    }
    sparkleFadeTimer.set(SPARKLE_FADE);
  }


  if (noBanksAround()) {
    setColor(makeColorHSB(GOLDHUE, 240, 255));
    setColorOnFace(makeColorHSB(GOLDHUE, sparkleSat, 255), sparkleFace);
  } else {
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        if (getBlinkMode(getLastValueReceivedOnFace(f)) == BANK) {
          connectedFaces[f] = 1;
          if (sparkleFace == f) {
            setColorOnFace(makeColorHSB(GOLDHUE, sparkleSat, 255), sparkleFace);
          } else {
            setColorOnFace(GOLDEN, f);
          }
        } else {
          connectedFaces[f] = 0;
          setColorOnFace(VAULT, f);
        }
      } else {
        connectedFaces[f] = 0;
        setColorOnFace(VAULT, f);
      }
    }
  }
}

void damageDisplay() {
  damageDim = damageTimer.getRemaining();
  setColor(dim(teamColor[lastConnectedTeam], damageDim));
}

void teamSet() {
  breathe(180, 255);
  setColor(dim(teamColor[team], dimness));
  setColorOnFace(OFF, theifOffFace);
  setColorOnFace(OFF, (theifOffFace + 3) % 6);
}

void breathe(byte low, byte high) {
  byte breathProgress = map(millis() % PERIOD, 0, PERIOD, 0, 255);
  dimness = map(sin8_C(breathProgress), 0, 255, low, high);
}

bool noBanksAround() {
  byte bankNeighbors = 0;
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getBlinkMode(getLastValueReceivedOnFace(f)) == BANK) {
        bankNeighbors++;
      }
    }
  }
  if (bankNeighbors == 0) {
    return true;
  } else {
    return false;
  }
}


void finalDamageDisplay() {
  if (finalDamageTimer.getRemaining() > 1000) {//initial fast spin
    if (victoryTimer.isExpired()) {
      victoryFace++;
      victoryTimer.set(VICTORY_LAP);
    }
    setColor(dim(teamColor[lastConnectedTeam], 70));
  } else {
    //we're gonna map between VICTORY_LAP and DEAD_DURATION)
    int spinDuration = map(1000 - finalDamageTimer.getRemaining(), 0, 1000, VICTORY_LAP, DEAD_DURATION);
    if (victoryTimer.isExpired()) {
      victoryFace++;
      victoryTimer.set(spinDuration);
    }

    //then also we're gonna fade up the golden
    byte goldenBrightness = map(1000 - finalDamageTimer.getRemaining(), 0, 1000, 70, 255);
    setColor(dim(GOLDEN, goldenBrightness));
  }

  setColorOnFace(teamColor[lastConnectedTeam], victoryFace % 6);
  //make sure the other animation is synced
  //deadFace = victoryFace;
}

//captured pieces spin
void deadDisplay() {
  if (deadWaitTimer.isExpired()) {
    if (deadTimer.isExpired()) {
      setColor(GOLDEN);
      setColorOnFace(teamColor[lastConnectedTeam], deadFace % 6);
      deadFace++;
      if (deadFace == 13) {
        deadWaitTimer.set(DEAD_WAIT);
        deadFace = 0;
      }
      deadTimer.set(DEAD_DURATION);
    }
  } else {
    setColor(GOLDEN);
  }
}

void impactDisplay() {
  int timeElapsed = ((IMPACT_OFFSET * 3) + IMPACT_FADE) - impactTimer.getRemaining();

  FOREACH_FACE(f) {
    //calculate group
    byte group = 0;
    if (abs(f - impactFace) == 1 || abs(f - impactFace) == 5) {
      group = 1;
    } else if (abs(f - impactFace) == 2 || abs(f - impactFace) == 4) {
      group = 2;
    } else if (abs(f - impactFace) == 3) {
      group = 3;
    }

    //calculate brightness
    //should I be on?
    byte brightness = 70;
    if (timeElapsed > (IMPACT_OFFSET * group)) {
      brightness = max(255 - map(timeElapsed, IMPACT_OFFSET * group, (IMPACT_OFFSET * group) + IMPACT_FADE, 0, 185), 0);
    }

    //actually set the face lights
    setColorOnFace(dim(teamColor[lastConnectedTeam], brightness), f);
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

#include <EEPROM.h>
#include <Servo.h>
#include <NewPing.h>
#include "ServoAnimator.h"
#include "Config.h"
#include "Animations.h"
#include <CommandQueue.h>

// Objects
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
ServoAnimator anim(NUM_JOINTS);
CommandQueue cmdQ(COMMAND_QUEUE_LENGTH);

// mode
uint8_t mode = MODE_INTERACTIVE;
boolean enableRandom = true; // true to enable random wanders, false to disable
long lastCommand, pauseUntil;

String cmd;  // cmd received over serial - builds up char at a time

COMMAND randomCmd;

unsigned int lastSonarReading = MAX_DISTANCE;
unsigned int avgSonarReading = MAX_DISTANCE;  // averaged over several samples
long sonarTimer;

void setup() {
  Serial.begin(9600);
  Serial.println("SCOB");

  // load servo centers from EEPROM
  loadConfig();

  // init servos
  for (uint8_t i=0; i<NUM_JOINTS; i++) {
      anim.initServo(i, servoPins[i], servoCenters[i]);
  }

  anim.setAnimation(stand);
  lastCommand = millis();
  sonarTimer = millis();
  pauseUntil = lastCommand;
}


void loop() {
    // Parse Logo commands from Serial
    if (Serial.available()) {
        char c = toupper(Serial.read());
        if (c == '\r' || c == '\n') {  // if found a line end
            if (cmd != "") {  // check the command isn't blank
                if (cmdQ.isFull()) {
                    Serial.println("BUSY");
                } else {
                    parseCommand(cmd);
                    Serial.println("OK:" + cmd);
                }

                // reset the command buffer
                cmd = "";
            }
        } else {
            cmd += c;  // append the character onto the command buffer
        }
        lastCommand = millis();
        mode = MODE_INTERACTIVE;
        enableRandom = false; // turn off at first interactive command, turn back on with RND
    }

  if (enableRandom && cmdQ.isEmpty() && millis() - lastCommand > 5000) {
    mode = MODE_RANDOM;
    enableRandom = false;  // disable after first random walk
  }

  // keep animation going
  anim.update();

  // take sonar readings
  if (millis() - sonarTimer > SONAR_INTERVAL) {
      lastSonarReading = sonar.ping_cm();
      avgSonarReading = (3 * avgSonarReading + lastSonarReading) / 4;
      sonarTimer = millis();
  }

  // is current movement complete?  and we're not pausing?
  if (!anim.isBusy() && millis() > pauseUntil) {
      // work out what to do next
      switch(mode){
          case MODE_INTERACTIVE:
            if (!cmdQ.isEmpty()) doCommand(cmdQ.dequeue());
            break;

          case MODE_RANDOM:
            if (cmdQ.isEmpty()) {
              int r = random(-1, 7);
              if (r == -1) {
                // rest for a little while
                pauseUntil = millis() + 5000;
              } else {
                randomCmd.cmdType = r;
                doCommand(&randomCmd);
              }
            }
            break;

        case MODE_WANDER:
            doWander();
            break;
      }
  }
}

static void parseCommand(String c) {
    // parse and queue/insert
    uint8_t cmdType = 0xff;

    // check for urgent commands
    boolean doNow = false;
    if (c[0] == '!') {
        doNow = true;
        c = c.substring(1);
    }

    // parse the command type
    if (c.startsWith("FD")) {
        cmdType = CMD_FD;
    } else if (c.startsWith("BK")) {
        cmdType = CMD_BK;
    } else if (c.startsWith("RT")) {
        cmdType = CMD_RT;
    } else if (c.startsWith("LT")) {
        cmdType = CMD_LT;
    } else if (c.startsWith("ST")) {
        cmdType = CMD_ST;
    } else if (c.startsWith("PG")) {
        cmdType = CMD_PG;
    } else if (c.startsWith("POS")) {
        cmdType = CMD_POS;
    } else if (c.startsWith("FT")) {
        cmdType = CMD_FT;
    } else if (c.startsWith("TL")) {
        cmdType = CMD_TL;
    } else if (c.startsWith("SP")) {
        cmdType = CMD_SP;
    } else if (c.startsWith("SV")) {
        cmdType = CMD_SV;
    } else if (c.startsWith("SC")) {
        cmdType = CMD_SC;
    } else if (c.startsWith("PF")) {
        cmdType = CMD_PF;
    } else if (c.startsWith("RND")) {
        mode = MODE_RANDOM;
    } else if (c.startsWith("TO")) {
        cmdType = CMD_TO;
    }

    // give up if command not recognised
    if (cmdType == 0xff) return;

    // lose the command name, keep the parameters
    int sp = c.indexOf(' ');
    if (sp > -1) {
        c = c.substring(sp+1);
    } else {
        c = "";
    }

    // insert/queue the command
    if (doNow) {
        anim.stop();  // stop the bot
        cmdQ.insert(c, cmdType);  // insert the new command at the head of the command queue
    } else {
        cmdQ.enqueue(c, cmdType);
    }

    // debug what bot is up to
    /*
    Serial.print("CQ Peek: ");
    Serial.println(cmdQ.peekAtType());
    Serial.print("bot.isQFull:");
    Serial.println(bot.isQFull());
    */
}

static void doCommand(COMMAND *c)
{
    if (c == NULL) return;

    if (c->cmdType == CMD_TO) {
        // 5 parameters to parse, all integers
        // parse directly into interactive animation
        uint8_t pn = 0;  // param number
        uint8_t sp1 = 0; // start of next param
        uint8_t sp2;
        while(1) {
            sp2 = c->cmd.indexOf(' ', sp1); // space following next param
            if (pn == 4) {
                interactiveDurations[0] = c->cmd.substring(sp1).toInt();
                break;
            } else {
                interactiveKeyFrames[0][pn] = c->cmd.substring(sp1, sp2).toInt();
                sp1 = sp2 + 1;
                pn++;
            }
        }
        updateInteractivePositions();

    } else {

        // Parse out parameter values
        int sp = c->cmd.indexOf(' ');
        float f1 = 0;
        float f2 = 0;
        if (sp > -1) {
            f1 = c->cmd.substring(0,sp).toFloat();
            f2 = c->cmd.substring(sp+1).toFloat();
        } else {
            f1 = c->cmd.toFloat();
        }

        // Handle each command type
        switch(c->cmdType) {
            case CMD_FD:
                anim.setAnimation(walkForward);
                anim.setRepeatCount(f1);
                break;
            case CMD_BK:
                anim.setAnimation(walkForward, true);
                anim.setRepeatCount(f1);
                break;
            case CMD_LT:
                anim.setAnimation(turnLeft);
                anim.setRepeatCount(f1);
                break;
            case CMD_RT:
                anim.setAnimation(turnRight);
                anim.setRepeatCount(f1);
                break;
            case CMD_ST:
                anim.setAnimation(stand);
                break;
            case CMD_PG:
                Serial.print(sonar.ping_cm());
                Serial.println("cm");
                break;

            case CMD_POS:
                if (f1 < 0 || f1 > NUM_JOINTS-1) break;
                interactiveKeyFrames[0][(uint8_t)f1] = (byte)f2;
                updateInteractivePositions();
                break;
            case CMD_FT:
                anim.setAnimation(footTap);
                anim.setRepeatCount(f1);
                break;
            case CMD_TL:
                anim.setAnimation(standTall);
                anim.setRepeatCount(f1);
                break;
            case CMD_SP:
                anim.setSpeed(f1);
                break;
            case CMD_SV:
                saveConfig();
                break;
            case CMD_SC:
                if (f1 < 0 || f1 > NUM_JOINTS-1) break;
                servoCenters[(uint8_t)f1] = (uint8_t)f2;
                anim.setServoCenter((uint8_t)f1, (uint8_t)f2);
                anim.setAnimation(stand);
                break;
            case CMD_PF:
                pauseUntil = millis() + (f1*1000);
                break;
        }
    }
}

void updateInteractivePositions() {
    anim.setAnimation(interactive);

    // debug
    for (uint8_t i=0; i<NUM_JOINTS; i++) {
        if (i>0) Serial.print(',');
        Serial.print(interactiveKeyFrames[0][i]);
    }
    Serial.println();
}

void loadConfig() {
    // read first byte, see if it equals our "magic" value
    // if not, then we've never saved values to EEPROM, so just use defaults
    uint8_t m = EEPROM.read(EEPROM_MAGIC_ADDR);
    if (m == EEPROM_MAGIC) {
        for (uint8_t i=0; i<NUM_JOINTS; i++) {
            servoCenters[i] = EEPROM.read(EEPROM_CENTERS_ADDR + i);
        }
    }
}

void saveConfig() {
    // update magic
    EEPROM.update(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);

    // update centers
    for (uint8_t i=0; i<NUM_JOINTS; i++) {
        EEPROM.update(EEPROM_CENTERS_ADDR + i, servoCenters[i]);
    }

    Serial.println("SAVED");
}

void doWander() {
    /*
    Wander algorithm:
    1) lookLeft, take sonar reading
    2) lookRight, take sonar reading
    3) stand, take sonar reading
    4) work out farthest distance reading
    5) turn to face farthest distance reading (if required)
    6) walk forward x cycles - determined by sensor distance divided by stride of ~4cm
    */

    // state enum
    typedef enum {
        LOOKLEFT,
        LOOKRIGHT,
        LOOKFWD,
        TURN,
        WALK
    } WANDER_STATE;

    // internal state variables
    static WANDER_STATE wanderState = LOOKLEFT;

    static unsigned int
        leftDist = MAX_DISTANCE,
        rightDist = MAX_DISTANCE,
        fwdDist = MAX_DISTANCE;

    static uint8_t numStrides = 1;

    switch(wanderState) {
        case LOOKLEFT:
            Serial.println("lookLeft");
            // start by turning to look left
            anim.setAnimation(lookLeft);
            anim.setRepeatCount(1);
            wanderState = LOOKRIGHT;
            break;
        case LOOKRIGHT:
            Serial.println("lookRight");
            // now looking left, so take the left sonar reading
            leftDist = sonar.ping_cm();

            // and turn to look to the right
            anim.setAnimation(lookRight);
            anim.setRepeatCount(1);
            wanderState = LOOKFWD;
            break;
        case LOOKFWD:
            Serial.println("lookFwd");
            // looking right, take right sonar reading
            rightDist = sonar.ping_cm();

            // turn to look forward
            anim.setAnimation(stand);
            anim.setRepeatCount(1);
            wanderState = TURN;
            break;
        case TURN:
            Serial.println("turn");
            // looking forwards, take final sonar reading
            fwdDist = sonar.ping_cm();

            // turn left or right, if required
            if ((leftDist > rightDist && leftDist > fwdDist) || (fwdDist < STRIDE_LENGTH)) {
                numStrides = leftDist / STRIDE_LENGTH;
                anim.setAnimation(turnLeft);
                anim.setRepeatCount(1);
            } else if (rightDist > leftDist && rightDist > fwdDist) {
                numStrides = rightDist / STRIDE_LENGTH;
                anim.setAnimation(turnRight);
                anim.setRepeatCount(1);
            } else {
                numStrides = fwdDist / STRIDE_LENGTH;
            }

            if (numStrides > 5) numStrides = 5;

            wanderState = WALK;
            break;
        case WALK:
            Serial.print("walk:");
            Serial.println(numStrides);
            // now turned to face correct direction, so walk forwards
            anim.setAnimation(walkForward);
            anim.setRepeatCount(numStrides);
            wanderState = LOOKLEFT;
            break;
    }
}

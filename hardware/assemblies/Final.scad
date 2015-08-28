


module FinalAssembly () {

    assembly("assemblies/Final.scad", "Final", str("FinalAssembly()")) {

        // base part
        Head_STL();

        // steps
        step(1, "Insert sensor") {
            view(t=[0,0,0], r=[52,0,218], d=400);

            attach(Head_Con_SensorTX, HCSR04UltrasonicSensor_Con_TXCan)
                HCSR04UltrasonicSensor();
        }

        // insert left servo
        step(2, "Left servo") {
            view();

            attach(Head_Con_LeftServo, MicroServo_Con_Fixing1) {
                MicroServo();
                attach(MicroServo_Con_Horn, DefConDown)
                    ServoHorn();
            }
        }

        // insert right servo
        step(3, "Right servo") {
            view();

            attach(Head_Con_RightServo, MicroServo_Con_Fixing1) {
                MicroServo();
                attach(MicroServo_Con_Horn, DefConDown)
                    ServoHorn();
            }
        }

        step(4, "Power switch") {
            view();

            attach(Head_Con_PowerSwitch, MiniToggleSwitch_Con_Def)
                MiniToggleSwitch(washerOffset=dw);
        }

        step(5, "Shove in an Arduino") {
            view();

            attach(Head_Con_Arduino, ArduinoPro_Con_Center)
                ArduinoPro(headerpins=1, serialpins=1);
        }


        step(6, "Followed by a battery pack") {
            view();

            attach(Head_Con_BatteryPack,DefConDown)
                {
                    for (i=[0:3])
                        translate([i*11 - 16.5,0,0])
                        Battery(type=Battery_AAA);
                }
        }

        step(7, "Attach the legs") {
            view();

            attach(Head_Con_LeftServo, MicroServo_Con_Fixing1) {
                attach(
                    rollConnector(MicroServo_Con_Horn, -90),
                    offsetConnector(invertConnector(LeftLeg_Con_Hip), [0,0,-4.5])
                ) {
                    LeftLeg_STL();

                    attach(LeftLeg_Con_Ankle, LeftFootAssembly_Con_Servo)
                        LeftLegAssembly();
                }

            }

            attach(Head_Con_RightServo, MicroServo_Con_Fixing1) {
                attach(
                    rollConnector(MicroServo_Con_Horn, -90),
                    offsetConnector(invertConnector(RightLeg_Con_Hip), [0,0,-4.5])
                ) {
                    RightLeg_STL();

                    attach(RightLeg_Con_Ankle, RightFootAssembly_Con_Servo)
                        RightLegAssembly();
                }
            }
        }

    }
}

# GPIO Pinbelegung (Legacy-ID zu BCM)

Stand: 2026-07-28

Diese Tabelle dokumentiert die im Projekt verwendeten Legacy-Pin-IDs
(historisch WiringPi-Nummerierung) und deren Zuordnung auf BCM-GPIO.

## Mapping-Tabelle

| Legacy-ID | BCM-GPIO | Verwendung im Projekt |
|---:|---:|---|
| 10 | 8  | Debug-Ausgang (DEB_0) |
| 11 | 7  | IR_OUT |
| 21 | 5  | PC_OUT_MOVE_GIER_LEFT |
| 22 | 6  | PC_OUT_MOVE_GIER_RIGHT |
| 23 | 13 | PC_OUT_MOVE_NICK_UP |
| 24 | 19 | PC_OUT_MOVE_NICK_DOWN |
| 25 | 26 | PC_OUT_LOCK_REMOTE, TP_OUT_5 |
| 26 | 12 | SERVO_PWM_ZOOM |
| 27 | 16 | PC_OUT_1 |
| 28 | 20 | PC_OUT_2, PC_IN_1, TP_IN_1 |
| 29 | 21 | PC_OUT_3, PC_IN_2, TP_IN_2 |
| 31 | 1  | IR_IN |

## Hinweis

- Die technische Zuordnung erfolgt zentral in System_Raspberry/pigpio_adapter.c
	in der Funktion map_legacy_pin_to_bcm().
- I2C wird ueber den pigpio-Adapter abgewickelt (RPI_I2COpen,
	RPI_I2CReadReg8, RPI_I2CWriteReg8).
